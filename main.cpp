#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "./SDL2-2.0.10/include/SDL.h"
#include "./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600

#define GAME_HEIGHT 550

#define CUBE_SIZE 20
#define MAX_SNAKE_LENGTH 50
#define HISTORY_SIZE (MAX_SNAKE_LENGTH*10)
#define SNAKE_SPEED 200.0 //200.0 min for full functionality
#define MAX_SNAKE_SPEED 400.0
#define SNAKE_SPEED_UP 0.1

#define NEW_GAME_KEY 'n'
#define END_GAME_KEY SDLK_ESCAPE

struct SDLStruct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Surface* screen;
    SDL_Texture* scrtex;
    SDL_Surface* charset;
    SDL_Surface* body;
};

struct GameParameters {
    bool quit;
    double bodyX[MAX_SNAKE_LENGTH];
    double bodyY[MAX_SNAKE_LENGTH];
    double historyX[HISTORY_SIZE];
    double historyY[HISTORY_SIZE];
    int length;
    double velocityX;
    double velocityY;
};

struct GameTime {
    int frames;
    double fpsTimer;
    double fps;
    double worldTime;
};


// Funkcja rysująca napis na powierzchni
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
    int px, py, c;
    SDL_Rect s, d;
    s.w = 8;
    s.h = 8;
    d.w = 8;
    d.h = 8;
    while (*text) {
        c = *text & 255;
        px = (c % 16) * 8;
        py = (c / 16) * 8;
        s.x = px;
        s.y = py;
        d.x = x;
        d.y = y;
        SDL_BlitSurface(charset, &s, screen, &d);
        x += 8;
        text++;
    };
}

// Funkcja rysująca powierzchnię (sprite) na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
    SDL_Rect dest;
    dest.x = x - sprite->w / 2;
    dest.y = y - sprite->h / 2;
    dest.w = sprite->w;
    dest.h = sprite->h;
    SDL_BlitSurface(sprite, NULL, screen, &dest);
}

// Funkcja rysująca pojedynczy piksel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    *(Uint32*)p = color;
}

// Funkcja rysująca linię
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
    for (int i = 0; i < l; i++) {
        DrawPixel(screen, x, y, color);
        x += dx;
        y += dy;
    };
}

// Funkcja rysująca prostokąt
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
    int i;
    DrawLine(screen, x, y, k, 0, 1, outlineColor);
    DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
    DrawLine(screen, x, y, l, 1, 0, outlineColor);
    DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
    for (i = y + 1; i < y + k - 1; i++)
        DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
}

#ifdef __cplusplus
extern "C"
#endif

