#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_timer.h>

#include "../include/common.h"
#include "../include/structs.h"

#define FPS                     60
#define STARS_SQUARE            10000
#define STARS_PER_SQUARE        5
#define SHIP_RADIUS             17
#define SHIP_STARTING_X         0
#define SHIP_STARTING_Y         -700
#define STAR_CUTOFF             60
#define PLANET_CUTOFF           10
#define LANDING_CUTOFF          3
#define SPEED_LIMIT             300
#define APPROACH_LIMIT          100
#define PROJECTION_OFFSET       10
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static enum {
    STAGE_OFF = -1,
    STAGE_0
} landing_stage = STAGE_OFF;

// SDL 기본 내장 구조체 각각의 변수 설정
SDL_Window *window = NULL;
SDL_DisplayMode display_mode;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;
SDL_Color text_color;

const int camera_on = TRUE;
static float velocity;
const float g_launch = 0.7 * G_CONSTANT;
const float g_thrust = 1 * G_CONSTANT;

// 키보드 입력값 초기 설정
int left = OFF;
int right = OFF;
int thrust = OFF;
int console = ON;

// 게임 콘솔 시작을 위한 준비
struct game_console_entry game_console_entries[LOG_COUNT];

// 함수 원형 선언하기
int init_sdl(void);
void close_sdl(void);
void poll_events(int *quit);
void log_game_console(struct game_console_entry entries[], int index, float value);
void update_game_console(struct game_console_entry entries[]);
void destroy_game_console(struct game_console_entry entries[]);
void log_fps(unsigned int time_diff);
void cleanup_resources(struct planet_t *planet, struct ship_t *ship);
float gravitation_vel(float height, int radius);
int create_bgstars(struct bgstar_t bgstars[], int max_bgstars, struct ship_t *);
void update_bgstars(struct bgstar_t bgstars[], int stars_count, struct ship_t *, const struct camera_t *);
struct planet_t create_solar_system(void);
struct ship_t create_ship(void);
void update_planets(struct planet_t *planet, struct planet_t *parent, struct ship_t *, const struct camera_t *);
void project_planet(struct planet_t *, const struct camera_t *);
void update_ship_velocity(struct planet_t *planet, struct planet_t *parent, struct ship_t *, const struct camera_t *);
void update_camera(struct camera_t *, struct ship_t *);
void update_ship(struct ship_t *, const struct camera_t *);

int main(int argc, char *argv[])  //작성자 공동
{
    int quit = 0;

    // 전역 좌표
    int x_coord = 0;
    int y_coord = 0;

    // SDL초기화 하기
    if (!init_sdl())
    {
        fprintf(stderr, "Error: could not initialize SDL.\n");
        return 1;
    }

    // 우주선 생성하기
    struct ship_t ship = create_ship();

    // 우주선의 x,y좌표값을 이용해 카메라 초기 위치 설정
    struct camera_t camera = {
        .x = ship.position.x - (display_mode.w / 2),
        .y = ship.position.y - (display_mode.h / 2),
        .w = display_mode.w,
        .h = display_mode.h
    };

    // 태양계 행성 생성하기
    struct planet_t sol = create_solar_system();

    // 배경에 별 생성하기
    int max_bgstars = (int) (display_mode.w * display_mode.h * STARS_PER_SQUARE / STARS_SQUARE);
    max_bgstars *= 1.3; // 안정성을 위해 0.3정도 공간을 더 추가함
    struct bgstar_t bgstars[max_bgstars];
    int bgstars_count = create_bgstars(bgstars, max_bgstars, &ship);

    // time을 담는 변수들
    unsigned int start_time, end_time;

    // 계속하여 모든 함수를 루프 시킴 _작성자 강미림
    while (!quit)
    {
        start_time = SDL_GetTicks();

        // event 실행하기
        poll_events(&quit);

        // 배경 색상 정하기
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // 렌더러 초기화 하기
        SDL_RenderClear(renderer);

        // 배경에 별 생성하기
        if (camera_on)
        {
            update_bgstars(bgstars, bgstars_count, &ship, &camera);
        }

        // 태양계 안에 있는 행성을 recursive 하게 업데이트 하기
        update_planets(&sol, NULL, &ship, &camera);

        // 카메라 업데이트 하기
        if (camera_on)
        {
            update_camera(&camera, &ship);
        }

        // 우주선 업데이트 하기
        update_ship(&ship, &camera);

        // 우주선 x,y 좌표값 업데이트 하기
        x_coord = ship.position.x;
        y_coord = ship.position.y;

        // 태양과 상대적인 좌표 값 저장
        log_game_console(game_console_entries, X_INDEX, x_coord);
        log_game_console(game_console_entries, Y_INDEX, y_coord);

        // 태양과 상대적인 속도 저장
        log_game_console(game_console_entries, V_INDEX, velocity);

        // 게임 콘솔 화면 업데이트
        if (console)
        {
            update_game_console(game_console_entries);
        }

        // buffer를 전환하기
        SDL_RenderPresent(renderer);

        // FPS 설정하기
        if ((1000 / FPS) > ((end_time = SDL_GetTicks()) - start_time))
        {
            SDL_Delay((1000 / FPS) - (end_time - start_time));
        }

        // FPS 저장
        log_fps(end_time - start_time);
    }

    // 게임 콘솔 화면 없애기
    destroy_game_console(game_console_entries);

    // 변수 값들을 모두 cleanup하기
    cleanup_resources(&sol, &ship);

    // SDL 닫기
    close_sdl();

    return 0;
}

