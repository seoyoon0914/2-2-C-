/*
 * move_ship.c - 우주선의 움직임을 구현하는 프로그램 _작성자 공동
 */

#include <SDL2/SDL.h>

#include "../include/common.h"

extern int left;
extern int right;
extern int thrust;
extern int console;

/*
 * 우주선 조종하기: 상하좌우, 연료부스트(space), 조작키를 누를 때랑 누르지 않을 때를 나눠서 구현
 */
void poll_events(int *quit)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)      //switch문으로 나누어서 받기
        {
            case SDL_QUIT:
                *quit = 1;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_A:
                    case SDL_SCANCODE_LEFT:
                        left = ON;
                        break;
                    case SDL_SCANCODE_D:
                    case SDL_SCANCODE_RIGHT:
                        right = ON;
                        break;
                    case SDL_SCANCODE_SPACE:
                        thrust = ON;
                        break;
                    default:
                        break;
                }
                break;
             case SDL_KEYUP:
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_A:
                    case SDL_SCANCODE_LEFT:
                        left = OFF;
                        break;
                    case SDL_SCANCODE_D:
                    case SDL_SCANCODE_RIGHT:
                        right = OFF;
                        break;
                    case SDL_SCANCODE_SPACE:
                        thrust = OFF;
                        break;
                    case SDL_SCANCODE_TAB:
                        console = console ? OFF : ON;
                        break;
                    default:
                        break;
                }
                break;
        }
    }
}