bool InitSDL(SDLStruct& sdl) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdl.window, &sdl.renderer);
    if (rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(sdl.renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(sdl.window, "Snake Movement Example");

    sdl.screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    sdl.scrtex = SDL_CreateTexture(sdl.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_ShowCursor(SDL_DISABLE);

    sdl.charset = SDL_LoadBMP("./cs8x8.bmp");
    if (sdl.charset == NULL) {
        printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(sdl.screen);
        SDL_DestroyTexture(sdl.scrtex);
        SDL_DestroyWindow(sdl.window);
        SDL_DestroyRenderer(sdl.renderer);
        SDL_Quit();
        return 1;
    }
    SDL_SetColorKey(sdl.charset, true, 0x000000);

    sdl.body = SDL_LoadBMP("./body.bmp");
    if (sdl.body == NULL) {
        printf("SDL_LoadBMP(body.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(sdl.charset);
        SDL_FreeSurface(sdl.screen);
        SDL_DestroyTexture(sdl.scrtex);
        SDL_DestroyWindow(sdl.window);
        SDL_DestroyRenderer(sdl.renderer);
        SDL_Quit();
        return 1;
    }
    return 0;
}

void CleanSDL(SDLStruct& sdl) {
    SDL_FreeSurface(sdl.charset);
    SDL_FreeSurface(sdl.screen);
    SDL_FreeSurface(sdl.body);
    SDL_DestroyTexture(sdl.scrtex);
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_Quit();
}

void InitGame(GameParameters& game) {
    game.quit = false;
    game.length = 20;
    game.velocityX = 0;
    game.velocityY = 0;

    game.bodyX[0] = SCREEN_WIDTH / 2;
    game.bodyY[0] = SCREEN_HEIGHT / 2;

    for (int i = 1; i < game.length; i++) {
        game.bodyX[i] = game.bodyX[i - 1] + 20;
        game.bodyY[i] = game.bodyY[i - 1];
    }

    for (int i = 0; i < HISTORY_SIZE; i++) {
        game.historyX[i] = game.bodyX[0];
        game.historyY[i] = game.bodyY[0];
    }
}


float GetSnakeSpeed(GameTime t) {
    float s;
    s = (t.worldTime * SNAKE_SPEED_UP * SNAKE_SPEED + SNAKE_SPEED);
    if (s < MAX_SNAKE_SPEED) return s;
    else return MAX_SNAKE_SPEED;
}


bool UserInput(GameParameters& game, SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == END_GAME_KEY) game.quit = true;
        else if (event.key.keysym.sym == NEW_GAME_KEY) return false;
        else if (event.key.keysym.sym == SDLK_UP && game.velocityY == 0) {
            game.velocityX = 0;
            game.velocityY = -1; 
        }
        else if (event.key.keysym.sym == SDLK_DOWN && game.velocityY == 0) {
            game.velocityX = 0;
            game.velocityY = 1; 
        }
        else if (event.key.keysym.sym == SDLK_LEFT && game.velocityX == 0) {
            game.velocityX = -1;
            game.velocityY = 0; 
        }
        else if (event.key.keysym.sym == SDLK_RIGHT && game.velocityX == 0) {
            game.velocityX = 1;
            game.velocityY = 0;
        }
    }
    return true;
}


void MoveSnake(GameParameters& game, GameTime& time, double delta) {

    float currentSpeed = GetSnakeSpeed(time);

    // history update(deleting useless elements)
    for (int i = HISTORY_SIZE - 1; i > 0; i--) {
        game.historyX[i] = game.historyX[i - 1];
        game.historyY[i] = game.historyY[i - 1];
    }
    // writting head to history
    game.historyX[0] = game.bodyX[0];
    game.historyY[0] = game.bodyY[0];

    // head movement
    game.bodyX[0] += game.velocityX * currentSpeed * delta;
    game.bodyY[0] += game.velocityY * currentSpeed * delta;

    // going right when reaching boarders
    if (game.bodyX[0] <= CUBE_SIZE && game.bodyY[0] <= CUBE_SIZE) { game.velocityX = 1; game.velocityY = 0; }
    else if (game.bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE && game.bodyY[0] <= CUBE_SIZE) { game.velocityX = 0; game.velocityY = 1; }
    else if (game.bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE && game.bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { game.velocityX = -1; game.velocityY = 0; }
    else if (game.bodyX[0] <= CUBE_SIZE && game.bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { game.velocityX = 0; game.velocityY = -1; }
    else if (game.bodyX[0] <= CUBE_SIZE) { game.velocityX = 0; game.velocityY = -1; }
    else if (game.bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE) { game.velocityX = 0; game.velocityY = 1; }
    else if (game.bodyY[0] <= CUBE_SIZE) { game.velocityX = 1;  game.velocityY = 0; }
    else if (game.bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { game.velocityX = -1; game.velocityY = 0; }

    // going to the other side when reaching boarders
    //if (bodyX[0] < 0) bodyX[0] = SCREEN_WIDTH;
    //else if (bodyX[0] > SCREEN_WIDTH) bodyX[0] = 0;
    //if (bodyY[0] < 0) bodyY[0] = GAME_HEIGHT;
    //else if (bodyY[0] > GAME_HEIGHT) bodyY[0] = 0;
    // other parts movement
    for (int i = 1; i < game.length; i++) {
        float s = (time.worldTime * SNAKE_SPEED_UP + 1);
        if (s > MAX_SNAKE_SPEED/SNAKE_SPEED) s= (MAX_SNAKE_SPEED / SNAKE_SPEED);
        int historyIndex = i * (HISTORY_SIZE / game.length)/s;
        game.bodyX[i] = game.historyX[historyIndex];
        game.bodyY[i] = game.historyY[historyIndex];
    }
}

void Draw(SDLStruct& sdl, GameParameters& game, GameTime& time) {
    char text[128];

    SDL_FillRect(sdl.screen, NULL, SDL_MapRGB(sdl.screen->format, 0x00, 0x00, 0x00));

    // Snake
    for (int i = 0; i < game.length; i++) {
        DrawSurface(sdl.screen, sdl.body, (int)game.bodyX[i], (int)game.bodyY[i]);
    }

    // Updates 
    DrawRectangle(sdl.screen, 4, GAME_HEIGHT + 4, SCREEN_WIDTH - 8, 36, SDL_MapRGB(sdl.screen->format, 0xFF, 0x00, 0x00), SDL_MapRGB(sdl.screen->format, 0x11, 0x11, 0xCC));
    float s = (time.worldTime * SNAKE_SPEED_UP + 1);
    if (s > MAX_SNAKE_SPEED/SNAKE_SPEED) s = MAX_SNAKE_SPEED/SNAKE_SPEED;
    sprintf(text, "Elapsed time = %.1lfs  %.0lfFPS  Speed: %.1lfx", time.worldTime, time.fps, s);
    DrawString(sdl.screen, sdl.screen->w / 2 - strlen(text) * 8 / 2, GAME_HEIGHT + 10, text, sdl.charset);
    sprintf(text, "Esc - exit, N - new game, Arrow keys - move");
    DrawString(sdl.screen, sdl.screen->w / 2 - strlen(text) * 8 / 2, GAME_HEIGHT + 26, text, sdl.charset);

    SDL_UpdateTexture(sdl.scrtex, NULL, sdl.screen->pixels, sdl.screen->pitch);
    SDL_RenderCopy(sdl.renderer, sdl.scrtex, NULL, NULL);
    SDL_RenderPresent(sdl.renderer);
}

bool Collision(const GameParameters& game) {
    for (int i = 4; i < game.length; i++) {
        if (fabs(game.bodyX[0] - game.bodyX[i]) <= CUBE_SIZE / 2 && fabs(game.bodyY[0] - game.bodyY[i]) <= CUBE_SIZE / 2) {
            return true;
        }
    }
    return false;
}


void GameOver(SDLStruct& sdl, GameParameters& game, GameTime& time) {
    char text[128];
    SDL_FillRect(sdl.screen, NULL, SDL_MapRGB(sdl.screen->format, 0x00, 0x00, 0x00));
    sprintf(text, "Game Over! Press ESC to quit or N to start a new game");
    DrawString(sdl.screen, SCREEN_WIDTH / 2 - strlen(text) * 8 / 2, GAME_HEIGHT / 2, text, sdl.charset);

    SDL_UpdateTexture(sdl.scrtex, NULL, sdl.screen->pixels, sdl.screen->pitch);
    SDL_RenderCopy(sdl.renderer, sdl.scrtex, NULL, NULL);
    SDL_RenderPresent(sdl.renderer);

    bool gameOver = true;
    while (gameOver) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == END_GAME_KEY) {
                    game.quit = true;
                    gameOver = false;
                }
                else if (event.key.keysym.sym == NEW_GAME_KEY) {
                    gameOver = false;
                    InitGame(game);
                    time = { 0, 0, 0, 0 };
                }
            }
        }
    }
}


