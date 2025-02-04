/*
 * matrix_rain.c
 *
 * Matrix Rain simulation with lightning effects using SDL2.
 * All comments have been standardized for clarity and consistency.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* Configuration */
#define FONT_SIZE 16

/* Global window dimensions */
int g_screen_width = 800;
int g_screen_height = 600;

/* Global physics constants */
#define GRAVITY 10.0f
#define TERMINAL_VELOCITY 100.0f
#define WIND_RESPONSE 2.0f

/* Unicode Characters */

/* List of Unicode characters: Hiragana, Katakana, Latin, Cyrillic, Numbers,
   Math symbols, Greek Alphabet, and Chinese characters. */
const char* unicode_chars[] = {
    /* Hiragana */
    "あ", "い", "う", "え", "お",
    "か", "き", "く", "け", "こ",
    "さ", "し", "す", "せ", "そ",
    "た", "ち", "つ", "て", "と",
    "な", "に", "ぬ", "ね", "の",
    "は", "ひ", "ふ", "へ", "ほ",
    "ま", "み", "む", "め", "も",
    "や", "ゆ", "よ",
    "ら", "り", "る", "れ", "ろ",
    "わ", "を", "ん",
    
    /* Katakana */
    "ア", "イ", "ウ", "エ", "オ",
    "カ", "キ", "ク", "ケ", "コ",
    "サ", "シ", "ス", "セ", "ソ",
    "タ", "チ", "ツ", "テ", "ト",
    "ナ", "ニ", "ヌ", "ネ", "ノ",
    "ハ", "ヒ", "フ", "ヘ", "ホ",
    "マ", "ミ", "ム", "メ", "モ",
    "ヤ", "ユ", "ヨ",
    "ラ", "リ", "ル", "レ", "ロ",
    "ワ", "ヲ", "ン",
    
    /* Latin */
    "A", "B", "C", "D", "E", "F", "G", "H", "I", 
    "J", "K", "L", "M", "N", "O", "P", "Q", "R", 
    "S", "T", "U", "V", "W", "X", "Y", "Z", 
    "a", "b", "c", "d", "e", "f", "g", "h", "i", 
    "j", "k", "l", "m", "n", "o", "p", "q", "r", 
    "s", "t", "u", "v", "w", "x", "y", "z",
    
    /* Cyrillic */
    "Б", "Д", "Ж", "З", "И", "Л", "У", "Ц", 
    "Ч", "Ш", "Щ", "Ъ", "Ь", "Э", "Ю", "Я",
    
    /* Numbers */
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    
    /* Math symbols */
    "+", "-", "×", "÷", "=", "≠", "≤", "≥", "±", 
    "∑", "∏", "√", "∞", "∫", "∂", "∆", "∇", "∈", 
    "∉", "∋", "∅", "∧", "∨", "⊕", "⊗", "⊆", "⊇", 
    "∝", "∴", "∵", "∃", "∀", "∩", "∪", "≈", "≅",
    
    /* Greek Alphabet */
    "Α", "Β", "Γ", "Δ", "Θ", "Ι", "Λ", "Ξ", "Π", 
    "Σ", "Φ", "Ψ", "Ω", "α", "β", "γ", "δ", "ε", 
    "ζ", "η", "θ", "ι", "κ", "λ", "μ", "ν", "ξ", 
    "ο", "π", "ρ", "σ", "τ", "υ", "φ", "χ", "ψ", "ω",
    
    /* Chinese Characters */
    "你", "好", "我", "是", "天", "地", "人", "山", "水", "火", 
    "大", "小", "中", "国", "学", "生", "爱", "书", "车", "猫", 
    "狗", "月", "日", "年", "风", "雨", "花", "草", "树", "家",
    "鼠", "牛", "虎", "兔", "龙", "蛇", "马", "羊", "猴", "鸡", 
    "猪", "星", "空", "光", "影", "梦", "夜", "晨", "时", "钟", 
    "金", "银", "玉", "石", "海", "湖", "江", "河", "山", "川",
};