/*
 * 배경화면에 별 생성하기 _작성자 이서윤
 */
int create_bgstars(struct bgstar_t bgstars[], int max_bgstars, struct ship_t *ship)
{
    int i = 0, row, column, is_star;
    int end = FALSE;

    for (row = 0; row < display_mode.h && !end; row++)
    {
        for (column = 0; column < display_mode.w && !end; column++)
        {
            is_star = rand() % STARS_SQUARE < STARS_PER_SQUARE;

            if (is_star)
            {
                struct bgstar_t star;
                star.position.x = column + ship->position.x;
                star.position.y = row + ship->position.y;
                star.rect.x = 0;
                star.rect.y = 0;

                if (rand() % 12 < 1)
                {
                    star.rect.w = 2;
                    star.rect.h = 2;
                }
                else
                {
                    star.rect.w = 1;
                    star.rect.h = 1;
                }

                star.opacity = (rand() % (246 - 30)) + 10; // Skip 0-9 and go up to 225
                bgstars[i++] = star;
            }

            if (i >= max_bgstars)
            {
                end = TRUE;
            }
        }
    }

    return i;
}

/*
 * 배경화면에 별들을 움직이게 하고 별 색상 부여 _ 작성자 이서윤
 */
void update_bgstars(struct bgstar_t bgstars[], int stars_count, struct ship_t *ship, const struct camera_t *camera)
{
    int i;

    for (i = 0; i < stars_count; i++)
    {
        bgstars[i].position.x -= 0.2 * ship->vx / FPS;
        bgstars[i].position.y -= 0.2 * ship->vy / FPS;

        bgstars[i].rect.x = (int) (bgstars[i].position.x + (camera->w / 2));
        bgstars[i].rect.y = (int) (bgstars[i].position.y + (camera->h / 2));

        // 오른쪽 바운더리
        if (bgstars[i].position.x > ship->position.x - camera->x)
        {
            bgstars[i].position.x -= camera->w;
        }
        // 왼쪽 바운더리
        else if (bgstars[i].position.x < camera->x - ship->position.x)
        {
            bgstars[i].position.x += camera->w;
        }

        // 위쪽 바운더리
        if (bgstars[i].position.y > ship->position.y - camera->y)
        {
            bgstars[i].position.y -= camera->h;
        }
        // 아래쪽 바운더리
        else if (bgstars[i].position.y < camera->y - ship->position.y)
        {
            bgstars[i].position.y += camera->h;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, bgstars[i].opacity);
        SDL_RenderFillRect(renderer, &bgstars[i].rect);
    }
}

/*
 * 카메라 위치 업데이트 하기_작성자 이서윤
 */
void update_camera(struct camera_t *camera, struct ship_t *ship)
{
    camera->x = ship->position.x - camera->w / 2;
    camera->y = ship->position.y - camera->h / 2;
}

