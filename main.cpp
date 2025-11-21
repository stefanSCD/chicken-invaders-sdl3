#include <iostream>
#include <vector>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "circle.h"

struct Entity {
    glm::vec2 pos;
    glm::vec2 size;
};

//define window dimensions
constexpr int WINDOW_WIDTH{640};
constexpr int WINDOW_HEIGHT{480};
constexpr float PLAYER_SPEED{400.0f};
constexpr float BULLET_SPEED{600.0f};
// --- PARAMETRI INAMICI ---
float ENEMY_MOVE_SPEED = 100.0f; // Cât de repede merg stânga-dreapta
int enemyDirection = 1; // 1 = Dreapta, -1 = Stânga
float ENEMY_DROP_DISTANCE = 20.0f; // Cât coboară când lovesc marginea

struct Square {
    // jucator si rate
    glm::vec2 pos; // Poziția (X, Y)
    float size; // Lățimea laturii
    SDL_Color color; // Culoarea
    bool active; // Dacă e false, nu îl mai desenăm (e mort)

    void draw(SDL_Renderer *renderer) {
        if (!active) return;

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_FRect rect = {pos.x, pos.y, size, size};
        SDL_RenderFillRect(renderer, &rect);
    }
};

//define SDL Window related variables
SDL_Window *window{nullptr};
SDL_Renderer *renderer{nullptr};
SDL_Event currentEvent;
SDL_Color backgroundColor{255, 255, 255, 255};

Square player;
std::vector<Square> enemies; // Lista de inamici
std::vector<Circle> bullets;

bool quit{false};
bool isGameOver = false;
float mouseX{-1.0f}, mouseY{-1.0f};

float displayScale{1.0f};

bool checkSquareCollision(const Square &a, const Square &b) {
    return (a.pos.x < b.pos.x + b.size &&
            a.pos.x + a.size > b.pos.x &&
            a.pos.y < b.pos.y + b.size &&
            a.pos.y + a.size > b.pos.y);
}

bool initWindow() {
    bool success{true};

    //Try to initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s\n", SDL_GetError());
        success = false;
    } else {
        //Try to create the window and renderer
        displayScale = SDL_GetDisplayContentScale(1);

        if (!SDL_CreateWindowAndRenderer(
            "SDL Hello World Example",
            static_cast<int>(displayScale * WINDOW_WIDTH),
            static_cast<int>(displayScale * WINDOW_HEIGHT),
            0,
            &window, &renderer)) {
            SDL_Log("Failed to create window and renderer: %s\n", SDL_GetError());
            success = false;
        } else {
            //Apply global display scaling to renderer
            SDL_SetRenderScale(renderer, displayScale, displayScale);

            //Set background color
            SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b,
                                   backgroundColor.a);

            //Apply background color
            SDL_RenderClear(renderer);
        }
    }

    return success;
}

void initGame() {
    // 1. Setup Jucător (Pătrat Albastru jos)
    player.pos = glm::vec2(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT - 50);
    player.size = 30.0f;
    player.color = {0, 0, 255, 255}; // Albastru
    player.active = true;

    // 2. Setup Inamici (Grilă de Pătrate Roșii)
    int rows = 3;
    int cols = 6;
    float startX = 50.0f;
    float startY = 30.0f;
    float gap = 60.0f; // Distanța dintre ei

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Square enemy;
            enemy.pos = glm::vec2(startX + j * gap, startY + i * gap);
            enemy.size = 30.0f; // Rațele sunt pătrate
            enemy.color = {255, 0, 0, 255}; // Roșu
            enemy.active = true;
            enemies.push_back(enemy); // Îl băgăm în listă
        }
    }
}


void processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) quit = true;

        // TRAGERE (Pe eveniment Key Down, ca să nu tragi "mitralieră" dacă ții apăsat)
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_SPACE) {
                // Creăm un glonț (Cerc)
                Circle bullet;
                bullet.radius = 5.0f;
                // Glonțul pleacă din mijlocul jucătorului
                bullet.pos = glm::vec2(player.pos.x + player.size / 2, player.pos.y);
                bullet.color = {255, 0, 0, 255};

                bullets.push_back(bullet); // Adăugăm în vector
            } else if (event.key.key == SDLK_R && isGameOver) {
                // Resetăm variabilele
                enemies.clear(); // Ștergem inamicii vechi
                bullets.clear(); // Ștergem gloanțele
                isGameOver = false;

                // Re-inițializăm jocul (poziții, inamici noi)
                initGame();

                // Resetăm și parametrii globali de dificultate (dacă i-ai modificat)
                ENEMY_MOVE_SPEED = 100.0f;
                std::cout << "Game Restarted!" << std::endl;
            }
        }
    }
}

