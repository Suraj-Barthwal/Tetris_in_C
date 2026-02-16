#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// Constants
#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 650
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_SIZE 30

// File to store high score
const char* HIGHSCORE_FILE = "highscore.txt";
// Tetromino shapes (4x4 grids)
// Each shape is represented as a 4x4 array where 1 = block, 0 = empty
const int SHAPES[7][4][4] = {
    // I piece
    {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
    // O piece
    {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
    // T piece
    {{0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0}},
    // S piece
    {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
    // Z piece
    {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
    // J piece
    {{0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0}},
    // L piece
    {{0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0}}
};

// Colors for each piece (RGB)
const SDL_Color COLORS[7] = {
    {0, 255, 255, 255},   // Cyan (I)
    {255, 255, 0, 255},   // Yellow (O)
    {128, 0, 128, 255},   // Purple (T)
    {0, 255, 0, 255},     // Green (S)
    {255, 0, 0, 255},     // Red (Z)
    {0, 0, 255, 255},     // Blue (J)
    {255, 165, 0, 255}    // Orange (L)
};

// Structure for the current piece
typedef struct {
    int shape[4][4];
    int x, y;        // Position on board
    int type;        // Index in SHAPES array
} Piece;

// Structure for game state
typedef struct {
    int board[BOARD_HEIGHT][BOARD_WIDTH];
    Piece current;
    int score;
    int high_score; // We'll use this in the next step
    bool game_over;
    bool game_started; // <-- ADD THIS
    Uint32 last_fall_time;
    int fall_speed;
} GameState;

// Global font variables
TTF_Font* gFontTitle = NULL;
TTF_Font* gFontBody = NULL;
// Global sound variables 

Mix_Chunk *gClearSound = NULL;
Mix_Music *gMusic = NULL;

// Function prototypes
void init_piece(Piece *p, int type);
bool check_collision(GameState *game, Piece *piece, int dx, int dy);
void merge_piece(GameState *game);
int clear_lines(GameState *game);
bool rotate_piece(GameState *game);
void draw_game(SDL_Renderer *renderer, GameState *game);
void reset_game(GameState *game);
int load_highscore();
void save_highscore(int score);

// Initialize a new piece
void init_piece(Piece *p, int type) {
    p->type = type;
    p->x = BOARD_WIDTH / 2 - 2;  // Center horizontally
    p->y = 0;
    
    // Copy shape from SHAPES array
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            p->shape[i][j] = SHAPES[type][i][j];
        }
    }
}

// Check if piece collides with board boundaries or other blocks
bool check_collision(GameState *game, Piece *piece, int dx, int dy) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (piece->shape[i][j]) {
                int new_x = piece->x + j + dx;
                int new_y = piece->y + i + dy;
                
                // Check horizontal boundaries
                if (new_x < 0 || new_x >= BOARD_WIDTH) {
                    return true;
                }
                // Check bottom boundary
                if (new_y >= BOARD_HEIGHT) {
                    return true;
                }
                
                // Check collision with existing blocks (ignore if above board)
                if (new_y >= 0 && game->board[new_y][new_x] != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Merge current piece into the board
void merge_piece(GameState *game) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (game->current.shape[i][j]) {
                int x = game->current.x + j;
                int y = game->current.y + i;
                if (y >= 0 && y < BOARD_HEIGHT && x >= 0 && x < BOARD_WIDTH) {
                    game->board[y][x] = game->current.type + 1;  // Store color type
                }
            }
        }
    }
}

// Clear completed lines and update score
int clear_lines(GameState *game) {
    int lines_cleared = 0;
    
    // Check each row from bottom to top
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        bool full = true;
        
        // Check if row is full
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (game->board[i][j] == 0) {
                full = false;
                break;
            }
        }
        
        // If row is full, clear it and shift rows down
        if (full) {
            lines_cleared++;
            
            // Shift all rows above down by one
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < BOARD_WIDTH; j++) {
                    game->board[k][j] = game->board[k-1][j];
                }
            }
            
            // Clear top row
            for (int j = 0; j < BOARD_WIDTH; j++) {
                game->board[0][j] = 0;
            }
            
            i++;  // Check same row again since we shifted
        }
    }
    
    game->score += lines_cleared * 100;
    return lines_cleared;
}