/*
 *  우주선 생성하기_작성자 공동
 */
struct ship_t create_ship(void)
{
    struct ship_t ship;

    ship.image = "../assets/sprites/ship.png";
    ship.radius = SHIP_RADIUS;
    ship.position.x = SHIP_STARTING_X;
    ship.position.y = SHIP_STARTING_Y -3900;
    ship.vx = 0;
    ship.vy = 0;
    ship.angle = 0;
    SDL_Surface *surface = IMG_Load(ship.image);
    ship.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    ship.rect.x = ship.position.x - ship.radius;
    ship.rect.y = ship.position.y - ship.radius;
    ship.rect.w = 2 * ship.radius;
    ship.rect.h = 2 * ship.radius;
    ship.main_img_rect.x = 0; 
    ship.main_img_rect.y = 0;
    ship.main_img_rect.w = 162;
    ship.main_img_rect.h = 162;
    ship.thrust_img_rect.x = 256; 
    ship.thrust_img_rect.y = 0;
    ship.thrust_img_rect.w = 162;
    ship.thrust_img_rect.h = 162;

    // 지정 좌표 주위에서 우주선이 돌도록 하기
    ship.rotation_pt.x = ship.radius;
    ship.rotation_pt.y = ship.radius;

    return ship;
}

/*
 * 태양계 생성하기  _작성자 공동
 */
struct planet_t create_solar_system(void)
{
    // 태양
    struct planet_t sol;

    sol.name = "Sol";
    sol.image = "../assets/images/sol.png";
    sol.radius = 300;
    sol.position.x = 0;
    sol.position.y = 0;
    sol.vx = 0;
    sol.vy = 0;
    sol.dx = 0;
    sol.dy = 0;
    SDL_Surface *sol_surface = IMG_Load(sol.image);
    sol.texture = SDL_CreateTextureFromSurface(renderer, sol_surface);
    SDL_FreeSurface(sol_surface);
    sol.rect.x = sol.position.x - sol.radius;
    sol.rect.y = sol.position.y - sol.radius;
    sol.rect.w = 2 * sol.radius;
    sol.rect.h = 2 * sol.radius;
    sol.color.r = 255;
    sol.color.g = 255;
    sol.color.b = 0;
    sol.moons[0] = NULL;

    // 수성
    struct planet_t *mercury = NULL;
    mercury = (struct planet_t *) malloc(sizeof(struct planet_t));

    if (mercury != NULL)
    {
        mercury->name = "Mercury";
        mercury->image = "../assets/images/mercury.png";
        mercury->radius = 70;
        mercury->position.x = sol.position.x;
        mercury->position.y = sol.position.y - 1500;
        mercury->vx = gravitation_vel(abs((int) sol.position.y - (int) mercury->position.y), sol.radius); // 초기속도
        mercury->vy = 0;
        mercury->dx = 0;
        mercury->dy = 0;
        SDL_Surface *mercury_surface = IMG_Load(mercury->image);
        mercury->texture = SDL_CreateTextureFromSurface(renderer, mercury_surface);
        SDL_FreeSurface(mercury_surface);
        mercury->rect.x = mercury->position.x - mercury->radius;
        mercury->rect.y = mercury->position.y - mercury->radius;
        mercury->rect.w = 2 * mercury->radius;
        mercury->rect.h = 2 * mercury->radius;
        mercury->color.r = 192;
        mercury->color.g = 192;
        mercury->color.b = 192;
        mercury->moons[0] = NULL;

        sol.moons[0] = mercury;
        sol.moons[1] = NULL;
    }

    // 금성
    struct planet_t *venus = NULL;
    venus = (struct planet_t *) malloc(sizeof(struct planet_t));