#define NUM_UNICODE_CHARS (sizeof(unicode_chars) / sizeof(unicode_chars[0]))

/* Pre-rendered textures for Unicode characters */
SDL_Texture *unicode_textures[NUM_UNICODE_CHARS];

/* Data Structures */

/* Falling column for matrix rain */
typedef struct {
    float x;                  /* Horizontal position (head) in pixels */
    float y;                  /* Vertical position (head) in pixels */
    float vx;                 /* Horizontal velocity (pixels/s) */
    float vy;                 /* Vertical velocity (pixels/s) */
    int length;               /* Number of characters in the column */
    float depth;              /* Brightness factor (0.0 to 1.0) */
    int *indices;             /* Array of indices into unicode_chars */
    float char_update_timer;  /* Timer for character updates */
} Column;

/* Lightning branch structure */
typedef struct {
    SDL_Point *points;
    int num_points;
} LightningBranch;

/* Lightning effect representation */
typedef struct {
    float timer;              /* Remaining time for the effect */
    float initial_timer;      /* Initial duration */
    int effect_type;          /* 0: bolt, 1: full-screen flash */
    SDL_Point *points;        /* Main bolt points */
    int num_points;           /* Number of main bolt points */

    /* Precomputed branches (constant during the effect) */
    LightningBranch *branches;
    int num_branches;
} LightningEffect;

/* Global Variables */

/* Array of active falling columns */
Column **columns = NULL;
size_t num_columns = 0;
size_t columns_capacity = 0;

/* SDL objects */
SDL_Window   *window   = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font     *font     = NULL;
SDL_Texture  *canvas   = NULL;  /* Offscreen render target for trail effect */

int char_width, char_height;     /* Character dimensions (monospace) */
Uint32 last_ticks = 0;

LightningEffect *lightning = NULL;

/* Wind effect variables */
float current_wind_angle = 0.0f;       /* Current wind angle (degrees) */
float target_wind_angle = 0.0f;        /* Target wind angle (degrees) */
float wind_start_angle = 0.0f;         /* Wind angle at transition start */
float wind_idle_timer = 3.0f;          /* Idle duration before wind change */
float wind_transition_timer = 0.0f;    /* Timer during wind transition */
float wind_transition_duration = 0.0f; /* Transition duration */
bool wind_in_transition = false;       /* Flag: wind is transitioning */

/* Utility Functions */

/* Returns a random index for the unicode_chars array */
int random_unicode_index(void) {
    return rand() % NUM_UNICODE_CHARS;
}

/* Create a new falling column at the given horizontal position */
Column *create_column(int col_index) {
    Column *col = malloc(sizeof(Column));
    if (!col) return NULL;
    col->x = (float)col_index;
    col->y = -(rand() % g_screen_height);
    col->length = 5 + rand() % 23;
    col->depth = (float)(rand() % 101) / 100.0f;
    col->char_update_timer = 0.0f;
    col->indices = malloc(col->length * sizeof(int));
    if (!col->indices) {
        free(col);
        return NULL;
    }
    for (int i = 0; i < col->length; i++) {
        col->indices[i] = random_unicode_index();
    }
    /* Initialize vertical speed (50-200 pixels/s); no horizontal speed */
    col->vy = 50.0f + (float)(rand() % 150);
    col->vx = 0.0f;
    return col;
}

/* Free the memory allocated for a column */
void destroy_column(Column *col) {
    if (col) {
        free(col->indices);
        free(col);
    }
}

