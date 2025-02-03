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

/* ------------------------ Configuration ------------------------ */

#define FONT_SIZE     16

/* Global (dynamic) window size variables */
int g_screen_width = 800;
int g_screen_height = 600;

/* Add these global constants right after your configuration section */
#define GRAVITY 10.0f
#define TERMINAL_VELOCITY 100.0f
#define WIND_RESPONSE 2.0f

/* ------------------------ Unicode Characters Data Structure ------------------------ */

const char* unicode_chars[] = {
    // Hiragana
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
    
    // Katakana
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
    
    // Latin
    "A", "B", "C", "D", "E", "F", "G", "H", "I", 
    "J", "K", "L", "M", "N", "O", "P", "Q", "R", 
    "S", "T", "U", "V", "W", "X", "Y", "Z", "a", 
    "b", "c", "d", "e", "f", "g", "h", "i", "j", 
    "k", "l", "m", "n", "o", "p", "q", "r", "s", 
    "t", "u", "v", "w", "x", "y", "z",
    
    // Cyrillic
    "Б", "Д", "Ж", "З", "И", "Л", "У", "Ц", 
    "Ч", "Ш", "Щ", "Ъ", "Ь", "Э", "Ю", "Я",
    
    // Numbers
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    
    // Math symbols
    "+", "-", "×", "÷", "=", "≠", "≤", "≥", "±", 
    "∑", "∏", "√", "∞", "∫", "∂", "∆", "∇", "∈", 
    "∉", "∋", "∅", "∧", "∨", "⊕", "⊗", "⊆", "⊇", 
    "∝", "∴", "∵", "∃", "∀", "∩", "∪", "≈", "≅",
    
    // Greek Alphabet
    "Α", "Β", "Γ", "Δ", "Θ", "Ι", "Λ", "Ξ", "Π", 
    "Σ", "Φ", "Ψ", "Ω", "α", "β", "γ", "δ", "ε", 
    "ζ", "η", "θ", "ι", "κ", "λ", "μ", "ν", "ξ", 
    "ο", "π", "ρ", "σ", "τ", "υ", "φ", "χ", "ψ", "ω"

    // Chinese Characters
    "你", "好", "我", "是", "天", "地", "人", "山", "水", "火", 
    "大", "小", "中", "国", "学", "生", "爱", "书", "车", "猫", 
    "狗", "月", "日", "年", "风", "雨", "花", "草", "树", "家",
    "鼠", "牛", "虎", "兔", "龙", "蛇", "马", "羊", "猴", "鸡", 
    "猪", "星", "空", "光", "影", "梦", "夜", "晨", "时", "钟", 
    "金", "银", "玉", "石", "海", "湖", "江", "河", "山", "川",
};


#define NUM_UNICODE_CHARS (sizeof(unicode_chars) / sizeof(unicode_chars[0]))

/* Pre‑rendered textures for each Unicode character */
SDL_Texture *unicode_textures[NUM_UNICODE_CHARS];

/* ------------------------ Data Structures ------------------------ */

/* A falling column in the Matrix rain */
typedef struct {
    float x;             // horizontal position (in pixels) of the column's head
    float y;             // vertical position (in pixels) for the head character
    float vx;            // horizontal velocity (pixels per second)
    float vy;            // vertical velocity (pixels per second)
    int length;          // number of characters in the column
    float depth;         // depth factor (0.0 to 1.0) for brightness variations
    int *indices;        // array of indices into the unicode_chars array (length = 'length')
    float char_update_timer; // timer used to periodically change characters
} Column;

typedef struct {
    SDL_Point *points;
    int num_points;
} LightningBranch;

/* Updated Lightning Effect Structures */
typedef struct {
    float timer;
    float initial_timer;
    int effect_type; // 0: bolt only, 1: full screen flash
    SDL_Point *points;
    int num_points;

    // Precomputed branches (these won't change each frame)
    LightningBranch *branches;
    int num_branches;
} LightningEffect;