    if (venus != NULL)
    {
        venus->name = "Venus";
        venus->image = "../assets/images/venus.png";
        venus->radius = 100;
        venus->position.x = sol.position.x;
        venus->position.y = sol.position.y - 3000;
        venus->vx = gravitation_vel(abs((int) sol.position.y - (int) venus->position.y), sol.radius); // 초기속도
        venus->vy = 0;
        venus->dx = 0;
        venus->dy = 0;
        SDL_Surface *venus_surface = IMG_Load(venus->image);
        venus->texture = SDL_CreateTextureFromSurface(renderer, venus_surface);
        SDL_FreeSurface(venus_surface);
        venus->rect.x = venus->position.x - venus->radius;
        venus->rect.y = venus->position.y - venus->radius;
        venus->rect.w = 2 * venus->radius;
        venus->rect.h = 2 * venus->radius;
        venus->color.r = 215;
        venus->color.g = 140;
        venus->color.b = 0;
        venus->moons[0] = NULL;

        sol.moons[1] = venus;
        sol.moons[2] = NULL;
    }

    // 지구
    struct planet_t *earth = NULL;
    earth = (struct planet_t *) malloc(sizeof(struct planet_t));

    if (earth != NULL)
    {
        earth->name = "Earth";
        earth->image = "../assets/images/earth.png";
        earth->radius = 100;
        earth->position.x = sol.position.x;
        earth->position.y = sol.position.y - 4500;
        earth->vx = gravitation_vel(abs((int) sol.position.y - (int) earth->position.y), sol.radius); // 초기속도
        earth->vy = 0;
        earth->dx = 0;
        earth->dy = 0;
        SDL_Surface *earth_surface = IMG_Load(earth->image);
        earth->texture = SDL_CreateTextureFromSurface(renderer, earth_surface);
        SDL_FreeSurface(earth_surface);
        earth->rect.x = earth->position.x - earth->radius;
        earth->rect.y = earth->position.y - earth->radius;
        earth->rect.w = 2 * earth->radius;
        earth->rect.h = 2 * earth->radius;
        earth->color.r = 135;
        earth->color.g = 206;
        earth->color.b = 235;
        earth->moons[0] = NULL;

        sol.moons[2] = earth;
        sol.moons[3] = NULL;

        // 달
        struct planet_t *moon = NULL;
        moon = (struct planet_t *) malloc(sizeof(struct planet_t));

        if (moon != NULL)
        {
            moon->name = "Moon";
            moon->image = "../assets/images/moon.png";
            moon->radius = 45;
            moon->position.x = earth->position.x;
            moon->position.y = earth->position.y - 710;
            moon->vx = gravitation_vel(abs((int) earth->position.y - (int) moon->position.y), earth->radius); // 초기속도
            moon->vy = 0;
            moon->dx = 0;
            moon->dy = 0;
            SDL_Surface *moon_surface = IMG_Load(moon->image);
            moon->texture = SDL_CreateTextureFromSurface(renderer, moon_surface);
            SDL_FreeSurface(moon_surface);
            moon->rect.x = moon->position.x - moon->radius;
            moon->rect.y = moon->position.y - moon->radius;
            moon->rect.w = 2 * moon->radius;
            moon->rect.h = 2 * moon->radius;
            moon->color.r = 220;
            moon->color.g = 220;
            moon->color.b = 220;
            moon->moons[0] = NULL;

            earth->moons[0] = moon;
            earth->moons[1] = NULL;
        }
    }

    // 화성
    struct planet_t *mars = NULL;
    mars = (struct planet_t *) malloc(sizeof(struct planet_t));

    if (mars != NULL)
    {
        mars->name = "Mars";
        mars->image = "../assets/images/mars.png";
        mars->radius = 85;
        mars->position.x = sol.position.x;
        mars->position.y = sol.position.y - 6000;
        mars->vx = gravitation_vel(abs((int) sol.position.y - (int) mars->position.y), sol.radius); // 초기속도
        mars->vy = 0;
        mars->dx = 0;
        mars->dy = 0;
        SDL_Surface *mars_surface = IMG_Load(mars->image);
        mars->texture = SDL_CreateTextureFromSurface(renderer, mars_surface);
        SDL_FreeSurface(mars_surface);
        mars->rect.x = mars->position.x - mars->radius;
        mars->rect.y = mars->position.y - mars->radius;
        mars->rect.w = 2 * mars->radius;
        mars->rect.h = 2 * mars->radius;
        mars->color.r = 255;
        mars->color.g = 69;
        mars->color.b = 0;
        mars->moons[0] = NULL;

        sol.moons[3] = mars;
        sol.moons[4] = NULL;
    }

