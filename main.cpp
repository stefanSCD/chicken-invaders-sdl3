#include <iostream>
#include <vector>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdlib>
#include <ctime>
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

std::vector<Circle> enemyBullets; // Lista de ouă
float eggTimer = 0.0f; // Cronometru intern
constexpr float EGG_SPAWN_TIME = 1.0f; // Trage un ou la fiecare 1 secundă
constexpr float EGG_SPEED = 300.0f; // Viteza oului

bool quit{false};
bool isGameOver = false;
float mouseX{-1.0f}, mouseY{-1.0f};
int score = 0;
float displayScale{1.0f};

void spawnEnemies() {
    enemies.clear(); // Ștergem inamicii vechi (chiar dacă sunt morți/inactive)

    int rows = 3;
    int cols = 6;

    // Putem crește numărul de rânduri pe măsură ce avansezi (opțional)
    // int rows = 3 + (score / 20); // Exemplu: la fiecare 20 puncte, un rând în plus

    float startX = 50.0f;
    float startY = 50.0f;
    float gap = 60.0f;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Square enemy;
            enemy.pos = glm::vec2(startX + j * gap, startY + i * gap);
            enemy.size = 30.0f;
            enemy.color = {255, 0, 0, 255};
            enemy.active = true;
            enemies.push_back(enemy);
        }
    }

    std::cout << "Val nou de inamici!" << std::endl;
}

// --- LOGICA PENTRU DESENAT SCORUL (Digital Style) ---
void drawDigit(SDL_Renderer *renderer, int digit, float x, float y, float size) {
    // Definim cele 7 segmente ale unui ceas digital
    //  A
    // F B
    //  G
    // E C
    //  D
    bool segments[10][7] = {
        {1, 1, 1, 1, 1, 1, 0}, // 0
        {0, 1, 1, 0, 0, 0, 0}, // 1
        {1, 1, 0, 1, 1, 0, 1}, // 2
        {1, 1, 1, 1, 0, 0, 1}, // 3
        {0, 1, 1, 0, 0, 1, 1}, // 4
        {1, 0, 1, 1, 0, 1, 1}, // 5
        {1, 0, 1, 1, 1, 1, 1}, // 6
        {1, 1, 1, 0, 0, 0, 0}, // 7
        {1, 1, 1, 1, 1, 1, 1}, // 8
        {1, 1, 1, 1, 0, 1, 1} // 9
    };

    if (digit < 0 || digit > 9) return;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Culoare Albă pt Scor

    // Coordonate relative
    float w = size;
    float h = size * 2; // Cifrele sunt de 2 ori mai înalte decât late

    // Desenăm segmentele active
    if (segments[digit][0]) SDL_RenderLine(renderer, x, y, x + w, y); // A (Sus)
    if (segments[digit][1]) SDL_RenderLine(renderer, x + w, y, x + w, y + h / 2); // B (Sus-Dreapta)
    if (segments[digit][2]) SDL_RenderLine(renderer, x + w, y + h / 2, x + w, y + h); // C (Jos-Dreapta)
    if (segments[digit][3]) SDL_RenderLine(renderer, x, y + h, x + w, y + h); // D (Jos)
    if (segments[digit][4]) SDL_RenderLine(renderer, x, y + h / 2, x, y + h); // E (Jos-Stânga)
    if (segments[digit][5]) SDL_RenderLine(renderer, x, y, x, y + h / 2); // F (Sus-Stânga)
    if (segments[digit][6]) SDL_RenderLine(renderer, x, y + h / 2, x + w, y + h / 2); // G (Mijloc)
}

void drawScore(SDL_Renderer *renderer, int value) {
    std::string s = std::to_string(value);
    float digitSize = 15.0f; // Cât de mare e o cifră
    float spacing = 25.0f; // Spațiu între cifre

    // Calculăm poziția de start (Dreapta Sus)
    // Plecăm de la margine (WINDOW_WIDTH) și scădem lungimea textului
    float startX = WINDOW_WIDTH - (s.length() * spacing) - 20.0f;
    float startY = 20.0f;

    for (char c: s) {
        int digit = c - '0'; // Conversie char la int
        drawDigit(renderer, digit, startX, startY, digitSize);
        startX += spacing;
    }
}

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

    spawnEnemies();
}


void processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) quit = true;

        // TRAGERE (Pe eveniment Key Down, ca să nu tragi "mitralieră" dacă ții apăsat)
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_SPACE && !event.key.repeat) {
                // Creăm un glonț (Cerc)
                Circle bullet;
                bullet.radius = 5.0f;
                // Glonțul pleacă din mijlocul jucătorului
                bullet.pos = glm::vec2(player.pos.x + player.size / 2, player.pos.y);
                bullet.color = {255, 0, 0, 255};

                bullets.push_back(bullet); // Adăugăm în vector
            } else if (event.key.key == SDLK_R && isGameOver) {
                // Resetăm variabilele
                score = 0;
                enemies.clear(); // Ștergem inamicii vechi
                bullets.clear(); // Ștergem gloanțele
                enemyBullets.clear();

                eggTimer = 0.0f;

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
                score++;
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

    // A. Spawm Ouă (Tragere)
    eggTimer += dt;
    if (eggTimer >= EGG_SPAWN_TIME) {
        eggTimer = 0; // Resetăm timerul

        // Găsim toți inamicii vii
        std::vector<int> livingEnemiesIndices;
        for (int i = 0; i < enemies.size(); i++) {
            if (enemies[i].active) {
                livingEnemiesIndices.push_back(i);
            }
        }

        // Dacă mai există inamici, alegem unul random
        if (!livingEnemiesIndices.empty()) {
            int randomIndex = livingEnemiesIndices[rand() % livingEnemiesIndices.size()];
            Square &shooter = enemies[randomIndex];

            // Creăm oul
            Circle egg;
            egg.radius = 7.0f; // Puțin mai mare ca gloanțele tale
            egg.pos = glm::vec2(shooter.pos.x + shooter.size / 2, shooter.pos.y + shooter.size);
            egg.color = {255, 255, 255, 255}; // Alb (ca un ou)

            enemyBullets.push_back(egg);
        }
    }

    // B. Mișcare Ouă și Coliziune cu Jucătorul
    for (int i = enemyBullets.size() - 1; i >= 0; i--) {
        enemyBullets[i].pos.y += EGG_SPEED * dt; // Merg în JOS (+)

        // Verificăm coliziunea cu Jucătorul (folosim funcția existentă cerc-pătrat)
        if (checkCollision(enemyBullets[i], player)) {
            isGameOver = true;
            std::cout << "GAME OVER: Te-a lovit un ou!" << std::endl;
        }

        // Ștergem oul dacă iese din ecran (jos)
        if (enemyBullets[i].pos.y - enemyBullets[i].radius > WINDOW_HEIGHT) {
            enemyBullets.erase(enemyBullets.begin() + i);
        }
    }

    int activeEnemies = 0;
    for (const auto &enemy: enemies) {
        if (enemy.active) {
            activeEnemies++;
        }
    }

    // Dacă nu mai e nimeni viu...
    if (activeEnemies == 0) {
        // 1. Facem jocul puțin mai greu
        ENEMY_MOVE_SPEED += 50.0f; // Cresc viteza

        // 2. Aducem o nouă armată
        spawnEnemies();

        // 3. Resetăm gloanțele inamice ca să fie corect (să nu te lovească un ou din nivelul trecut)
        enemyBullets.clear();
        bullets.clear();
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

    for (Circle &egg: enemyBullets) {
        egg.draw(renderer);
    }
    drawScore(renderer, score);
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
    srand(time(nullptr));
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