/* ------------------------ Global Variables ------------------------ */

Column **columns = NULL;
size_t num_columns = 0;
size_t columns_capacity = 0;

/* SDL objects */
SDL_Window   *window   = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font     *font     = NULL;
SDL_Texture  *canvas   = NULL;  // offscreen render target for the trail effect

int char_width, char_height;     // dimensions of a character (assumes monospace)
Uint32 last_ticks = 0;

LightningEffect *lightning = NULL;

/* Global Wind Effect Variables */
float current_wind_angle = 0.0f;    // The current wind angle (in degrees)
float target_wind_angle = 0.0f;     // The new random wind target angle
float wind_start_angle = 0.0f;      // The starting angle at the beginning of a transition
float wind_idle_timer = 3.0f;       // Idle time before starting a transition
float wind_transition_timer = 0.0f; // Timer during the transition phase
float wind_transition_duration = 0.0f; // Duration over which to smoothly transition
bool wind_in_transition = false;    // Flag indicating whether a transition is active

/* ------------------------ Utility Functions ------------------------ */

/* Return a random index into the unicode_chars array */
int random_unicode_index(void) {
    return rand() % NUM_UNICODE_CHARS;
}

/* Create a new falling column for a given column index */
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
    // Initialize vertical speed between 50 and 200 pixels/sec, and horizontal speed to 0.
    col->vy = 50.0f + (float)(rand() % 150);
    col->vx = 0.0f;
    return col;
}

/* Free a column and its resources */
void destroy_column(Column *col) {
    if (col) {
        free(col->indices);
        free(col);
    }
}