    // 목성
    struct planet_t *jupiter = NULL;
    jupiter = (struct planet_t *) malloc(sizeof(struct planet_t));

    if (jupiter != NULL)
    {
        jupiter->name = "Jupiter";
        jupiter->image = "../assets/images/jupiter.png";
        jupiter->radius = 160;
        jupiter->position.x = sol.position.x;
        jupiter->position.y = sol.position.y - 7800;
        jupiter->vx = gravitation_vel(abs((int) sol.position.y - (int) jupiter->position.y), sol.radius); // 초기속도
        jupiter->vy = 0;
        jupiter->dx = 0;
        jupiter->dy = 0;
        SDL_Surface *jupiter_surface = IMG_Load(jupiter->image);
        jupiter->texture = SDL_CreateTextureFromSurface(renderer, jupiter_surface);
        SDL_FreeSurface(jupiter_surface);
        jupiter->rect.x = jupiter->position.x - jupiter->radius;
        jupiter->rect.y = jupiter->position.y - jupiter->radius;
        jupiter->rect.w = 2 * jupiter->radius;
        jupiter->rect.h = 2 * jupiter->radius;
        jupiter->color.r = 244;
        jupiter->color.g = 164;
        jupiter->color.b = 96;
        jupiter->moons[0] = NULL;

        sol.moons[4] = jupiter;
        sol.moons[5] = NULL;
    }

    // 토성
    struct planet_t *saturn = NULL;
    saturn = (struct planet_t *) malloc(sizeof(struct planet_t));

    if (saturn != NULL)
    {
        saturn->name = "Saturn";
        saturn->image = "../assets/images/saturn.png";
        saturn->radius = 150;
        saturn->position.x = sol.position.x;
        saturn->position.y = sol.position.y - 9200;
        saturn->vx = gravitation_vel(abs((int) sol.position.y - (int) saturn->position.y), sol.radius); // 초기속
        saturn->vy = 0;
        saturn->dx = 0;
        saturn->dy = 0;
        SDL_Surface *saturn_surface = IMG_Load(saturn->image);
        saturn->texture = SDL_CreateTextureFromSurface(renderer, saturn_surface);
        SDL_FreeSurface(saturn_surface);
        saturn->rect.x = saturn->position.x - saturn->radius;
        saturn->rect.y = saturn->position.y - saturn->radius;
        saturn->rect.w = 2 * saturn->radius;
        saturn->rect.h = 2 * saturn->radius;
        saturn->color.r = 244;
        saturn->color.g = 250;
        saturn->color.b = 80;
        saturn->moons[0] = NULL;

        sol.moons[5] = saturn;
        sol.moons[6] = NULL;
    }

    // 천왕성
    struct planet_t *uranus = NULL;
    uranus = (struct planet_t *) malloc(sizeof(struct planet_t));

    if (uranus != NULL)
    {
        uranus->name = "uranus";
        uranus->image = "../assets/images/uranus.png";
        uranus->radius = 170;
        uranus->position.x = sol.position.x;
        uranus->position.y = sol.position.y - 11500;
        uranus->vx = gravitation_vel(abs((int) sol.position.y - (int) uranus->position.y), sol.radius); //초기속도값 설정

        uranus->vy = 0;
        uranus->dx = 0;
        uranus->dy = 0;
        SDL_Surface *uranus_surface = IMG_Load(uranus->image);
        uranus->texture = SDL_CreateTextureFromSurface(renderer, uranus_surface);
        SDL_FreeSurface(uranus_surface);
        uranus->rect.x = uranus->position.x - uranus->radius;
        uranus->rect.y = uranus->position.y - uranus->radius;
        uranus->rect.w = 2 * uranus->radius;
        uranus->rect.h = 2 * uranus->radius;
        uranus->color.r = 0;
        uranus->color.g = 230;
        uranus->color.b = 50;
        uranus->moons[0] = NULL;

        sol.moons[6] = uranus;
        sol.moons[7] = NULL;
    }

    // 해왕성
    struct planet_t *neptune = NULL;
    neptune = (struct planet_t *) malloc(sizeof(struct planet_t));

