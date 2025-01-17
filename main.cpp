#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>


extern "C" {
#include "./SDL2-2.0.10/include/SDL.h"
#include "./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600
#define GAME_HEIGHT 525

#define CUBE_SIZE 20
#define SNAKE_LENGTH 20
#define MAX_SNAKE_LENGTH 50
#define HISTORY_SIZE (MAX_SNAKE_LENGTH*10)
#define SNAKE_SPEED 200.0 //200.0 min for full functionality, beginig speed
#define MAX_SNAKE_SPEED 600.0
#define SNAKE_SPEED_UP 0.05
#define SNAKE_SPEED_DOWN 5 //in seconds
#define DOT_RADIUS 10

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
};

struct Snake {
    double bodyX[MAX_SNAKE_LENGTH];
    double bodyY[MAX_SNAKE_LENGTH];
    double historyX[HISTORY_SIZE];
    double historyY[HISTORY_SIZE];
    int length;
    float speed;
    double velocityX;
    double velocityY;
    int eaten;
};

struct Dot {
    int x;
    int y;
    Uint32 color;
    double spawnTime;
    double duration;
    bool visible;
};

struct GameTime {
    int frames;
    double fpsTimer;
    double fps;
    double worldTime;
    double snakeTime;
    double snakeLimitTime;
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

void DrawDot(SDL_Surface* surface, int cx, int cy, Uint32 color) {
    if (cx != NULL && cy != NULL) {
        for (int y = -DOT_RADIUS; y <= DOT_RADIUS; y++) {
            for (int x = -DOT_RADIUS; x <= DOT_RADIUS; x++) {
                if (x * x + y * y <= DOT_RADIUS * DOT_RADIUS) { // Check if the point is inside the circle
                    DrawPixel(surface, cx + x, cy + y, color);
                }
            }
        }
    }
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

void InitGame(bool* quit, Snake& s, Dot& b, Dot& r, SDLStruct sdl) {
    *quit = false;
    s.length = SNAKE_LENGTH;
    s.velocityX = 0;
    s.velocityY = 0;
    s.eaten = 0;
    s.speed = SNAKE_SPEED;

    s.bodyX[0] = SCREEN_WIDTH / 2;
    s.bodyY[0] = SCREEN_HEIGHT / 2;

    b.x = (rand() % ((SCREEN_WIDTH / CUBE_SIZE) - 2) + 1) * CUBE_SIZE;
    b.y = (rand() % ((GAME_HEIGHT / CUBE_SIZE) - 2) + 1) * CUBE_SIZE;
    b.color = SDL_MapRGB(sdl.screen->format, 0, 0, 255);

    r.x = 0;
    r.y = 0;
    r.color = SDL_MapRGB(sdl.screen->format, 255, 0, 0);
    r.spawnTime = 0;
    r.duration = 10.0;
    r.visible = false;

    for (int i = 1; i < s.length; i++) {
        s.bodyX[i] = s.bodyX[i - 1];
        s.bodyY[i] = s.bodyY[i - 1];
    }

    for (int i = 0; i < HISTORY_SIZE; i++) {
        s.historyX[i] = s.bodyX[0];
        s.historyY[i] = s.bodyY[0];
    }
}


float GetSnakeSpeed(GameTime t, Snake& s) {
    s.speed = (t.snakeTime * SNAKE_SPEED_UP * SNAKE_SPEED + SNAKE_SPEED);
    if (s.speed > MAX_SNAKE_SPEED) {
        t.snakeLimitTime = t.snakeTime;
        s.speed = MAX_SNAKE_SPEED;
    }
    return s.speed;
}


bool UserInput(bool* quit, Snake& s, SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == END_GAME_KEY) *quit = true;
        else if (event.key.keysym.sym == NEW_GAME_KEY) return false;
        else if (event.key.keysym.sym == SDLK_UP && s.velocityY == 0) {
            s.velocityX = 0;
            s.velocityY = -1;
        }
        else if (event.key.keysym.sym == SDLK_DOWN && s.velocityY == 0) {
            s.velocityX = 0;
            s.velocityY = 1;
        }
        else if (event.key.keysym.sym == SDLK_LEFT && s.velocityX == 0) {
            s.velocityX = -1;
            s.velocityY = 0;
        }
        else if (event.key.keysym.sym == SDLK_RIGHT && s.velocityX == 0) {
            s.velocityX = 1;
            s.velocityY = 0;
        }
    }
    return true;
}

void BlueDotCollision(Snake& s, Dot& b) {
    if (abs((int)s.bodyX[0] - b.x) <= CUBE_SIZE && abs((int)s.bodyY[0] - b.y) <= CUBE_SIZE) {
        s.eaten++;
        if (s.length < MAX_SNAKE_LENGTH) {
            s.length = s.length + 5;
        }
        bool okPos;
        do {
            okPos = true;
            b.x = (rand() % ((SCREEN_WIDTH / CUBE_SIZE) - 2) + 1) * CUBE_SIZE;
            b.y = (rand() % ((GAME_HEIGHT / CUBE_SIZE) - 2) + 1) * CUBE_SIZE;

            for (int i = 0; i < HISTORY_SIZE; i++) {
                if (fabs(b.x - s.historyX[i]) < CUBE_SIZE &&
                    fabs(b.y - s.historyY[i]) < CUBE_SIZE) {
                    okPos = 0;
                    break;
                }
            }
        } while (!okPos);
    }
}