/* Initialize pre-rendered textures for Unicode characters */
void init_unicode_textures(void) {
    for (int i = 0; i < NUM_UNICODE_CHARS; i++) {
        SDL_Color white = { 255, 255, 255, 255 };
        /* Render UTF-8 string to surface */
        SDL_Surface *surf = TTF_RenderUTF8_Solid(font, unicode_chars[i], white);
        if (!surf) {
            printf("Failed to render '%s': %s\n", unicode_chars[i], TTF_GetError());
            continue;
        }
        unicode_textures[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }
    /* Set character dimensions based on the first texture */
    if (unicode_textures[0]) {
        SDL_QueryTexture(unicode_textures[0], NULL, NULL, &char_width, &char_height);
    }
}

/* 
 * Compute the wind influence factor for a column based on its x position.
 * During a wind transition, a "wave" propagates across the screen:
 *   - If the wind is increasing (target_wind_angle > wind_start_angle), the wind
 *     comes from the left. Columns with x values below the wave front get full effect.
 *   - If the wind is decreasing (target_wind_angle < wind_start_angle), the wind 
 *     comes from the right.
 *
 * The transition zone (over which columns gradually come under wind's influence) is
 * made dynamic based on the magnitude of the change in wind angle.
 */
float get_wind_factor(float col_x) {
    if (!wind_in_transition)
        return 1.0f;  // if not in transition, all columns receive full effect

    float wave_progress = wind_transition_timer / wind_transition_duration;  // [0,1]
    float angle_diff = fabs(target_wind_angle - wind_start_angle);
    // A larger wind change should cause a faster (shorter) transition zone.
    float zone = 50.0f - (angle_diff * 0.2f);
    if (zone < 10.0f)
        zone = 10.0f;

    if (target_wind_angle > wind_start_angle) {
        // Wind emerges from the left; wave front moves right.
        float wave_front = wave_progress * g_screen_width;
        if (col_x <= wave_front) {
            return 1.0f;
        } else if (col_x < wave_front + zone) {
            return 1.0f - ((col_x - wave_front) / zone);
        } else {
            return 0.0f;
        }
    } else {
        // Wind emerges from the right; wave front moves left.
        float wave_front = g_screen_width - (wave_progress * g_screen_width);
        if (col_x >= wave_front) {
            return 1.0f;
        } else if (col_x > wave_front - zone) {
            return 1.0f - ((wave_front - col_x) / zone);
        } else {
            return 0.0f;
        }
    }
}

/* Update falling columns: position, velocity, and character content */
void update_columns(float delta) {
    size_t write_index = 0;
    int extended_margin = char_height * 50;  /* Retain columns within extended bounds */
    
    for (size_t i = 0; i < num_columns; i++) {
        Column *col = columns[i];

        /* Apply gravity */
        col->vy += GRAVITY * delta;
        if (col->vy > TERMINAL_VELOCITY) col->vy = TERMINAL_VELOCITY;

        /* Adjust horizontal velocity based on wind with a staggered (swishing) effect */
        float target_vx = tan(current_wind_angle * M_PI / 180.0f) * col->vy;
        float wind_factor = get_wind_factor(col->x);  // Compute per-column delay based on its x coordinate.
        col->vx += (target_vx - col->vx) * WIND_RESPONSE * wind_factor * delta;

        /* Update position */
        col->x += col->vx * delta;
        col->y += col->vy * delta;

        /* Update characters periodically */
        col->char_update_timer += delta;
        if (col->char_update_timer > 0.1f) {
            for (int j = 0; j < col->length; j++) {
                if (rand() % 2 == 0) {
                    col->indices[j] = random_unicode_index();
                }
            }
            col->char_update_timer = 0.0f;
        }

        /* Compute fall angle and letter offsets */
        float fall_angle = atan2(col->vx, col->vy);
        float dx = -char_height * sin(fall_angle);
        float dy = -char_height * cos(fall_angle);

        /* Calculate bounding box for the column */
        float letter0_x = col->x;
        float letter_end_x = col->x + (col->length - 1) * dx;
        float min_x = (letter0_x < letter_end_x) ? letter0_x : letter_end_x;
        float max_x = (letter0_x > letter_end_x) ? letter0_x : letter_end_x;

        float letter0_y = col->y;
        float letter_end_y = col->y + (col->length - 1) * dy;
        float min_y = (letter0_y < letter_end_y) ? letter0_y : letter_end_y;
        float max_y = (letter0_y > letter_end_y) ? letter0_y : letter_end_y;

        /* Retain columns that are within the extended margin */
        if (max_y >= -extended_margin && min_y <= g_screen_height + extended_margin &&
            max_x >= -extended_margin && min_x <= g_screen_width + extended_margin) {
            columns[write_index++] = col;
        } else {
            destroy_column(col);
        }
    }
    num_columns = write_index;

    /* Occasionally spawn a new column over an extended range */
    int margin = char_height * 50;
    if ((rand() % 100) < 20) {
        int col_index = (rand() % (g_screen_width + 2 * margin)) - margin;
        Column *newcol = create_column(col_index);
        if (newcol) {
            if (num_columns >= columns_capacity) {
                size_t new_capacity = (columns_capacity == 0) ? 16 : columns_capacity * 2;
                Column **new_columns = realloc(columns, new_capacity * sizeof(Column *));
                if (new_columns) {
                    columns = new_columns;
                    columns_capacity = new_capacity;
                } else {
                    destroy_column(newcol);
                    return;
                }
            }
            columns[num_columns++] = newcol;
        }
    }
}

/* Render all falling columns */
void render_columns(void) {
    for (int i = 0; i < num_columns; i++) {
        Column *col = columns[i];
        
        /* Calculate scale and horizontal offset based on depth */
        float scale = 0.5f + 0.5f * col->depth;
        int scaled_width = (int)(char_width * scale);
        int offset = (char_width - scaled_width) / 2;
        
        /* Determine fall rotation angle */
        float fall_angle = atan2(col->vx, col->vy);
        float angle_deg = -fall_angle * 180.0f / M_PI;
        
        /* Calculate displacement between successive characters */
        float dx = -char_height * sin(fall_angle);
        float dy = -char_height * cos(fall_angle);
        
        for (int j = 0; j < col->length; j++) {
            float letterX = col->x + j * dx;
            float letterY = col->y + j * dy;
            if (letterY < -char_height || letterY > g_screen_height) continue;
            
            int index = col->indices[j];
            SDL_Texture *tex = unicode_textures[index];
            if (!tex) continue;
            
            SDL_Color color;
            if (j == 0) {
                /* Head of column: white */
                color = (SDL_Color){255, 255, 255, 255};
            } else {
                /* Tail: green varying by depth */
                int brightness = (int)(col->depth * 200) + 55;
                if (brightness > 255) brightness = 255;
                color = (SDL_Color){0, brightness, 0, 255};
            }
            SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
            
            SDL_Rect dst = { (int)letterX + offset, (int)letterY, scaled_width, char_height };
            SDL_Point center = { dst.w / 2, dst.h / 2 };
            
            SDL_RenderCopyEx(renderer, tex, NULL, &dst, angle_deg, &center, SDL_FLIP_NONE);
        }
    }
}

/* Handle SDL events (quit and window resize) */
void handle_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#else
            exit(0);
#endif
        }
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                SDL_GetWindowSize(window, &g_screen_width, &g_screen_height);
                printf("Window resized to: %dx%d\n", g_screen_width, g_screen_height);
                if (canvas) {
                    SDL_DestroyTexture(canvas);
                }
                canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                             g_screen_width, g_screen_height);
                if (!canvas) {
                    printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
                    exit(1);
                }
                SDL_SetRenderTarget(renderer, canvas);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                SDL_SetRenderTarget(renderer, NULL);
            }
        }
    }
}