/* Initialize the pre‑rendered textures for the Unicode characters */
void init_unicode_textures(void) {
    for (int i = 0; i < NUM_UNICODE_CHARS; i++) {
        SDL_Color white = { 255, 255, 255, 255 };
        /* Use TTF_RenderUTF8_Solid to support UTF‑8 encoded strings */
        SDL_Surface *surf = TTF_RenderUTF8_Solid(font, unicode_chars[i], white);
        if (!surf) {
            printf("Failed to render unicode char '%s': %s\n", unicode_chars[i], TTF_GetError());
            continue;
        }
        unicode_textures[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }
    /* Use the first texture to determine character dimensions (assumes a monospace font) */
    if (unicode_textures[0]) {
        SDL_QueryTexture(unicode_textures[0], NULL, NULL, &char_width, &char_height);
    }
}

/* ------------------------ Column Update & Render ------------------------ */

/* Update the positions and content of all falling columns */
void update_columns(float delta) {
    size_t write_index = 0;
    int extended_margin = char_height * 50;  // Allow columns to be kept if they're within this margin.
    
    for (size_t i = 0; i < num_columns; i++) {
        Column *col = columns[i];

        // Apply gravitational acceleration.
        col->vy += GRAVITY * delta;
        if (col->vy > TERMINAL_VELOCITY) col->vy = TERMINAL_VELOCITY;

        // Compute target horizontal velocity based on current wind.
        float target_vx = tan(current_wind_angle * M_PI / 180.0f) * col->vy;
        col->vx += (target_vx - col->vx) * WIND_RESPONSE * delta;

        // Update column position based on velocities.
        col->x += col->vx * delta;
        col->y += col->vy * delta;

        col->char_update_timer += delta;
        if (col->char_update_timer > 0.1f) {
            for (int j = 0; j < col->length; j++) {
                if (rand() % 2 == 0) {
                    col->indices[j] = random_unicode_index();
                }
            }
            col->char_update_timer = 0.0f;
        }

        // Compute the falling angle and per-letter offsets.
        float fall_angle = atan2(col->vx, col->vy);
        float dx = -char_height * sin(fall_angle);
        float dy = -char_height * cos(fall_angle);

        // Determine the column's bounding box.
        float letter0_x = col->x;
        float letter_end_x = col->x + (col->length - 1) * dx;
        float min_x = (letter0_x < letter_end_x) ? letter0_x : letter_end_x;
        float max_x = (letter0_x > letter_end_x) ? letter0_x : letter_end_x;

        float letter0_y = col->y;
        float letter_end_y = col->y + (col->length - 1) * dy;
        float min_y = (letter0_y < letter_end_y) ? letter0_y : letter_end_y;
        float max_y = (letter0_y > letter_end_y) ? letter0_y : letter_end_y;

        // Keep columns if any part is visible within an extended margin.
        if (max_y >= -extended_margin && min_y <= g_screen_height + extended_margin &&
            max_x >= -extended_margin && min_x <= g_screen_width + extended_margin) {
            columns[write_index++] = col;
        } else {
            destroy_column(col);
        }
    }
    num_columns = write_index;

    // Spawn new columns over an extended horizontal range.
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

/* Render all columns onto the current render target */
void render_columns(void) {
    for (int i = 0; i < num_columns; i++) {
        Column *col = columns[i];
        
        // Precompute scale and centering offset.
        float scale = 0.5f + 0.5f * col->depth;
        int scaled_width = (int)(char_width * scale);
        int offset = (char_width - scaled_width) / 2;
        
        // Compute the falling angle from the column's velocity.
        float fall_angle = atan2(col->vx, col->vy);  // Radians relative to vertical.
        float angle_deg = -fall_angle * 180.0f / M_PI; // Rotation in degrees.
        
        // Determine displacement between successive letters along the falling trajectory.
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
                color = (SDL_Color){255, 255, 255, 255};
            } else {
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

/* ------------------------ Event Handling for Resizing ------------------------ */
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

/* ------------------------ Lightning Effect Functions ------------------------ */
SDL_Point* generate_fractal_lightning_points(int startX, int startY, int endX, int endY, float displacement, int detail, int *num_points) {
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
            
            // Calculate the effective offset range.
            float effective_range = displacement;
            if (fabs((float)(B.x - A.x)) > 0.001f && fabs(perpX) > 1e-6) {
                float max_allowed = (fabs((float)(B.x - A.x)) / 2.0f) / fabs(perpX);
                effective_range = fmin(displacement, max_allowed);
            }
            float random_offset = ((float)rand()/(float)RAND_MAX) * 2.0f * effective_range - effective_range;
            midX += perpX * random_offset;
            midY += perpY * random_offset;
            
            // Ensure vertical ordering: force midY to be strictly between A.y and B.y
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

LightningEffect* generate_lightning() {
    LightningEffect* l = malloc(sizeof(LightningEffect));
    if (!l) return NULL;
    // 20% chance to be a full screen flash (effect_type == 1)
    l->effect_type = (rand() % 2 == 0) ? 1 : 0;
    float initial_displacement = 0.0f; // Declare here for branch generation
    if (l->effect_type == 1) {
        l->timer = 0.5f;
        l->initial_timer = 0.5f;
        // For full screen flash, no bolt is needed.
        l->points = NULL;
        l->num_points = 0;
    } else {
        l->timer = 1.5f;
        l->initial_timer = 1.5f;
        // Generate a realistic fractal lightning bolt.
        int startX = rand() % g_screen_width;
        int startY = 0;
        int endX = rand() % g_screen_width;
        // End somewhere between 70% and 100% of the screen height.
        int endY = (g_screen_height * (70 + rand() % 31)) / 100;
        initial_displacement = g_screen_width / 8.0f;
        int detail = 6; // Recursion depth for fractal detail.
        l->points = generate_fractal_lightning_points(startX, startY, endX, endY, initial_displacement, detail, &l->num_points);
    }
    
    // Pre‑allocate space for branches.
    l->branches = malloc((l->num_points ? l->num_points : 1) * sizeof(LightningBranch));
    l->num_branches = 0;
    if (l->points && l->num_points > 1) {
        for (int i = 0; i < l->num_points - 1; i++) {
            if (rand() % 100 < 25) {  // 25% chance to create a branch on this segment.
                SDL_Point start = l->points[i];
                // Instead of basing the branch angle on the segment direction,
                // force the branch to point within a narrow, mostly‐vertical cone.
                // This ensures branch segments extend downward without excessive horizontal deviation.
                float branch_angle = 1.5708f - 0.7854f + ((float)rand()/(float)RAND_MAX) * (0.7854f * 2);
                // branch_angle now is in [0.7854, 2.3562] radians (approx. [45°,135°]).
                int branch_length = 50 + rand() % 51; // Length between 50 and 100 pixels.
                int branch_endX = start.x + (int)(branch_length * cosf(branch_angle));
                int branch_endY = start.y + (int)(branch_length * sinf(branch_angle));
                if (branch_endX < 0) branch_endX = 0;
                if (branch_endX >= g_screen_width) branch_endX = g_screen_width - 1;
                // Make sure the branch goes downward from its starting point.
                if (branch_endY < start.y + 1) branch_endY = start.y + 1;
                if (branch_endY >= g_screen_height) branch_endY = g_screen_height - 1;
                int branch_num_points = 0;
                SDL_Point *branch_points = generate_fractal_lightning_points(start.x, start.y, branch_endX, branch_endY, initial_displacement / 2.0f, 3, &branch_num_points);
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

void draw_filled_thick_line(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, float thickness, SDL_Color color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len == 0) return;
    float udx = dx / len;
    float udy = dy / len;
    float half_thickness = thickness / 2.0f;
    // Perpendicular vector for offset
    float px = -udy * half_thickness;
    float py = udx * half_thickness;
    
    // Define the four vertices of the quad
    SDL_Vertex vertices[4];
    vertices[0].position.x = x1 + px;
    vertices[0].position.y = y1 + py;
    vertices[1].position.x = x2 + px;
    vertices[1].position.y = y2 + py;
    vertices[2].position.x = x2 - px;
    vertices[2].position.y = y2 - py;
    vertices[3].position.x = x1 - px;
    vertices[3].position.y = y1 - py;
    
    // Set vertex colors
    for (int i = 0; i < 4; i++) {
        vertices[i].color = color;
    }
    
    // Define two triangles to fill the quad
    int indices[6] = {0, 1, 2, 0, 2, 3};
    
    SDL_RenderGeometry(renderer, NULL, vertices, 4, indices, 6);
}

void draw_glowing_lightning_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int base_thickness, SDL_Color baseColor) {
    SDL_Color glowColor = baseColor;
    // Reduce alpha for the glow layers.
    glowColor.a = (Uint8)(baseColor.a * 0.5);
    // Draw outer glow layers with slightly thicker lines.
    draw_filled_thick_line(renderer, (float)x1, (float)y1, (float)x2, (float)y2, base_thickness + 6, glowColor);
    draw_filled_thick_line(renderer, (float)x1, (float)y1, (float)x2, (float)y2, base_thickness + 4, glowColor);
    // Draw the main lightning line.
    draw_filled_thick_line(renderer, (float)x1, (float)y1, (float)x2, (float)y2, base_thickness, baseColor);
}

void draw_lightning(LightningEffect *l) {
    // Compute the fading factor (alpha).
    float alpha_factor = l->timer / l->initial_timer;
    Uint8 alpha = (Uint8)(255 * alpha_factor);
    SDL_Color white = {255, 255, 255, alpha};
    int base_thickness = 3; // Base thickness for the main lightning line.

    // Draw the main bolt with a glowing effect.
    for (int i = 0; i < l->num_points - 1; i++) {
         float t = 1.0f;
         // Taper the endpoints: the very first and last segments are drawn thinner.
         if (i == 0 || i == l->num_points - 2) {
             t = 0.5f;
         }
         int seg_thickness = (int)(base_thickness * t);
         if(seg_thickness < 1) seg_thickness = 1;
         draw_glowing_lightning_line(renderer,
                                     l->points[i].x, l->points[i].y,
                                     l->points[i+1].x, l->points[i+1].y,
                                     seg_thickness, white);
    }
    // Draw the pre‑computed branches.
    for (int i = 0; i < l->num_branches; i++) {
         LightningBranch branch = l->branches[i];
         for (int j = 0; j < branch.num_points - 1; j++) {
              float t = 1.0f;
              if (j == 0 || j == branch.num_points - 2) {
                  t = 0.5f;
              }
              int seg_thickness = (int)(base_thickness * t);
              if(seg_thickness < 1) seg_thickness = 1;
              draw_glowing_lightning_line(renderer,
                                          branch.points[j].x, branch.points[j].y,
                                          branch.points[j+1].x, branch.points[j+1].y,
                                          seg_thickness, white);
         }
    }
}

/* ------------------------ Main Loop ------------------------ */
void main_loop(void *arg) {
    handle_events();
    Uint32 current_ticks = SDL_GetTicks();
    float delta = (current_ticks - last_ticks) / 1000.0f;
    last_ticks = current_ticks;
    
    // Update global wind effect
    if (wind_in_transition) {
        wind_transition_timer += delta;
        float t = wind_transition_timer / wind_transition_duration;
        if (t >= 1.0f) {
            current_wind_angle = target_wind_angle;
            wind_in_transition = false;
            // Set a new idle period (randomized between 3 and 8 seconds)
            wind_idle_timer = 3.0f + ((float)rand()/ (float)RAND_MAX)*5.0f;
            wind_transition_timer = 0.0f;
            wind_transition_duration = 0.0f;
        } else {
            // Smooth linear interpolation between wind_start_angle and target_wind_angle
            current_wind_angle = wind_start_angle + (target_wind_angle - wind_start_angle) * t;
        }
    } else {
        wind_idle_timer -= delta;
        if (wind_idle_timer <= 0) {
            wind_in_transition = true;
            // Choose a transition duration between 1 and 5 seconds
            wind_transition_duration = 1.0f + (((float)rand() / (float)RAND_MAX) * 4.0f);
            wind_transition_timer = 0.0f;
            wind_start_angle = current_wind_angle;
            // Generate a new random target between -45° and 45°
            target_wind_angle = -45.0f + (((float)rand() / (float)RAND_MAX) * 90.0f);
        }
    }
    
    SDL_SetRenderTarget(renderer, canvas);
    /* Draw a translucent black rectangle for the fading trail effect */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);
    SDL_RenderFillRect(renderer, NULL);
    
    update_columns(delta);
    render_columns();
    
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, canvas, NULL, NULL);
    
    // Lightning effect integration.
    if (lightning) {
        lightning->timer -= delta;
        if (lightning->effect_type == 1) { 
            // Fading full-screen flash effect.
            float alpha_factor = lightning->timer / lightning->initial_timer;
            if (alpha_factor < 0) { 
                alpha_factor = 0;
            }
            Uint8 fade_alpha = (Uint8)(255 * alpha_factor);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, fade_alpha);
            SDL_RenderFillRect(renderer, NULL);
        } else {
            // Draw the bolt with glowing, thick lines.
            draw_lightning(lightning);
        }
        // Clean up once the lightning's timer runs out.
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
        // Lightning frequency remains as before (approx. 0.6% chance per frame).
        if (rand() % 1000 < 6) {
            lightning = generate_lightning();
        }
    }
    
    SDL_RenderPresent(renderer);
}

/* ------------------------ Main Function ------------------------ */
int main(int argc, char *argv[]) {
    printf("YARRRRRRR\n");
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

    window = SDL_CreateWindow("Matrix Rain Screen", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                g_screen_width, g_screen_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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

    // Initialize the dynamic array for columns.
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
        SDL_Delay(16);  // ~60 FPS
    }
#endif

    // Cleanup Unicode textures.
    for (int i = 0; i < NUM_UNICODE_CHARS; i++) {
        if (unicode_textures[i])
            SDL_DestroyTexture(unicode_textures[i]);
    }
    // Clean up and free all columns.
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
