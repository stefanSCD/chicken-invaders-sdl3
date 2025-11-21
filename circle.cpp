#include "circle.h"

void plotPoints(SDL_Renderer *renderer, int x, int y, Circle circle) {
    SDL_SetRenderDrawColor(renderer,
                           circle.color.r,
                           circle.color.g,
                           circle.color.b,
                           circle.color.a);
    SDL_RenderPoint(renderer,
                    circle.pos.x + x,
                    circle.pos.y + y);
    SDL_RenderPoint(renderer,
                    circle.pos.x + x,
                    circle.pos.y - y);
    SDL_RenderPoint(renderer,
                    circle.pos.x - x,
                    circle.pos.y - y);
    SDL_RenderPoint(renderer,
                    circle.pos.x - x,
                    circle.pos.y + y);
    SDL_RenderPoint(renderer,
                    circle.pos.x + y,
                    circle.pos.y + x);
    SDL_RenderPoint(renderer,
                    circle.pos.x + y,
                    circle.pos.y - x);
    SDL_RenderPoint(renderer,
                    circle.pos.x - y,
                    circle.pos.y - x);
    SDL_RenderPoint(renderer,
                    circle.pos.x - y,
                    circle.pos.y + x);
}

void plotLines(SDL_Renderer *renderer, int x, int y, Circle circle) {
    SDL_SetRenderDrawColor(renderer,
                           circle.color.r,
                           circle.color.g,
                           circle.color.b,
                           circle.color.a);
    SDL_RenderLine(renderer,
                   circle.pos.x - x,
                   circle.pos.y + y,
                   circle.pos.x + x,
                   circle.pos.y + y);
    SDL_RenderLine(renderer,
                   circle.pos.x - x,
                   circle.pos.y - y,
                   circle.pos.x + x,
                   circle.pos.y - y);
    SDL_RenderLine(renderer,
                   circle.pos.x - y,
                   circle.pos.y + x,
                   circle.pos.x + y,
                   circle.pos.y + x);
    SDL_RenderLine(renderer,
                   circle.pos.x - y,
                   circle.pos.y - x,
                   circle.pos.x + y,
                   circle.pos.y - x);
}

void DrawCircle(SDL_Renderer *renderer, Circle circle) {
    int currentY = static_cast<int>(circle.radius);
    int d = static_cast<int>((1.0f / 4.0f) - circle.radius);

    for (int currentX = 0; currentX <= ceil(circle.radius / sqrt(2.0f)); currentX++) {
        plotPoints(renderer, currentX, currentY, circle);
        d += 2 * currentX + 1;

        if (d > 0) {
            d += 2 - 2 * currentY;
            currentY--;
        }
    }
}

void FillCircle(SDL_Renderer *renderer, Circle circle) {
    int currentY = static_cast<int>(circle.radius);
    int d = static_cast<int>((1.0f / 4.0f) - circle.radius);

    for (int currentX = 0; currentX <= ceil(circle.radius / sqrt(2.0f)); currentX++) {
        plotLines(renderer, currentX, currentY, circle);
        d += 2 * currentX + 1;

        if (d > 0) {
            d += 2 - 2 * currentY;
            currentY--;
        }
    }
}

Circle::Circle() {
    this->radius = 0.0f;
    this->pos = glm::vec2(0.0f, 0.0f);
    this->color = SDL_Color{0, 0, 0, 255};
}

Circle::Circle(float r, glm::vec2 c, SDL_Color col) {
    this->radius = r;
    this->pos = c;
    this->color = col;
}

void Circle::draw(SDL_Renderer *renderer) {
    FillCircle(renderer, *this);
    Circle circle = *this;
    circle.color.r /= 2;
    circle.color.g /= 2;
    circle.color.b /= 2;
    DrawCircle(renderer, circle);
}