/* Lightning Effect Functions */

/* Generate fractal points for a lightning bolt */
SDL_Point* generate_fractal_lightning_points(int startX, int startY, int endX, int endY,
                                               float displacement, int detail, int *num_points) {
    int current_count = 2;
    SDL_Point *points = malloc(current_count * sizeof(SDL_Point));
    if (!points) return NULL;
    points[0].x = startX; points[0].y = startY;
    points[1].x = endX;   points[1].y = endY;
    
    for (int i = 0; i < detail; i++) {
        int new_count = current_count * 2 - 1;
        SDL_Point *new_points = malloc(new_count * sizeof(SDL_Point));
        if (!new_points) {
            free(points);
            return NULL;
        }
        new_points[0] = points[0];
        for (int j = 0; j < current_count - 1; j++) {
            SDL_Point A = points[j];
            SDL_Point B = points[j+1];
            float midX = (A.x + B.x) / 2.0f;
            float midY = (A.y + B.y) / 2.0f;
            
            float dx = B.x - A.x;
            float dy = B.y - A.y;
            float norm = sqrtf(dx * dx + dy * dy);
            float perpX = 0, perpY = 0;
            if (norm != 0) {
                perpX = -dy / norm;
                perpY = dx / norm;
            }
            
            /* Limit offset magnitude */
            float effective_range = displacement;
            if (fabs((float)(B.x - A.x)) > 0.001f && fabs(perpX) > 1e-6) {
                float max_allowed = (fabs((float)(B.x - A.x)) / 2.0f) / fabs(perpX);
                effective_range = fmin(displacement, max_allowed);
            }
            float random_offset = ((float)rand() / (float)RAND_MAX) * 2.0f * effective_range - effective_range;
            midX += perpX * random_offset;
            midY += perpY * random_offset;
            
            /* Enforce vertical ordering */
            if (midY < A.y + 1) midY = A.y + 1;
            if (midY > B.y - 1) midY = B.y - 1;
            
            new_points[j * 2 + 1].x = (int)midX;
            new_points[j * 2 + 1].y = (int)midY;
            new_points[j * 2 + 2] = B;
        }
        free(points);
        points = new_points;
        current_count = new_count;
        displacement /= 2.0f;
    }
    *num_points = current_count;
    return points;
}