void RedDotCollision(Snake& s, Dot& r, GameTime& t) {
    if (r.visible && fabs(s.bodyX[0] - r.x) <= CUBE_SIZE && fabs(s.bodyY[0] - r.y) <= CUBE_SIZE) {

        s.eaten++;
       
        if (rand() % 2 && s.length >= 10) {
            s.length = s.length - 5;
        }
        else if (s.speed > SNAKE_SPEED && t.snakeTime > SNAKE_SPEED_DOWN) {
            if (s.speed == MAX_SNAKE_SPEED) {
                while (s.speed >= MAX_SNAKE_SPEED) {
                    t.snakeTime = t.snakeTime - SNAKE_SPEED_DOWN;
                    s.speed = (t.snakeTime * SNAKE_SPEED_UP * SNAKE_SPEED + SNAKE_SPEED);
                }
            }
            else t.snakeTime = t.snakeTime - SNAKE_SPEED_DOWN;
        }
        else if (s.length >= 10) s.length = s.length - 5;
        r.visible = false;
    }
    else if (r.visible && (t.worldTime - r.spawnTime > r.duration)) {
        r.visible = false;
    }
}



void MoveSnake(Snake& s, GameTime& time, double delta) {

    float currentSpeed = GetSnakeSpeed(time, s);

    // history update(deleting useless elements)
    for (int i = HISTORY_SIZE - 1; i > 0; i--) {
        s.historyX[i] = s.historyX[i - 1];
        s.historyY[i] = s.historyY[i - 1];
    }
    // writting head to history
    s.historyX[0] = s.bodyX[0];
    s.historyY[0] = s.bodyY[0];

    // head movement
    s.bodyX[0] += s.velocityX * currentSpeed * delta;
    s.bodyY[0] += s.velocityY * currentSpeed * delta;

    // going right when reaching boarders
    if (s.bodyX[0] <= CUBE_SIZE && s.bodyY[0] <= CUBE_SIZE) { s.velocityX = 1; s.velocityY = 0; }
    else if (s.bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE && s.bodyY[0] <= CUBE_SIZE) { s.velocityX = 0; s.velocityY = 1; }
    else if (s.bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE && s.bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { s.velocityX = -1; s.velocityY = 0; }
    else if (s.bodyX[0] <= CUBE_SIZE && s.bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { s.velocityX = 0; s.velocityY = -1; }
    else if (s.bodyX[0] <= CUBE_SIZE) { s.velocityX = 0; s.velocityY = -1; }
    else if (s.bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE) { s.velocityX = 0; s.velocityY = 1; }
    else if (s.bodyY[0] <= CUBE_SIZE) { s.velocityX = 1;  s.velocityY = 0; }
    else if (s.bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { s.velocityX = -1; s.velocityY = 0; }

    // going to the other side when reaching boarders
    //if (bodyX[0] < 0) bodyX[0] = SCREEN_WIDTH;
    //else if (bodyX[0] > SCREEN_WIDTH) bodyX[0] = 0;
    //if (bodyY[0] < 0) bodyY[0] = GAME_HEIGHT;
    //else if (bodyY[0] > GAME_HEIGHT) bodyY[0] = 0;
    // 
    // other parts movement
    for (int i = 1; i < s.length; i++) {
        float speed = (time.snakeTime * SNAKE_SPEED_UP + 1);
        if (speed > MAX_SNAKE_SPEED / SNAKE_SPEED) speed = (MAX_SNAKE_SPEED / SNAKE_SPEED);
        int historyIndex = i * (HISTORY_SIZE / CUBE_SIZE) / speed;
        s.bodyX[i] = s.historyX[historyIndex];
        s.bodyY[i] = s.historyY[historyIndex];
    }
}

void Draw(SDLStruct& sdl, Snake& s, Dot& b, Dot& r, GameTime& time) {
    char text[128];

    SDL_FillRect(sdl.screen, NULL, SDL_MapRGB(sdl.screen->format, 0x00, 0x00, 0x00));

    // Snake
    for (int i = 0; i < s.length; i++) {
        DrawSurface(sdl.screen, sdl.body, (int)s.bodyX[i], (int)s.bodyY[i]);
    }

    //blueDot 
    DrawDot(sdl.screen, b.x, b.y, b.color);

    //redDot
    if (r.visible) {
        DrawDot(sdl.screen, r.x, r.y, r.color);
    }

    // Updates 
    DrawRectangle(sdl.screen, 4, GAME_HEIGHT + 4, SCREEN_WIDTH - 8, 36, SDL_MapRGB(sdl.screen->format, 0xFF, 0x00, 0x00), SDL_MapRGB(sdl.screen->format, 0x11, 0x11, 0xCC));
    sprintf(text, "Elapsed time = %.1lfs  %.0lfFPS  Speed: %.1lfx  Length:%d  Points:%d", time.worldTime, time.fps, (s.speed/SNAKE_SPEED), s.length, s.eaten);
    DrawString(sdl.screen, sdl.screen->w / 2 - strlen(text) * 8 / 2, GAME_HEIGHT + 10, text, sdl.charset);
    sprintf(text, "Esc - exit, N - new game, Arrow keys - move");
    DrawString(sdl.screen, sdl.screen->w / 2 - strlen(text) * 8 / 2, GAME_HEIGHT + 26, text, sdl.charset);

    DrawRectangle(sdl.screen, 4, GAME_HEIGHT + 46, SCREEN_WIDTH - 8, 18, SDL_MapRGB(sdl.screen->format, 0xFF, 0x00, 0x00), SDL_MapRGB(sdl.screen->format, 0x11, 0x11, 0xCC));
    int count = (int)(time.worldTime - r.spawnTime);
    DrawRectangle(sdl.screen, 4, GAME_HEIGHT + 46, count*(SCREEN_WIDTH - 8)/r.duration , 18, SDL_MapRGB(sdl.screen->format, 0xFF, 0x00, 0x00), SDL_MapRGB(sdl.screen->format, 0xFF, 0x00, 0x00));
    SDL_UpdateTexture(sdl.scrtex, NULL, sdl.screen->pixels, sdl.screen->pitch);
    SDL_RenderCopy(sdl.renderer, sdl.scrtex, NULL, NULL);
    SDL_RenderPresent(sdl.renderer);
}

bool Collision(Snake& s) {
    for (int i = 4; i < s.length; i++) {
        if (fabs(s.bodyX[0] - s.bodyX[i]) <= CUBE_SIZE / 2 && fabs(s.bodyY[0] - s.bodyY[i]) <= CUBE_SIZE / 2) {
            return true;
        }
    }
    return false;
}

void SpawnRedDot(Dot& r, GameTime t) {
    if (!r.visible && (rand() % 1000 < 5)) {
        r.x = (rand() % ((SCREEN_WIDTH / CUBE_SIZE) - 2) + 1) * CUBE_SIZE;
        r.y = (rand() % ((GAME_HEIGHT / CUBE_SIZE) - 2) + 1) * CUBE_SIZE;
        r.spawnTime = t.worldTime;
        r.visible = true;
    }

}


void GameOver(bool* quit, SDLStruct& sdl, Snake& s, Dot& b, Dot& r, GameTime& time) {
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
                    *quit = true;
                    gameOver = false;
                }
                else if (event.key.keysym.sym == NEW_GAME_KEY) {
                    gameOver = false;
                    InitGame(quit, s, b, r, sdl);
                    time = { 0, 0, 0, 0 };
                }
            }
        }
    }
}


