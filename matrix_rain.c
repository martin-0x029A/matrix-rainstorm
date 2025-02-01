#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* ------------------------ Configuration ------------------------ */

#define FONT_SIZE     16
#define MAX_COLUMNS   100

/* Global (dynamic) window size variables */
int g_screen_width = 800;
int g_screen_height = 600;

/* ------------------------ Unicode Characters Data Structure ------------------------ */

/*
 * Define an array of Unicode characters (UTF‑8 encoded) that you want to appear.
 * You can add any Unicode strings here (e.g. Japanese, Chinese, emoji, symbols, etc.).
 */
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
    "∝", "∴", "∵", "∃", "∀", "∩", "∪", "∼", "≈", "≅",
    
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

/* ------------------------ Global Variables ------------------------ */

/* Active columns (stored as pointers) */
Column *columns[MAX_COLUMNS];
int num_columns = 0;

/* SDL objects */
SDL_Window   *window   = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font     *font     = NULL;
SDL_Texture  *canvas   = NULL;  // offscreen render target for the trail effect

int char_width, char_height;     // dimensions of a character (assumes monospace)
Uint32 last_ticks = 0;

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
    for (int i = 0; i < num_columns; i++) {
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
    }
    /* Remove any column that has moved off the bottom of the screen */
    for (int i = 0; i < num_columns; ) {
        Column *col = columns[i];
        if (col->y - col->length * char_height > g_screen_height) {
            destroy_column(col);
            for (int j = i; j < num_columns - 1; j++) {
                columns[j] = columns[j+1];
            }
            num_columns--;
        } else {
            i++;
        }
    }
    /* Occasionally spawn a new column (ensuring one per horizontal cell) */
    if (num_columns < MAX_COLUMNS && (rand() % 100) < 50) {
        int max_col = g_screen_width / char_width;
        int col_index = rand() % max_col;
        bool exists = false;
        for (int i = 0; i < num_columns; i++) {
            if (columns[i]->col == col_index) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            Column *newcol = create_column(col_index);
            if (newcol) {
                columns[num_columns++] = newcol;
            }
        }
    }
}

/* Render all columns onto the current render target */
void render_columns(void) {
    for (int i = 0; i < num_columns; i++) {
        Column *col = columns[i];
        int x = col->col * char_width;
        for (int j = 0; j < col->length; j++) {
            float posY = col->y - j * char_height;
            if (posY < -char_height || posY > g_screen_height) continue;
            int index = col->indices[j];
            SDL_Texture *tex = unicode_textures[index];
            if (!tex) continue;
            SDL_Color color;
            if (j == 0) {
                /* Head character: bright white */
                color.r = 255;
                color.g = 255;
                color.b = 255;
                color.a = 255;
            } else {
                /* Trail characters: green with brightness modulated by depth */
                int brightness = (int)(col->depth * 200) + 55;
                if (brightness > 255) brightness = 255;
                color.r = 0;
                color.g = brightness;
                color.b = 0;
                color.a = 255;
            }
            SDL_SetTextureColorMod(tex, color.r, color.g, color.b);

            /* Calculate a dynamic width based on depth.
             *
             * Here, we use a simple scale factor such that when the column is "deeper"
             * (i.e. has a lower col->depth), the character is rendered narrower.
             * You can adjust the min_scale (here 0.5) as needed.
             */
            float scale = 0.5f + 0.5f * col->depth;  // scale in [0.5, 1.0]
            int scaled_width = (int)(char_width * scale);
            int offset = (char_width - scaled_width) / 2; // Center within the character cell

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
    /* Load a monospace font. Ensure "DejaVuSansMono.ttf" (or another appropriate TTF) is available. */
    font = TTF_OpenFont("NotoSansMonoCJK-VF.ttf.ttc", FONT_SIZE);
    if (!font) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    /* Initialize Unicode textures */
    init_unicode_textures();
    
    /* Create an offscreen canvas texture for the trail effect */
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

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop, NULL, 0, 1);
#else
    while (1) {
        main_loop(NULL);
        SDL_Delay(16);  // ~60 FPS
    }
#endif

    /* Cleanup Unicode textures */
    for (int i = 0; i < NUM_UNICODE_CHARS; i++) {
        if (unicode_textures[i])
            SDL_DestroyTexture(unicode_textures[i]);
    }
    for (int i = 0; i < num_columns; i++) {
        destroy_column(columns[i]);
    }
    if (canvas) SDL_DestroyTexture(canvas);
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
