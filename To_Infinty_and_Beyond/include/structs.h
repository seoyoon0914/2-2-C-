#ifndef STRUCTS_H
#define STRUCTS_H

// 게임 콘솔 시작을 위한 구조체
struct game_console_entry {
    char title[30];
    char value[16];
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect rect;
};

// x,y 좌표값 담는 구조체
struct position_t {
    float x;
    float y;
};

// 행성 구조체
struct planet_t {
    char *name;
    char *image;
    int radius;
    struct position_t position;
    float vx;
    float vy;
    float dx;
    float dy;
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_Rect projection;
    SDL_Color color;
    struct planet_t *moons[MAX_MOONS];
};

// 우주선 구조체
struct ship_t {
    char *image;
    int radius;
    struct position_t position;
    float angle;
    float vx;
    float vy;
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_Rect main_img_rect;
    SDL_Rect thrust_img_rect;
    SDL_Point rotation_pt;
};

// 배경화면 별에 대한 구조체
struct bgstar_t {
    struct position_t position;
    SDL_Rect rect;
    unsigned short opacity;
};

// 카메라 구조체
struct camera_t {
    float x;
    float y;
    int w;
    int h;
};

#endif
