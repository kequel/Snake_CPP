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

#define CUBE_SIZE 10

// Funkcja rysuj¹ca napis na powierzchni
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

// Funkcja rysuj¹ca powierzchniê (sprite) na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
    SDL_Rect dest;
    dest.x = x - sprite->w / 2;
    dest.y = y - sprite->h / 2;
    dest.w = sprite->w;
    dest.h = sprite->h;
    SDL_BlitSurface(sprite, NULL, screen, &dest);
}

// Funkcja rysuj¹ca pojedynczy piksel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    *(Uint32*)p = color;
}

// Funkcja rysuj¹ca liniê
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
    for (int i = 0; i < l; i++) {
        DrawPixel(screen, x, y, color);
        x += dx;
        y += dy;
    };
}

// Funkcja rysuj¹ca prostok¹t
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


int main(int argc, char** argv) {
    bool quit;
    bool new_game;
    int t1, t2, frames, rc;
    double delta, worldTime, fpsTimer, fps;
    SDL_Event event;
    SDL_Surface* screen, * charset;
    SDL_Surface* body; // Zmienna do przechowywania powierzchni dla body.bmp
    SDL_Texture* scrtex;
    SDL_Window* window;
    SDL_Renderer* renderer;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
    if (rc != 0) {
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetWindowTitle(window, "Snake Movement Example");

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_ShowCursor(SDL_DISABLE);

    charset = SDL_LoadBMP("./cs8x8.bmp");
    if (charset == NULL) {
        printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }
    SDL_SetColorKey(charset, true, 0x000000);

    // Wczytanie obrazu body.bmp
    body = SDL_LoadBMP("./body.bmp");
    if (body == NULL) {
        printf("SDL_LoadBMP(body.bmp) error: %s\n", SDL_GetError());
        SDL_FreeSurface(charset);
        SDL_FreeSurface(screen);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }

    char text[128];
    int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    int red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

    t1 = SDL_GetTicks();
    frames = 0;
    fpsTimer = 0;
    fps = 0;
    quit = 0;
    worldTime = 0;
    new_game = 0;

    double bodyX[50] = { 0 };
    double bodyY[50] = { 0 };
    double historyX[500] = { 0 };
    double historyY[500] = { 0 };

    int historyLength = 500; // history complexity
    int bodyLength = 10;     // length

    bodyX[0] = SCREEN_WIDTH / 2; // head
    bodyY[0] = SCREEN_HEIGHT / 2;

    for (int i = 1; i < bodyLength; i++) {
        bodyX[i] = bodyX[i - 1] + 20;
        bodyY[i] = bodyY[i - 1];
    }

    // starting history 
    for (int i = 0; i < historyLength; i++) {
        historyX[i] = bodyX[0];
        historyY[i] = bodyY[0];
    }

    double velocityX = 0;
    double velocityY = 0;

    while (!quit) {
        t2 = SDL_GetTicks();
        delta = (t2 - t1) * 0.001;
        t1 = t2;

        worldTime += delta;

        SDL_FillRect(screen, NULL, black);

        // history update(deleting useless elements)
        for (int i = historyLength - 1; i > 0; i--) {
            historyX[i] = historyX[i - 1];
            historyY[i] = historyY[i - 1];
        }
        // writting head to history
            historyX[0] = bodyX[0];
            historyY[0] = bodyY[0];

        // head movement
        bodyX[0] += velocityX * delta;
        bodyY[0] += velocityY * delta;

        // going right when reaching boarders
        if (bodyX[0] <= CUBE_SIZE && bodyY[0] <= CUBE_SIZE ) { velocityX = 200; velocityY = 0;}
        else if (bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE && bodyY[0] <= CUBE_SIZE) { velocityX = 0; velocityY = 200; }
        else if (bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE && bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { velocityX = -200; velocityY = 0; }
        else if (bodyX[0] <= CUBE_SIZE && bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { velocityX = 0; velocityY = -200; }
        else if (bodyX[0] <= CUBE_SIZE) { velocityX = 0; velocityY = -200; }
        else if (bodyX[0] >= SCREEN_WIDTH - CUBE_SIZE) { velocityX = 0; velocityY = 200; }
        else if (bodyY[0] <= CUBE_SIZE) { velocityX = 200; velocityY = 0; }
        else if (bodyY[0] >= GAME_HEIGHT - CUBE_SIZE) { velocityX = -200; velocityY = 0; }

        // going to the other side when reaching boarders
        //if (bodyX[0] < 0) bodyX[0] = SCREEN_WIDTH;
        //else if (bodyX[0] > SCREEN_WIDTH) bodyX[0] = 0;
        //if (bodyY[0] < 0) bodyY[0] = GAME_HEIGHT;
        //else if (bodyY[0] > GAME_HEIGHT) bodyY[0] = 0;

        // other parts movement
        for (int i = 1; i < bodyLength; i++) {
            int historyIndex = i * (historyLength / bodyLength); // Dopasowanie odstêpu w historii
            bodyX[i] = historyX[historyIndex];
            bodyY[i] = historyY[historyIndex];
        }

        // drawing
        for (int i = 0; i < bodyLength; i++) {
            DrawSurface(screen, body, (int)bodyX[i], (int)bodyY[i]);
        }

        fpsTimer += delta;
        if (fpsTimer > 0.5) {
            fps = frames * 2;
            frames = 0;
            fpsTimer -= 0.5;
        }

        // info
        DrawRectangle(screen, 4, GAME_HEIGHT + 4, SCREEN_WIDTH - 8, 36, red, blue);
        sprintf(text, "Elapsed time = %.1lf s  %.0lf FPS", worldTime, fps);
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, GAME_HEIGHT + 10, text, charset);
        sprintf(text, "Esc - exit, Arrow keys - move");
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, GAME_HEIGHT + 26, text, charset);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
                else if(event.key.keysym.sym == 'n') new_game = 1;
                else if (event.key.keysym.sym == SDLK_UP) { velocityX = 0; velocityY = -200; }
                else if (event.key.keysym.sym == SDLK_DOWN) { velocityX = 0; velocityY = 200; }
                else if (event.key.keysym.sym == SDLK_LEFT) { velocityX = -200; velocityY = 0; }
                else if (event.key.keysym.sym == SDLK_RIGHT) { velocityX = 200; velocityY = 0; }
                break;
            case SDL_QUIT:
                quit = 1;
                break;
            }
        }
        frames++;
    }


    SDL_FreeSurface(charset);
    SDL_FreeSurface(screen);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