    if (neptune != NULL)
    {
        neptune->name = "neptune";
        neptune->image = "../assets/images/neptune.png";
        neptune->radius = 140;
        neptune->position.x = sol.position.x;
        neptune->position.y = sol.position.y - 13500;
        neptune->vx = gravitation_vel(abs((int) sol.position.y - (int) neptune->position.y), sol.radius); // 초기 속도
        neptune->vy = 0;
        neptune->dx = 0;
        neptune->dy = 0;
        SDL_Surface *neptune_surface = IMG_Load(neptune->image);
        neptune->texture = SDL_CreateTextureFromSurface(renderer, neptune_surface);
        SDL_FreeSurface(neptune_surface);
        neptune->rect.x = neptune->position.x - neptune->radius;
        neptune->rect.y = neptune->position.y - neptune->radius;
        neptune->rect.w = 2 * neptune->radius;
        neptune->rect.h = 2 * neptune->radius;
        neptune->color.r = 0;
        neptune->color.g = 0;
        neptune->color.b = 250;
        neptune->moons[0] = NULL;

        sol.moons[7] = neptune;
        sol.moons[8] = NULL;
    }



    return sol;
}

/*
 * 행성을 지속적으로 업데이트 하기 _ 작성자 공동
 */
void update_planets(struct planet_t *planet, struct planet_t *parent, struct ship_t *ship, const struct camera_t *camera)
{
    if (parent != NULL)
    {
        float delta_x = 0.0;
        float delta_y = 0.0;
        float distance;
        float g_planet = G_CONSTANT;
        float dx = parent->dx;
        float dy = parent->dy;

        // 행성 위치 업데이트 하기
        planet->position.x += parent->dx;
        planet->position.y += parent->dy;

        // 얼마만큼 이동했는지 계산하기(거리 구하기)
        delta_x = parent->position.x - planet->position.x;
        delta_y = parent->position.y - planet->position.y;
        distance = sqrt(delta_x * delta_x + delta_y * delta_y);

        // 속도와 위치 이동 값 계산하기
        if (distance > (parent->radius + planet->radius))
        {
            g_planet = G_CONSTANT * (float) (parent->radius * parent->radius) / (distance * distance);

            planet->vx += g_planet * delta_x / distance;
            planet->vy += g_planet * delta_y / distance;

            dx = (float) planet->vx / FPS;
            dy = (float) planet->vy / FPS;

            planet->dx = dx;
            planet->dy = dy;
        }

        // 행성 속도 업데이트 하기
        planet->position.x += dx;
        planet->position.y += dy;
    }

    int i = 0;

    for (i = 0; i < MAX_MOONS && planet->moons[i] != NULL; i++)
    {
        update_planets(planet->moons[i], planet, ship, camera);
    }

    planet->rect.x = (int) (planet->position.x - planet->radius - camera->x);
    planet->rect.y = (int) (planet->position.y - planet->radius - camera->y);

    // 카메라 안에 있을 경우 행성 그리기
    if (planet->position.x - planet->radius <= camera->x + camera->w && planet->position.x + planet->radius > camera->x &&
        planet->position.y - planet->radius <= camera->y + camera->h && planet->position.y + planet->radius > camera->y)
    {
        SDL_RenderCopy(renderer, planet->texture, NULL, &planet->rect);
    }
    // 좌표축에 행성 OFFSET 그리기
    else
    {
        project_planet(planet, camera);
    }

    // 우주선 속도 업데이트 하기
    update_ship_velocity(planet, parent, ship, camera);
}

/*
 * 좌표축에 행성 OFFSET 그리기 _ 작성자 공동
 */