int main(int argc, char** argv) {
    SDLStruct sdl;
    bool quit;
    Snake snake;
    Dot blueDot, redDot;
    GameTime time = { 0, 0, 0, 0 };

    if (InitSDL(sdl)) return 1;
    InitGame(&quit, snake, blueDot, redDot, sdl);

    int t1 = SDL_GetTicks();

    while (!quit) {
        int t2 = SDL_GetTicks();
        double delta = (t2 - t1) * 0.001;
        t1 = t2;

        time.worldTime += delta;
        time.snakeTime += delta;
        time.fpsTimer += delta;
        time.frames++;
        if (time.fpsTimer > 0.5) {
            time.fps = time.frames * 2;
            time.frames = 0;
            time.fpsTimer -= 0.5;
        }

        SpawnRedDot(redDot, time);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (!UserInput(&quit, snake, event)) {
                CleanSDL(sdl);
                time = { 0, 0, 0, 0 };
                if (InitSDL(sdl)) return 1;
                InitGame(&quit, snake, blueDot, redDot, sdl);
                int t1 = SDL_GetTicks();
                double delta = (t2 - t1) * 0.001;
                t1 = t2;

                time.worldTime += delta;
                time.snakeTime += delta;
                time.fpsTimer += delta;
                time.frames++;

                if (time.fpsTimer > 0.5) {
                    time.fps = time.frames * 2;
                    time.frames = 0;
                    time.fpsTimer -= 0.5;
                }
            }
        }
        MoveSnake(snake, time, delta);
        Draw(sdl, snake, blueDot, redDot, time);
        if (Collision(snake) && snake.bodyX[19] != SCREEN_WIDTH / 2) GameOver(&quit, sdl, snake, blueDot, redDot, time);
        BlueDotCollision(snake, blueDot);
        RedDotCollision(snake, redDot, time);
    }

    CleanSDL(sdl);
    return 0;
}