/* Create a new lightning effect */
LightningEffect* generate_lightning() {
    LightningEffect* l = malloc(sizeof(LightningEffect));
    if (!l) return NULL;
    /* 50% chance for full-screen flash (effect_type 1), otherwise bolt (0) */
    l->effect_type = (rand() % 2 == 0) ? 1 : 0;
    float initial_displacement = 0.0f;
    if (l->effect_type == 1) {
        l->timer = 0.5f;
        l->initial_timer = 0.5f;
        l->points = NULL;
        l->num_points = 0;
    } else {
        l->timer = 1.5f;
        l->initial_timer = 1.5f;
        /* Generate fractal bolt */
        int startX = rand() % g_screen_width;
        int startY = 0;
        int endX = rand() % g_screen_width;
        /* Target between 70% and 100% of screen height */
        int endY = (g_screen_height * (70 + rand() % 31)) / 100;
        initial_displacement = g_screen_width / 8.0f;
        int detail = 6;  /* Recursion depth */
        l->points = generate_fractal_lightning_points(startX, startY, endX, endY,
                                                      initial_displacement, detail, &l->num_points);
    }
    
    /* Pre-allocate space for branches */
    l->branches = malloc((l->num_points ? l->num_points : 1) * sizeof(LightningBranch));
    l->num_branches = 0;
    if (l->points && l->num_points > 1) {
        for (int i = 0; i < l->num_points - 1; i++) {
            if (rand() % 100 < 25) {  /* 25% chance to spawn a branch */
                SDL_Point start = l->points[i];
                /* Force branch to be mostly downward */
                float branch_angle = 1.5708f - 0.7854f + ((float)rand()/(float)RAND_MAX) * (0.7854f * 2);
                int branch_length = 50 + rand() % 51;  /* 50 to 100 pixels */
                int branch_endX = start.x + (int)(branch_length * cosf(branch_angle));
                int branch_endY = start.y + (int)(branch_length * sinf(branch_angle));
                if (branch_endX < 0) branch_endX = 0;
                if (branch_endX >= g_screen_width) branch_endX = g_screen_width - 1;
                if (branch_endY < start.y + 1) branch_endY = start.y + 1;
                if (branch_endY >= g_screen_height) branch_endY = g_screen_height - 1;
                int branch_num_points = 0;
                SDL_Point *branch_points = generate_fractal_lightning_points(start.x, start.y,
                                                                             branch_endX, branch_endY,
                                                                             initial_displacement / 2.0f, 3, &branch_num_points);
                if (branch_points && branch_num_points >= 2) {
                    l->branches[l->num_branches].points = branch_points;
                    l->branches[l->num_branches].num_points = branch_num_points;
                    l->num_branches++;
                } else if (branch_points) {
                    free(branch_points);
                }
            }
        }
    }
    
    return l;
}