int main(int argc, char** argv) {
    SDLStruct sdl;
    GameParameters game;
    GameTime time = { 0, 0, 0, 0 };

    if (InitSDL(sdl)) return 1;
    InitGame(game);

    int t1 = SDL_GetTicks();

    while (!game.quit) {
        int t2 = SDL_GetTicks();
        double delta = (t2 - t1) * 0.001;
        t1 = t2;

        time.worldTime += delta;
        time.fpsTimer += delta;
        time.frames++;
        if (time.fpsTimer > 0.5) {
            time.fps = time.frames * 2;
            time.frames = 0;
            time.fpsTimer -= 0.5;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (!UserInput(game, event)) {
                CleanSDL(sdl);
                time = { 0, 0, 0, 0 };
                if (InitSDL(sdl)) return 1;
                InitGame(game);
                int t1 = SDL_GetTicks();
                double delta = (t2 - t1) * 0.001;
                t1 = t2;

                time.worldTime += delta;
                time.fpsTimer += delta;
                time.frames++;

                if (time.fpsTimer > 0.5) {
                    time.fps = time.frames * 2;
                    time.frames = 0;
                    time.fpsTimer -= 0.5;
                }
            }
        }
        MoveSnake(game, time, delta);
        Draw(sdl, game, time);
        if (Collision(game) && game.bodyX[19] != SCREEN_WIDTH / 2) GameOver(sdl, game, time);
    }

    CleanSDL(sdl);
    return 0;
}
