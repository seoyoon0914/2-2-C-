/*
 * console.c - 게임 작동, 이미지 렌더링에 필요한 함수들을 정의하는 프로그램 _작성자 이서윤
 */

#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/common.h"
#include "../include/structs.h"

extern struct game_console_entry game_console_entries[];
extern SDL_Renderer *renderer;
extern TTF_Font *font;
extern SDL_Color text_color;

void log_game_console(struct game_console_entry entries[], int index, float value);


 // log FPS 함수 정의, 게임 작동에 필요한 타임 계산 

void log_fps(unsigned int time_diff)
{
    static unsigned total_time = 0, current_fps = 0;

    total_time += time_diff;
    current_fps++;

    if (total_time >= 1000)
    {
        log_game_console(game_console_entries, FPS_INDEX, (float) current_fps);

        total_time = 0;
        current_fps = 0;
    }
}


  // 게임 콘솔의 entries 배열에 데이터를 저장 함수
 
void log_game_console(struct game_console_entry entries[], int index, float value)
{
    char text[16];

    sprintf(text, "%f", value);
    strcpy(entries[index].value, text);
}


 // 구조체 내부의 entries 배열을 받아 이미지 렌더링 업데이트 
 
void update_game_console(struct game_console_entry entries[])
{
    int i;

    for (i = 0; i < LOG_COUNT; i++)
    {
        entries[i].surface = TTF_RenderText_Solid(font, entries[i].value, text_color);
        entries[i].texture = SDL_CreateTextureFromSurface(renderer, entries[i].surface);
        entries[i].rect.x = 120;
        entries[i].rect.y = (i + 1) * 20;
        entries[i].rect.w = 100;
        entries[i].rect.h = 15;
        SDL_RenderCopy(renderer, entries[i].texture, NULL, &entries[i].rect);
    }
}


 // 게임 종료, 이미지 Destroy & free 시키기
void destroy_game_console(struct game_console_entry entries[])
{
    int i;

    for (i = 0; i < LOG_COUNT; i++)
    {
        SDL_DestroyTexture(entries[i].texture);
        SDL_FreeSurface(entries[i].surface);
    }
}
