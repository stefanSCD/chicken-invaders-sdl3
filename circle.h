#ifndef CIRCLE_H
#define CIRCLE_H

#include <SDL3/SDL.h>
#include "glm/glm.hpp"

struct Circle {
    float radius;
    glm::vec2 pos;
    SDL_Color color;
    glm::vec2 dir;
    float speed;

    Circle();
    Circle(float r, glm::vec2 c, SDL_Color color);

    void draw(SDL_Renderer* renderer);
};

void plotPoints(SDL_Renderer *renderer, int x, int y, Circle circle);
void plotLines(SDL_Renderer *renderer, int x, int y, Circle circle);
void DrawCircle(SDL_Renderer *renderer, Circle circle);
void FillCircle(SDL_Renderer *renderer, Circle circle);

#endif //CIRCLE_H