bool checkCollision(const Circle &bullet, const Square &enemy) {
    // Considerăm cercul ca fiind un pătrat mic pentru simplitate
    // (sau verificăm dacă centrul cercului a intrat în pătratul inamicului)

    float bulletLeft = bullet.pos.x - bullet.radius;
    float bulletRight = bullet.pos.x + bullet.radius;
    float bulletTop = bullet.pos.y - bullet.radius;
    float bulletBottom = bullet.pos.y + bullet.radius;

    float enemyLeft = enemy.pos.x;
    float enemyRight = enemy.pos.x + enemy.size;
    float enemyTop = enemy.pos.y;
    float enemyBottom = enemy.pos.y + enemy.size;

    // Verificăm suprapunerea
    if (bulletRight >= enemyLeft && bulletLeft <= enemyRight &&
        bulletBottom >= enemyTop && bulletTop <= enemyBottom) {
        return true;
    }
    return false;
}

void update(float dt) {
    if (isGameOver) return;
    // 1. Mișcare Jucător (înmulțim cu dt)
    const bool *keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_LEFT]) player.pos.x -= PLAYER_SPEED * dt;
    if (keys[SDL_SCANCODE_RIGHT]) player.pos.x += PLAYER_SPEED * dt;

    // Limite jucător
    if (player.pos.x < 0) player.pos.x = 0;
    if (player.pos.x + player.size > WINDOW_WIDTH) player.pos.x = WINDOW_WIDTH - player.size;

    // 2. Mișcare Gloanțe (înmulțim cu dt)
    for (int i = bullets.size() - 1; i >= 0; i--) {
        bullets[i].pos.y -= BULLET_SPEED * dt; // <--- Aici e magia

        bool hit = false;
        for (auto &enemy: enemies) {
            if (enemy.active && checkCollision(bullets[i], enemy)) {
                enemy.active = false;
                hit = true;
                break;
            }
        }

        if (hit || bullets[i].pos.y + bullets[i].radius < 0) {
            bullets.erase(bullets.begin() + i);
        }
    }
    bool hitEdge = false;

    // Pasul A: Îi mutăm pe toți pe orizontală
    for (auto &enemy: enemies) {
        if (!enemy.active) continue;

        enemy.pos.x += enemyDirection * ENEMY_MOVE_SPEED * dt;

        // Pasul B: Verificăm dacă vreunul a lovit marginea
        // (Dacă merge dreapta și trece de margine SAU merge stânga și trece de 0)
        if ((enemyDirection == 1 && enemy.pos.x + enemy.size > WINDOW_WIDTH) ||
            (enemyDirection == -1 && enemy.pos.x < 0)) {
            hitEdge = true;
        }

        // Condiția A: Inamicul a ajuns jos (la nivelul jucătorului)
        if (enemy.pos.y + enemy.size >= player.pos.y) {
            isGameOver = true;
            std::cout << "GAME OVER: Invazie!" << std::endl;
        }

        // Condiția B: Inamicul a lovit jucătorul
        if (checkSquareCollision(player, enemy)) {
            isGameOver = true;
            std::cout << "GAME OVER: Coliziune!" << std::endl;
        }
    }

    // Pasul C: Dacă s-a lovit marginea, schimbăm direcția și coborâm TOATĂ trupa
    if (hitEdge) {
        enemyDirection *= -1; // Inversăm direcția (1 devine -1, -1 devine 1)

        for (auto &enemy: enemies) {
            enemy.pos.y += ENEMY_DROP_DISTANCE;

            // Corecție fină: îi împingem puțin înapoi ca să nu rămână blocați în perete
            if (enemyDirection == 1) enemy.pos.x += 5;
            else enemy.pos.x -= 5;
        }

        // Opțional: Pe măsură ce coboară, devin mai agresivi (mai rapizi)
        ENEMY_MOVE_SPEED += 20.0f;
    }
}

void drawFrame() {
    //Clear the background
    if (isGameOver) {
        SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255); // Roșu închis
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Negru
    }
    SDL_RenderClear(renderer);

    player.draw(renderer);

    // 2. Desenăm Inamicii
    for (Square &enemy: enemies) {
        enemy.draw(renderer);
    }

    // 3. Desenăm Gloanțele (folosind funcția din circle.h)
    for (Circle &bullet: bullets) {
        bullet.draw(renderer);
    }

    //Update window
    SDL_RenderPresent(renderer);
}

void cleanup() {
    //Destroy renderer
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    //Destroy window
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    //Quit SDL
    SDL_Quit();
}

int main() {
    backgroundColor = {0, 0, 0, 255}; // Negru

    if (!initWindow()) return -1;
    SDL_zero(currentEvent);
    initGame();

    // Variabile pentru timp
    Uint64 lastTime = SDL_GetTicks();

    while (!quit) {
        // --- CALCUL DELTA TIME ---
        Uint64 currentTime = SDL_GetTicks();
        // Facem diferența și împărțim la 1000.0f ca să obținem secunde (ex: 0.016s)
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        // -------------------------

        processEvents();

        update(deltaTime); // Trimitem timpul în funcție

        drawFrame();
    }

    cleanup();
    return 0;
}
