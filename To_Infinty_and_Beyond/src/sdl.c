/*
 * sdl.c - sdl 게임 작동에 필요한 함수들을 정의한 프로그램 _작성자 공동
 */

#include <SDL2/SDL.h>        // SDL2 라이브러리 불러오기
#include <SDL2/SDL_ttf.h>

#include "../include/common.h"  



#define FONT_SIZE           14

//sdl 내장 함수 불러오
extern SDL_Window *window;
extern SDL_DisplayMode display_mode;
extern SDL_Renderer *renderer;
extern TTF_Font *font;
extern SDL_Color text_color;


 // SDL
int init_sdl(void)
{
    
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Could not initialize SDL: %s\n", SDL_GetError());
        return FALSE;
    }

    // display mode 불러오기
    if (SDL_GetDesktopDisplayMode(0, &display_mode))
    {
        SDL_Log("Could not get desktop display mode: %s\n", SDL_GetError());
        return FALSE;
    }


    // 윈도우 창 생성기
    window = SDL_CreateWindow("Gravity",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              display_mode.w,
                              display_mode.h,
                              0);

    if (window == NULL)
    {
        SDL_Log("Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return FALSE;
    }

    // 풀스크린 뷰로 띄우기
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    // 컨텍스트를 활용해 2D rendering 생성하기
    Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    renderer = SDL_CreateRenderer(window, -1, render_flags);

    // 렌더러가 Null이면 에러
    if (renderer == NULL)
    {
        SDL_Log("Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    // SDL_ttf 라이브러리 초기화 함
    if (TTF_Init() == -1)
    {
        SDL_Log("Could not initialize SDL_ttf: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    // 폰트 로드하기
    font = TTF_OpenFont("../assets/fonts/consola.ttf", FONT_SIZE);

    // 폰트가 NULL 값이면 에러
    if (font == NULL)
    {
        SDL_Log("Could not load font: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    // 하얀색으로 텍스트 색상 설정
    text_color.r = 255;
    text_color.g = 255;
    text_color.b = 255;

    return TRUE;
}


 // 이미지, 창 종료 
void close_sdl(void)
{
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