// Rotate piece clockwise (90 degrees) with a simple wall-kick
bool rotate_piece(GameState *game) {
    Piece rotated = game->current;
    
    // Rotate the shape matrix 90 degrees clockwise
    // new[i][j] = old[3-j][i]
    int temp[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i][j] = rotated.shape[3 - j][i];
        }
    }
    
    // Copy rotated shape
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            rotated.shape[i][j] = temp[i][j];
        }
    }
    
    // Try rotation at current position; if collision, try small kicks
    const int kicks[] = {0, -1, 1, -2, 2}; // simple kicks: center, left, right, larger left/right
    for (size_t k = 0; k < sizeof(kicks)/sizeof(kicks[0]); k++) {
        if (!check_collision(game, &rotated, kicks[k], 0)) {
            rotated.x += kicks[k];
            game->current = rotated;
            return true;
        }
    }
    
    return false;
}

// Draw the game board and current piece
void draw_game(SDL_Renderer *renderer, GameState *game) {
    // Clear screen with dark background
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);
    
    // Draw board blocks
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (game->board[i][j] != 0) {
                SDL_Color color = COLORS[game->board[i][j] - 1];
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
                
                SDL_Rect rect = {j * BLOCK_SIZE + 50, i * BLOCK_SIZE + 20, 
                                 BLOCK_SIZE - 2, BLOCK_SIZE - 2};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    
    // Draw current piece
    SDL_Color piece_color = COLORS[game->current.type];
    SDL_SetRenderDrawColor(renderer, piece_color.r, piece_color.g, piece_color.b, 255);
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (game->current.shape[i][j]) {
                int x = (game->current.x + j) * BLOCK_SIZE + 50;
                int y = (game->current.y + i) * BLOCK_SIZE + 20;
                
                SDL_Rect rect = {x, y, BLOCK_SIZE - 2, BLOCK_SIZE - 2};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    
    // Draw grid lines
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    for (int i = 0; i <= BOARD_HEIGHT; i++) {
        SDL_RenderDrawLine(renderer, 50, i * BLOCK_SIZE + 20, 
                          BOARD_WIDTH * BLOCK_SIZE + 50, i * BLOCK_SIZE + 20);
    }
    for (int j = 0; j <= BOARD_WIDTH; j++) {
        SDL_RenderDrawLine(renderer, j * BLOCK_SIZE + 50, 20, 
                          j * BLOCK_SIZE + 50, BOARD_HEIGHT * BLOCK_SIZE + 20);
    }
    
    SDL_RenderPresent(renderer);
}

// Saves the high score to a file
void save_highscore(int score) {
    FILE* file = fopen(HIGHSCORE_FILE, "w");
    if (file == NULL) {
        printf("Warning: Could not save high score file.\n");
        return;
    }
    fprintf(file, "%d", score);
    fclose(file);
}

// Loads the high score from a file
int load_highscore() {
    FILE* file = fopen(HIGHSCORE_FILE, "r");
    if (file == NULL) {
        return 0; // No file, so high score is 0
    }
    int score = 0;
    fscanf(file, "%d", &score);
    fclose(file);
    return score;
}

// Reset game to initial state
void reset_game(GameState *game) {
    // ... clear board ...
    
    int current_high_score = game->high_score; // <-- Store the high score
    
    // Clear the whole struct (this is a good practice)
    memset(game, 0, sizeof(GameState)); 
    
    // Now restore/set values
    game->score = 0;
    game->high_score = current_high_score; // <-- Restore the high score
    game->game_over = false;
    game->game_started = true; // reset_game now means we are playing
    game->fall_speed = 500;
    game->last_fall_time = SDL_GetTicks();
    
    init_piece(&game->current, rand() % 7);
}

// Draw centered text at (x, y)
void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, 
               int x, int y, SDL_Color color) {
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (surface == NULL) {
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
    }

    // Get texture dimensions
    SDL_Rect destRect = { x, y, surface->w, surface->h };
    SDL_FreeSurface(surface); // We don't need the surface anymore

    // Center the text
    destRect.x = x - (destRect.w / 2);
    destRect.y = y - (destRect.h / 2);

    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    SDL_DestroyTexture(texture); // Clean up
}