void project_planet(struct planet_t *planet, const struct camera_t *camera)
{
    float delta_x, delta_y, point;

    delta_x = planet->position.x - (camera->x + ((camera->w / 2) - PROJECTION_OFFSET));
    delta_y = planet->position.y - (camera->y + ((camera->h / 2) - PROJECTION_OFFSET));

    if (delta_x > 0 && delta_y < 0)
    {
        point = (int) (((camera->h / 2) - PROJECTION_OFFSET) * delta_x / (- delta_y));

        if (point <= (camera->w / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = (camera->w / 2) + point;
            planet->projection.y = PROJECTION_OFFSET;
        }
        else
        {
            point = (int) (((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET) * (- delta_y) / delta_x);
            planet->projection.x = camera->w - PROJECTION_OFFSET;
            planet->projection.y = point + PROJECTION_OFFSET;
        }
    }
    else if (delta_x > 0 && delta_y > 0)
    {
        point = (int) (((camera->w / 2) - PROJECTION_OFFSET) * delta_y / delta_x);

        if (point <= (camera->h / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = camera->w - PROJECTION_OFFSET;
            planet->projection.y = (camera->h / 2) + point;
        }
        else
        {
            point = (int) (((camera->h / 2) - PROJECTION_OFFSET) * delta_x / delta_y);
            planet->projection.x = (camera->w / 2) + point;
            planet->projection.y = camera->h - PROJECTION_OFFSET;
        }
    }
    else if (delta_x < 0 && delta_y > 0)
    {
        point = (int) (((camera->h / 2) - PROJECTION_OFFSET) * (- delta_x) / delta_y);

        if (point <= (camera->w / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = (camera->w / 2) - point;
            planet->projection.y = camera->h - PROJECTION_OFFSET;
        }
        else
        {
            point = (int) (((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET) * delta_y / (- delta_x));
            planet->projection.x = PROJECTION_OFFSET;
            planet->projection.y = camera->h - PROJECTION_OFFSET - point;
        }
    }
    else if (delta_x < 0 && delta_y < 0)
    {
        point = (int) (((camera->w / 2) - PROJECTION_OFFSET) * (- delta_y) / (- delta_x));

        if (point <= (camera->h / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = PROJECTION_OFFSET;
            planet->projection.y = (camera->h / 2) - point;
        }
        else
        {
            point = (int) (((camera->w / 2) - PROJECTION_OFFSET) - ((camera->h / 2) - PROJECTION_OFFSET) * (- delta_x) / (- delta_y));
            planet->projection.x = point + PROJECTION_OFFSET;
            planet->projection.y = PROJECTION_OFFSET;
        }
    }

    planet->projection.w = 5;
    planet->projection.h = 5;
    SDL_SetRenderDrawColor(renderer, planet->color.r, planet->color.g, planet->color.b, 255);
    SDL_RenderFillRect(renderer, &planet->projection);
}

/*
 * 우주선 속도 업데이트 하기 _ 작성자 공동
 */
void update_ship_velocity(struct planet_t *planet, struct planet_t *parent, struct ship_t *ship, const struct camera_t *camera)
{
    float delta_x = 0.0;
    float delta_y = 0.0;
    float distance;
    float g_planet = 0;
    int is_star = parent == NULL;
    int collision_point = planet->radius;

    delta_x = planet->position.x - ship->position.x;
    delta_y = planet->position.y - ship->position.y;
    distance = sqrt(delta_x * delta_x + delta_y * delta_y);

    // 행성과의 충돌 감지
    if (distance <= collision_point + ship->radius)
    {
        landing_stage = STAGE_0;
        g_planet = 0;

        if (is_star)
        {
            ship->vx = 0;
            ship->vy = 0;
        }
        else
        {
            ship->vx = planet->vx;
            ship->vy = planet->vy;
            ship->vx += parent->vx;
            ship->vy += parent->vy;
        }

        // 랜딩하는 각도 찾기
        if (ship->position.y == planet->position.y)
        {
            if (ship->position.x > planet->position.x)
            {
                ship->angle = 90;
                ship->position.x = planet->position.x + collision_point + ship->radius; // 충돌한 행성 표면에 우주선 고정시키기
            }
            else
            {
                ship->angle = 270;
                ship->position.x = planet->position.x - collision_point - ship->radius; // 충돌한 행성 표면에 우주선 고정시키기
            }
        }
        else if (ship->position.x == planet->position.x)
        {
            if (ship->position.y > planet->position.y)
            {
                ship->angle = 180;
                ship->position.y = planet->position.y + collision_point + ship->radius; // 충돌한 행성 표면에 우주선 고정시키기
            }
            else
            {
                ship->angle = 0;
                ship->position.y = planet->position.y - collision_point - ship->radius; // 충돌한 행성 표면에 우주선 고정시키기
            }
        }
        else
        {
            // 우주선의 x,y 좌표가 행성의 x,y좌표값보다 클 때
            if (ship->position.y > planet->position.y && ship->position.x > planet->position.x)
            {
                ship->angle = (asin(abs((int) (planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 180 - ship->angle;

            }
            // 우주선의 y좌표는 행성의 y좌표보다 크고 우주선의 x좌표는 행성의 x좌표보다 작을 때
            else if (ship->position.y > planet->position.y && ship->position.x < planet->position.x)
            {
                ship->angle = (asin(abs((int) (planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 180 + ship->angle;
            }
            // 우주선의 x,y좌표가 행성의 x,y 좌표값보다 작을 때
            else if (ship->position.y < planet->position.y && ship->position.x < planet->position.x)
            {
                ship->angle = (asin(abs((int) (planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 360 - ship->angle;
            }
            // 그 외의 경우
            else
            {
                ship->angle = asin(abs((int) (planet->position.x - ship->position.x)) / distance) * 180 / M_PI;
            }

            ship->position.x = ((ship->position.x - planet->position.x) * (collision_point + ship->radius) / distance) + planet->position.x; // 충돌한 행성 표면에 우주선 고정시키기
            ship->position.y = ((ship->position.y - planet->position.y) * (collision_point + ship->radius) / distance) + planet->position.y; // 충돌한 행성 표면에 우주선 고정시키기
        }

        // 발사 적용시키기
        if (thrust)
        {
            ship->vx -= g_launch * delta_x / distance;
            ship->vy -= g_launch * delta_y / distance;
        }
    }
    // PLANET_CUTOFF 범위 안에 우주선 들어올 경우
    else if ((is_star && distance < STAR_CUTOFF * planet->radius) ||
        (!is_star && distance < PLANET_CUTOFF * planet->radius))
    {
        g_planet = G_CONSTANT * (float) (planet->radius * planet->radius) / (distance * distance);

        ship->vx += g_planet * delta_x / distance;
        ship->vy += g_planet * delta_y / distance;
    }

    velocity = sqrt((ship->vx * ship->vx) + (ship->vy * ship->vy));
}

/*
 * 우주선 위치 업데이트 및 우주선 그리기 _작성자 공동
 */
void update_ship(struct ship_t *ship, const struct camera_t *camera)
{
    float radians;

    // 우주선 돌아가는 각도 업데이트
    if (right && !left && landing_stage == STAGE_OFF)
    {
        ship->angle += 3;
    }
    
    if (left && !right && landing_stage == STAGE_OFF)
    {
        ship->angle -= 3;
    }

    if (ship->angle > 360)
    {
        ship->angle -= 360;
    }

    // 우주선 발사 적용
    if (thrust)
    {
        landing_stage = STAGE_OFF;
        radians = M_PI * ship->angle / 180;

        ship->vx += g_thrust * sin(radians);
        ship->vy -= g_thrust * cos(radians);
    }

    // 우주선 속도 제한
    if (velocity > SPEED_LIMIT)
    {
        ship->vx = SPEED_LIMIT * ship->vx / velocity;
        ship->vy = SPEED_LIMIT * ship->vy / velocity;
    }

    // 우주선 위치 업데이트
    ship->position.x += (float) ship->vx / FPS;
    ship->position.y += (float) ship->vy / FPS;

    if (camera_on)
    {
        // 스크린의 중앙 값에 정적 rect 위치 (float를 int로 바꿀때 부정확하기 때문에 생기는 빠른 깜빡거림 고치기)
        ship->rect.x = (camera->w / 2) - ship->radius;
        ship->rect.y = (camera->h / 2) - ship->radius;
    }
    else
    {
        // 우주선의 위치값에 따른 동역학적 rect 위치
        ship->rect.x = (int) (ship->position.x - ship->radius - camera->x);
        ship->rect.y = (int) (ship->position.y - ship->radius - camera->y);
    }

    // 우주선 그리기
    SDL_RenderCopyEx(renderer, ship->texture, &ship->main_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);

    // 우주선 발사 장면 그리기
    if (thrust)
    {
        SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
    }
}