/*
 * New helper function: draw_smooth_lightning_bolt
 *
 * This version computes a varying thickness along the bolt: the thickness is
 * highest in the center (at progress = 0.5) and tapers down to a specified minimum
 * at the ends (progress = 0 and 1).
 */
void draw_smooth_lightning_bolt(LightningEffect *l, int max_thickness, SDL_Color color) {
    int n = l->num_points;
    if (n < 2) return;

    int vertex_count = n * 2;
    SDL_Vertex *vertices = malloc(vertex_count * sizeof(SDL_Vertex));
    if (!vertices) return;

    // Set the minimum thickness at the edges (pointy ends)
    float min_thickness = 1.0f;  // adjust as needed

    for (int i = 0; i < n; i++) {
        SDL_Point p = l->points[i];
        // Compute the progress along the bolt [0, 1]
        float progress = (float)i / (n - 1);
        // Linear taper: maximum at center, min_thickness at the ends.
        // (1 - |2*progress - 1|) gives 0 at 0 and 1, and 1 at 0.5.
        float local_thickness = min_thickness + (max_thickness - min_thickness) * (1.0f - fabs(2.0f * progress - 1.0f));

        float tangent_x, tangent_y;
        if (i == 0) {
            tangent_x = l->points[i+1].x - p.x;
            tangent_y = l->points[i+1].y - p.y;
        } else if (i == n - 1) {
            tangent_x = p.x - l->points[i-1].x;
            tangent_y = p.y - l->points[i-1].y;
        } else {
            tangent_x = l->points[i+1].x - l->points[i-1].x;
            tangent_y = l->points[i+1].y - l->points[i-1].y;
        }
        float len = sqrtf(tangent_x * tangent_x + tangent_y * tangent_y);
        if (len == 0) { tangent_x = 1; tangent_y = 0; len = 1; }
        tangent_x /= len;
        tangent_y /= len;

        // Compute normal vector (perpendicular to the tangent)
        float normal_x = -tangent_y;
        float normal_y = tangent_x;

        // Create two vertices offset in opposite directions by the local thickness
        vertices[2 * i].position.x     = p.x + normal_x * local_thickness;
        vertices[2 * i].position.y     = p.y + normal_y * local_thickness;
        vertices[2 * i].color          = color;

        vertices[2 * i + 1].position.x = p.x - normal_x * local_thickness;
        vertices[2 * i + 1].position.y = p.y - normal_y * local_thickness;
        vertices[2 * i + 1].color      = color;
    }

    // Build indices from the triangle strip by converting to triangles.
    int num_triangles = vertex_count - 2;
    int indices_count = num_triangles * 3;
    int *indices = malloc(indices_count * sizeof(int));
    if (!indices) {
        free(vertices);
        return;
    }
    int idx = 0;
    for (int i = 0; i < vertex_count - 2; i++) {
        if (i % 2 == 0) {
            indices[idx++] = i;
            indices[idx++] = i + 1;
            indices[idx++] = i + 2;
        } else {
            indices[idx++] = i + 1;
            indices[idx++] = i;
            indices[idx++] = i + 2;
        }
    }

    SDL_RenderGeometry(renderer, NULL, vertices, vertex_count, indices, indices_count);

    free(vertices);
    free(indices);
}