int main(int argc, char *argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() == -1) {
        printf("SDL_ttf initialization failed: %s\n", TTF_GetError());
        return 1;
    }

    // Initialize SDL_mixer
    // MIX_INIT_MP3 and MIX_INIT_OGG are flags for formats
    if (Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG) < 0) {
        printf("SDL_mixer initialization failed: %s\n", Mix_GetError());
        return 1;
    }
    
    // Open the audio device
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer OpenAudio failed: %s\n", Mix_GetError());
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow("Tetris - PBL Project",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Load fonts
    gFontTitle = TTF_OpenFont("impact.ttf", 64);
    gFontBody = TTF_OpenFont("impact.ttf", 24); 
    if (gFontTitle == NULL || gFontBody == NULL) {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return 1;
    }

    // Load sounds
    gClearSound = Mix_LoadWAV("clear.wav");
    gMusic = Mix_LoadMUS("music.mp3");

    if (!gFontTitle || !gFontBody || !gClearSound || !gMusic) {
        printf("Failed to load audio files! SDL_mixer Error: %s\n", Mix_GetError());
        // This is not fatal, so we just print a warning
    }

    // Initialize random seed
    srand((unsigned)time(NULL));
    
    // Initialize game state
    GameState game;
    game.game_started = false; // Start on the start screen
    game.game_over = false;    // Make sure this is false too
    game.high_score = load_highscore();
    
    // Game loop
    bool running = true;
    SDL_Event event;
    
    while (running) {
        Uint32 current_time = SDL_GetTicks();
        
        // --- EVENT HANDLING ---
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            
            if (event.type == SDL_KEYDOWN) {
                // Check game state for input
                if (!game.game_started) {
                    // --- START SCREEN INPUT ---
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        reset_game(&game); // Now we start the game!
                        game.game_started = true;
                        Mix_PlayMusic(gMusic, -1);
                    }
                } else if (game.game_over) {
                    // --- GAME OVER INPUT ---
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        reset_game(&game); // This resets score, etc.
                    }
                } else {
                    // --- PLAYING INPUT ---
                    switch (event.key.keysym.sym) {
                        case SDLK_LEFT:
                            if (!check_collision(&game, &game.current, -1, 0)) {
                                game.current.x--;
                            }
                            break;
                            
                        case SDLK_RIGHT:
                            if (!check_collision(&game, &game.current, 1, 0)) {
                                game.current.x++;
                            }
                            break;
                            
                        case SDLK_DOWN:
                            if (!check_collision(&game, &game.current, 0, 1)) {
                                game.current.y++;
                                game.score += 1; // Bonus points
                            }
                            break;
                            
                        case SDLK_UP:
                            rotate_piece(&game);
                            break;
                    }
                }
                
                // --- UNIVERSAL INPUT ---
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }
        
        // --- GAME LOGIC ---
        if (game.game_started && !game.game_over) {
            // Automatic piece falling
            if (current_time - game.last_fall_time > game.fall_speed) {
                if (!check_collision(&game, &game.current, 0, 1)) {
                    game.current.y++;
                } else {
                    // Piece has landed
                    merge_piece(&game);
                    int lines = clear_lines(&game);
                    if (lines > 0) {
                    Mix_PlayChannel(-1, gClearSound, 0); 
                    }   
                    
                    // Create new piece
                    init_piece(&game.current, rand() % 7);
                    
                    // Check for game over (new piece immediately collides)
                    if (check_collision(&game, &game.current, 0, 0)) {
                        game.game_over = true;
                        printf("Game Over! Final Score: %d\n", game.score);
                        
                        // --- HIGH SCORE LOGIC ---
                        if (game.score > game.high_score) {
                            game.high_score = game.score;
                            save_highscore(game.high_score);
                            printf("New High Score: %d\n", game.high_score);
                        }
                    }
                }
                
                game.last_fall_time = current_time;
            }
        }
        
        // --- RENDER ---
        if (!game.game_started) {
            // Draw a simple start screen
            SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
            SDL_RenderClear(renderer);

            SDL_Color white = {255, 255, 255, 255};
            SDL_Color purple = {128, 0, 128, 255}; // T-piece color

            // Draw "TETRIS" (centered)
            draw_text(renderer, gFontTitle, "TETRIS", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 3, purple);

            // Draw "Press SPACE to Start" (centered)
            draw_text(renderer, gFontBody, "Press SPACE to Start", WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, white);

            SDL_SetWindowTitle(window, "Tetris - Press SPACE to Start");
            SDL_RenderPresent(renderer);
        } else {
            // Regular game/game-over drawing
            draw_game(renderer, &game);
            
            // Update window title
            char title[100];
            if (game.game_over) {
                snprintf(title, sizeof(title), "GAME OVER - Score: %d | High: %d (SPACE to restart)", 
                         game.score, game.high_score);
            } else {
                snprintf(title, sizeof(title), "Tetris - Score: %d | High Score: %d", 
                         game.score, game.high_score);
            }
            SDL_SetWindowTitle(window, title);
        }
        
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    TTF_CloseFont(gFontTitle);
    TTF_CloseFont(gFontBody);
    gFontTitle = NULL;
    gFontBody = NULL;
    TTF_Quit(); // Shut down SDL_ttf
    Mix_FreeChunk(gClearSound);
    Mix_FreeMusic(gMusic);
    gClearSound = NULL;
    gMusic = NULL;
    Mix_CloseAudio();
    Mix_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
