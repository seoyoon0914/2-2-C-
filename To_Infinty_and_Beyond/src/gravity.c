/*
 * gravity.c - 태양과 행성간 만유인력을 구현하는 프로그램 _작성자 강미림
 */

#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "../include/common.h"
#include "../include/structs.h"

#define COSMIC_CONSTANT         7.75


 //행성 텍스처 free 시키기 (recursive)
static void cleanup_planets(struct planet_t *planet)
{
    int i = 0;

    for (i = 0; i < MAX_MOONS && planet->moons[i] != NULL; i++)
    {
        cleanup_planets(planet->moons[i]);
    }

    SDL_DestroyTexture(planet->texture);
    
    if (planet != NULL)
    {
        free(planet);
        planet = NULL;
    }
}


//  clean up resources
 
void cleanup_resources(struct planet_t *planet, struct ship_t *ship)
{
    // Cleanup planets
    cleanup_planets(planet);

    // Cleanup ship
    SDL_DestroyTexture(ship->texture);
}



//만유인력과 이에 따른 행성의 궤도 속도를 구하는 함수
float gravitation_vel(float height, int radius)
{
    float v;

    v = COSMIC_CONSTANT * sqrt(G_CONSTANT * radius * radius / height);

    return v;
}


/*
 * Calculate orbital velocity for object orbiting
 * at height around object with radius.
 *
 * Centripetal force:
 * Fc = m * v^2 / h
 * In this project, we assume that m = r^2
 *
 * Gravitational force:
 * Fg = G_CONSTANT * M * m / h^2
 * In this project, we assume that M = R^2, m = r^2
 *
 * Fc = Fg
 * r^2 * v^2 / h = G_CONSTANT * R^2 * r^2 / h^2
 * v = sqrt(G_CONSTANT * R^2 / h);
 */