/*
 * Updated draw_lightning function: renders both the main bolt and its branches
 * using a smooth filled polygon with tapered thickness.
 */
void draw_lightning(LightningEffect *l) {
    /* Compute fade alpha */
    float alpha_factor = l->timer / l->initial_timer;
    Uint8 alpha = (Uint8)(255 * alpha_factor);
    SDL_Color white = { 255, 255, 255, alpha };
    SDL_Color glowColor = white;
    glowColor.a = (Uint8)(alpha * 0.5f); // glow with reduced alpha

    int base_thickness = 3;  /* Maximum thickness at the center of the bolt */

    /* Draw a smooth glowing bolt:
     * First, draw an outer glow (using a higher max thickness),
     * then draw the main bolt on top.
     */
    draw_smooth_lightning_bolt(l, base_thickness + 4, glowColor);
    draw_smooth_lightning_bolt(l, base_thickness, white);

    /* Draw branches with a similar tapering effect */
    for (int i = 0; i < l->num_branches; i++) {
        LightningBranch branch = l->branches[i];
        int n = branch.num_points;
        if (n < 2)
            continue;

        int vertex_count = n * 2;
        SDL_Vertex *vertices = malloc(vertex_count * sizeof(SDL_Vertex));
        if (!vertices)
            continue;

        float min_thickness = 1.0f;  // minimum thickness at branch ends
        for (int j = 0; j < n; j++) {
            SDL_Point p = branch.points[j];
            float progress = (float)j / (n - 1);
            float local_thickness = min_thickness + (base_thickness - min_thickness) * (1.0f - fabs(2.0f * progress - 1.0f));

            float tangent_x, tangent_y;
            if (j == 0) {
                tangent_x = branch.points[j+1].x - p.x;
                tangent_y = branch.points[j+1].y - p.y;
            } else if (j == n - 1) {
                tangent_x = p.x - branch.points[j-1].x;
                tangent_y = p.y - branch.points[j-1].y;
            } else {
                tangent_x = branch.points[j+1].x - branch.points[j-1].x;
                tangent_y = branch.points[j+1].y - branch.points[j-1].y;
            }
            float len = sqrtf(tangent_x * tangent_x + tangent_y * tangent_y);
            if (len == 0) { tangent_x = 1; tangent_y = 0; len = 1; }
            tangent_x /= len;
            tangent_y /= len;
            float normal_x = -tangent_y;
            float normal_y = tangent_x;

            vertices[2*j].position.x     = p.x + normal_x * local_thickness;
            vertices[2*j].position.y     = p.y + normal_y * local_thickness;
            vertices[2*j].color          = white;
            vertices[2*j+1].position.x   = p.x - normal_x * local_thickness;
            vertices[2*j+1].position.y   = p.y - normal_y * local_thickness;
            vertices[2*j+1].color        = white;
        }
        int num_triangles = vertex_count - 2;
        int indices_count = num_triangles * 3;
        int *indices = malloc(indices_count * sizeof(int));
        if (indices) {
            int idx = 0;
            for (int j = 0; j < vertex_count - 2; j++) {
                if(j % 2 == 0) {
                    indices[idx++] = j;
                    indices[idx++] = j+1;
                    indices[idx++] = j+2;
                } else {
                    indices[idx++] = j+1;
                    indices[idx++] = j;
                    indices[idx++] = j+2;
                }
            }
            SDL_RenderGeometry(renderer, NULL, vertices, vertex_count, indices, indices_count);
            free(indices);
        }
        free(vertices);
    }
}

