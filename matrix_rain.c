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
    int col;             // horizontal column index (in character cells)
    float y;             // vertical offset (in pixels) for the head character
    int length;          // number of characters in the column
    float speed;         // falling speed (pixels per second)
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

/* ------------------------ Utility Functions ------------------------ */

/* Return a random index into the unicode_chars array */
int random_unicode_index(void) {
    return rand() % NUM_UNICODE_CHARS;
}

/* Create a new falling column for a given column index */
Column *create_column(int col_index) {
    Column *col = malloc(sizeof(Column));
    if (!col) return NULL;
    col->col = col_index;
    /* Start the column at a random negative offset (so it "enters" from above) */
    col->y = -(rand() % g_screen_height);
    col->length = 5 + rand() % 23;   // length between 5 and 25 characters
    col->speed  = 50 + rand() % 150;   // speed between 50 and 200 pixels/sec
    col->depth  = (float)(rand() % 101) / 100.0f;
    col->char_update_timer = 0.0f;
    col->indices = malloc(col->length * sizeof(int));
    if (!col->indices) {
        free(col);
        return NULL;
    }
    for (int i = 0; i < col->length; i++) {
        col->indices[i] = random_unicode_index();
    }
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
    for (size_t i = 0; i < num_columns; i++) {
        Column *col = columns[i];
        col->y += col->speed * delta;
        col->char_update_timer += delta;
        
        if (col->char_update_timer > 0.1f) {
            for (int j = 0; j < col->length; j++) {
                if (rand() % 2 == 0) {  // 50% chance to change a character
                    col->indices[j] = random_unicode_index();
                }
            }
            col->char_update_timer = 0.0f;
        }
        
        // Keep columns that are still visible.
        if (col->y - col->length * char_height <= g_screen_height) {
            columns[write_index++] = col;
        } else {
            destroy_column(col);
        }
    }
    num_columns = write_index;
    
    // Randomly attempt to add a new column.
    if ((rand() % 100) < 20) {
        int col_index = rand() % g_screen_width;
        Column *newcol = create_column(col_index);
        if (newcol) {
            // Check and grow dynamic array if needed.
            if (num_columns >= columns_capacity) {
                size_t new_capacity = (columns_capacity == 0) ? 16 : columns_capacity * 2;
                Column **new_columns = realloc(columns, new_capacity * sizeof(Column *));
                if (new_columns) {
                    columns = new_columns;
                    columns_capacity = new_capacity;
                } else {
                    // If memory allocation fails, destroy the new column and skip adding it.
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
        int x = col->col;
        
        // Precompute the scale and offset as these are constant for the column.
        float scale = 0.5f + 0.5f * col->depth;  // scale in [0.5, 1.0]
        int scaled_width = (int)(char_width * scale);
        int offset = (char_width - scaled_width) / 2;
        
        for (int j = 0; j < col->length; j++) {
            float posY = col->y - j * char_height;
            if (posY < -char_height || posY > g_screen_height) continue;
            
            int index = col->indices[j];
            SDL_Texture *tex = unicode_textures[index];
            if (!tex) continue;
            
            SDL_Color color;
            if (j == 0) {
                // Head character: bright white.
                color.r = 255; color.g = 255; color.b = 255; color.a = 255;
            } else {
                // Trail characters: green with brightness modulated by depth.
                int brightness = (int)(col->depth * 200) + 55;
                if (brightness > 255) brightness = 255;
                color.r = 0; color.g = brightness; color.b = 0; color.a = 255;
            }
            SDL_SetTextureColorMod(tex, color.r, color.g, color.b);

            SDL_Rect dst = { x + offset, (int)posY, scaled_width, char_height };
            SDL_RenderCopy(renderer, tex, NULL, &dst);
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
LightningEffect* generate_lightning() {
    LightningEffect* l = malloc(sizeof(LightningEffect));
    if (!l) return NULL;
    // 20% chance to be a full screen flash (effect_type == 1)
    l->effect_type = (rand() % 5 == 0) ? 1 : 0;
    if (l->effect_type == 1) {
        l->timer = 0.5f;
        l->initial_timer = 0.5f;
    } else {
        l->timer = 1.5f;
        l->initial_timer = 1.5f;
    }
    
    int capacity = 64;
    SDL_Point* points = malloc(capacity * sizeof(SDL_Point));
    if (!points) { 
        free(l);
        return NULL;
    }
    int count = 0;
    int startX = rand() % g_screen_width;
    int startY = 0;
    points[count].x = startX;
    points[count].y = startY;
    count++;
    
    int currentX = startX;
    int currentY = startY;
    while (currentY < g_screen_height) {
         int stepY = 10 + rand() % 16; // vertical step between 10 and 25 pixels
         currentY += stepY;
         int offsetX = -15 + rand() % 31; // horizontal offset between -15 and 15
         currentX += offsetX;
         if (currentX < 0) currentX = 0;
         if (currentX >= g_screen_width) currentX = g_screen_width - 1;
         if (count >= capacity) {
             capacity *= 2;
             SDL_Point* new_points = realloc(points, capacity * sizeof(SDL_Point));
             if (!new_points) break;
             points = new_points;
         }
         points[count].x = currentX;
         points[count].y = currentY;
         count++;
         if (currentY >= g_screen_height - 20) break;
    }
    l->points = points;
    l->num_points = count;
    
    // Generate pre‑computed branches.
    l->branches = malloc(l->num_points * sizeof(LightningBranch));
    l->num_branches = 0;
    for (int i = 0; i < l->num_points - 1; i++) {
         if (rand() % 100 < 40) {  // 40% chance to create a branch at this segment.
             int branch_length = 2 + rand() % 4; // branch length from 2 to 5 segments.
             SDL_Point* branch_points = malloc((branch_length + 1) * sizeof(SDL_Point));
             if (!branch_points) continue;
             branch_points[0] = l->points[i];
             int bx = branch_points[0].x;
             int by = branch_points[0].y;
             int bcount = 1;
             for (int j = 0; j < branch_length; j++) {
                 int bx_new = bx + (-15 + rand() % 31);
                 int by_new = by + 15 + rand() % 16;
                 branch_points[bcount].x = bx_new;
                 branch_points[bcount].y = by_new;
                 bx = bx_new;
                 by = by_new;
                 bcount++;
             }
             l->branches[l->num_branches].points = branch_points;
             l->branches[l->num_branches].num_points = bcount;
             l->num_branches++;
         }
    }
    
    return l;
}

void draw_thick_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int thickness, SDL_Color color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len == 0) return;
    float udx = dx / len;
    float udy = dy / len;
    // Perpendicular vector for offset direction.
    float px = -udy;
    float py = udx;
    
    int half = thickness / 2;
    for (int offset = -half; offset <= half; offset++) {
        int ox1 = x1 + (int)(px * offset);
        int oy1 = y1 + (int)(py * offset);
        int ox2 = x2 + (int)(px * offset);
        int oy2 = y2 + (int)(py * offset);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer, ox1, oy1, ox2, oy2);
    }
}

void draw_glowing_lightning_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int base_thickness, SDL_Color baseColor) {
    SDL_Color glowColor = baseColor;
    // Reduce alpha for the glow layers.
    glowColor.a = (Uint8)(baseColor.a * 0.5);
    // Draw outer glow layers.
    draw_thick_line(renderer, x1, y1, x2, y2, base_thickness + 6, glowColor);
    draw_thick_line(renderer, x1, y1, x2, y2, base_thickness + 4, glowColor);
    // Draw the main lightning line.
    draw_thick_line(renderer, x1, y1, x2, y2, base_thickness, baseColor);
}

void draw_lightning(LightningEffect *l) {
    // Compute the fading factor (alpha).
    float alpha_factor = l->timer / l->initial_timer;
    Uint8 alpha = (Uint8)(255 * alpha_factor);
    SDL_Color white = {255, 255, 255, alpha};
    int base_thickness = 3; // Base thickness for the main lightning line.
    
    // Draw the main bolt with the glowing effect.
    for (int i = 0; i < l->num_points - 1; i++) {
        draw_glowing_lightning_line(renderer, l->points[i].x, l->points[i].y,
                                             l->points[i+1].x, l->points[i+1].y,
                                             base_thickness, white);
    }
    // Draw the pre‑computed branches.
    for (int i = 0; i < l->num_branches; i++) {
         LightningBranch branch = l->branches[i];
         for (int j = 0; j < branch.num_points - 1; j++) {
              draw_glowing_lightning_line(renderer, branch.points[j].x, branch.points[j].y,
                                                 branch.points[j+1].x, branch.points[j+1].y,
                                                 base_thickness, white);
         }
    }
}

/* ------------------------ Main Loop ------------------------ */
void main_loop(void *arg) {
    handle_events();
    Uint32 current_ticks = SDL_GetTicks();
    float delta = (current_ticks - last_ticks) / 1000.0f;
    last_ticks = current_ticks;
    
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
            // For a full screen flash, fill the screen with white.
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
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