/* Main loop: handle events, update simulation, and render scene */
void main_loop(void *arg) {
    handle_events();
    Uint32 current_ticks = SDL_GetTicks();
    float delta = (current_ticks - last_ticks) / 1000.0f;
    last_ticks = current_ticks;
    
    /* Update wind effect */
    if (wind_in_transition) {
        wind_transition_timer += delta;
        float t = wind_transition_timer / wind_transition_duration;
        if (t >= 1.0f) {
            current_wind_angle = target_wind_angle;
            wind_in_transition = false;
            wind_idle_timer = 3.0f + ((float)rand() / (float)RAND_MAX) * 5.0f;
            wind_transition_timer = 0.0f;
            wind_transition_duration = 0.0f;
        } else {
            current_wind_angle = wind_start_angle + (target_wind_angle - wind_start_angle) * t;
        }
    } else {
        wind_idle_timer -= delta;
        if (wind_idle_timer <= 0) {
            wind_in_transition = true;
            wind_transition_duration = 1.0f + (((float)rand() / (float)RAND_MAX) * 4.0f);
            wind_transition_timer = 0.0f;
            wind_start_angle = current_wind_angle;
            target_wind_angle = -45.0f + (((float)rand() / (float)RAND_MAX) * 90.0f);
        }
    }
    
    SDL_SetRenderTarget(renderer, canvas);
    /* Apply fade effect for trail */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_RenderFillRect(renderer, NULL);
    
    update_columns(delta);
    render_columns();
    
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, canvas, NULL, NULL);
    
    /* Handle lightning effect */
    if (lightning) {
        lightning->timer -= delta;
        if (lightning->effect_type == 1) { 
            float alpha_factor = lightning->timer / lightning->initial_timer;
            if (alpha_factor < 0) alpha_factor = 0;
            Uint8 fade_alpha = (Uint8)(255 * alpha_factor);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, fade_alpha);
            SDL_RenderFillRect(renderer, NULL);
        } else {
            draw_lightning(lightning);
        }
        if (lightning->timer <= 0) {
            free(lightning->points);
            for (int i = 0; i < lightning->num_branches; i++) {
                free(lightning->branches[i].points);
            }
            free(lightning->branches);
            free(lightning);
            lightning = NULL;
        }
    } else {
        /* Approximately 0.6% chance per frame to spawn lightning */
        if (rand() % 1000 < 6) {
            lightning = generate_lightning();
        }
    }
    
    SDL_RenderPresent(renderer);
}

/* Main entry point */
int main(int argc, char *argv[]) {
    printf("Matrix Rain starting...\n");
    srand((unsigned)time(NULL));
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
#ifdef __EMSCRIPTEN__
    /* Set attributes for WebGL/OpenGL ES */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
    
    /* Create SDL window */
    window = SDL_CreateWindow("Matrix Rain Screen", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                g_screen_width, g_screen_height,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    /* Create renderer with hardware acceleration and vsync */
    renderer = SDL_CreateRenderer(window, -1,
                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    font = TTF_OpenFont("matrix_font_subset.ttf", FONT_SIZE);
    if (!font) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    init_unicode_textures();
    
    canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                               g_screen_width, g_screen_height);
    if (!canvas) {
        printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderTarget(renderer, canvas);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);
    
    last_ticks = SDL_GetTicks();
    
    /* Allocate array for falling columns */
    columns_capacity = 16;
    columns = malloc(columns_capacity * sizeof(Column *));
    if (!columns) {
        printf("Failed to allocate columns array.\n");
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop, NULL, 0, 1);
#else
    while (1) {
        main_loop(NULL);
        SDL_Delay(16);  /* ~60 FPS */
    }
#endif
    
    /* Cleanup (unreachable in some environments) */
    for (int i = 0; i < NUM_UNICODE_CHARS; i++) {
        if (unicode_textures[i])
            SDL_DestroyTexture(unicode_textures[i]);
    }
    for (size_t i = 0; i < num_columns; i++) {
        destroy_column(columns[i]);
    }
    free(columns);
    if (canvas) SDL_DestroyTexture(canvas);
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
