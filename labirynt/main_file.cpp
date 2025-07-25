#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <random>
#include <ctime>
#include "constants.h"
#include "allmodels.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"
#include <iostream>
#include "ghost_data.h"
#include "torch_data.h"
using namespace std;

const float UNIT_SIZE = 2.0f;
const float FLOOR_HEIGHT = 2.0f * UNIT_SIZE; // Wysokość piętra

// program cieniujacy
ShaderProgram* sp;
std::vector<glm::vec4> lightPositions;
int light_count;
// Tekstury

GLuint wallTexture;
GLuint floorTexture;
GLuint ladderTexture;
GLuint ghostTexture;
GLuint torchTexture;


// Tryb widoku
bool birdViewMode = false;

// Efekt spadania - rozszerzony
bool damageEffectActive = false;
float damageEffectTimer = 0.0f;
const float DAMAGE_EFFECT_DURATION = 1.0f; // 1 sekunda efektu
float cameraShakeIntensity = 0.0f;
std::mt19937 shakeRng;

// Macierz poziomów labiryntu
// 0 = przejście, 1 = ściana, 2 = drabina w górę, 3 = drabina w dół, 4 = ściana z światłem, -1 = dziura 
std::vector<std::vector<std::vector<int>>> labirynt = {
    // Poziom 0 (parter)
    {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 4, 0, 4, 0, 4, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 4},
    {1, 0, 1, 1, 1, 4, 0, 1, 2, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 1},
    {1, 1, 1, 0, 4, 1, 1, 4, 0, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 4, 1, 1, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 4, 1, 1, 1, 1}
},
    // Poziom 1 (pierwsze piętro)
    {
        {1, 1, 1, 1, 4, 1, 1, 1, 4, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 4, 1, 0, 1, 1, 1, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 1, -1, 1},  // dziura
        {1, 0, 1, 0, 4, 1, 0, 1, 3, 1},  // 3 oznacza drabinę w dół
        {1, 0, 0, 0, 0, 0, 0, 1, -1, 1}, // dziura
        {1, 0, 1, -1, 1, 0, 4, 1, 0, 1},
        {1, 0, 0, 0, 1, 0, 0, 0, 0, 4},
        {1, -1, 1, 4, 1, 0, 1, 1, 1, 1},
        {1, 4, 1, 1, 1, 1, 1, 1, 1, 1}
    },
    //sufit
    {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    }
};

const int MAX_LEVELS = labirynt.size();
const int MAZE_HEIGHT = labirynt[0].size();
const int MAZE_WIDTH = labirynt[0][0].size();

struct Ghost {
    int gridX = 5, gridY = 5;  // początkowa pozycja
    int targetX = 5, targetY = 5;
    float worldX = 0, worldZ = 0;
    float moveProgress = 1.0f;
    float moveSpeed = 1.0f;  // wolniejszy niż gracz
    bool isMoving = false;

    int floor = 0;
    int targetFloor = 0;
    float floorProgress = 1.0f;
    bool isClimbing = false;

    float moveTimer = 0.0f;
    const float MOVE_INTERVAL = 1.0f;  // ruch co sekundę

    // zwrot
	float currentAngle = 0.0f;
	float targetAngle = 0.0f;
	float ghostfloat = 0.0f; // dodatkowy float do animacji
};

struct Player {
    int gridX = 1, gridY = 1;
    int targetX = 1, targetY = 1;
    float worldX = 0, worldZ = 0;
    float moveProgress = 1.0f;
    float moveSpeed = 3.0f;
    bool isMoving = false;

    int floor = 0;  // aktualny poziom
    int targetFloor = 0;
    float floorProgress = 1.0f;
    float climbSpeed = 2.0f;
    bool isClimbing = false;
    bool isFalling = false;  // nowy stan spadania

    float currentAngle = 0.0f;
    float targetAngle = 0.0f;
    float rotationProgress = 1.0f;
    float rotationSpeed = 5.0f;
    bool isRotating = false;
};

Player player;
Ghost ghost;

// Funkcja do wczytywania tekstur
GLuint readTexture(const char* filename) {
    GLuint tex;
    glActiveTexture(GL_TEXTURE0);

    std::vector<unsigned char> image;
    unsigned width, height;

    unsigned error = lodepng::decode(image, width, height, filename);

    if (error) {
        printf("Błąd wczytywania tekstury %s: %s\n", filename, lodepng_error_text(error));
        // Tworzymy białą teksturę jako fallback
        image.resize(4);
        image[0] = image[1] = image[2] = image[3] = 255;
        width = height = 1;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return tex;
}

void gridToWorld(int gx, int gy, float& wx, float& wz) {
    wx = gx * UNIT_SIZE - MAZE_WIDTH * UNIT_SIZE / 2.0f + UNIT_SIZE / 2.0f;
    wz = gy * UNIT_SIZE - MAZE_HEIGHT * UNIT_SIZE / 2.0f + UNIT_SIZE / 2.0f;
}

bool isPositionValid(int gx, int gy, int level) {
    if (level < 0 || level >= MAX_LEVELS) return false;
    if (gy < 0 || gy >= MAZE_HEIGHT || gx < 0 || gx >= MAZE_WIDTH) return false;
    int cellType = labirynt[level][gy][gx];
    return cellType == 0 || cellType == -1; // można stać na pustym polu lub dziurze (spadnie później)
}

bool isHole(int gx, int gy, int level) {
    if (level < 0 || level >= MAX_LEVELS) return false;
    if (gy < 0 || gy >= MAZE_HEIGHT || gx < 0 || gx >= MAZE_WIDTH) return false;
    return labirynt[level][gy][gx] == -1;
}

int getCellType(int gx, int gy, int level) {
    if (level < 0 || level >= MAX_LEVELS) return 1; // ściana poza granicami
    if (gy < 0 || gy >= MAZE_HEIGHT || gx < 0 || gx >= MAZE_WIDTH) return 1; // ściana poza granicami
    return labirynt[level][gy][gx];
}

void startFall() {
    if (player.floor > 0) {
        player.targetFloor = player.floor - 1;
        player.floorProgress = 0.0f;
        player.isFalling = true;
        player.isClimbing = true; // używamy tej samej mechaniki, ale szybciej
        player.climbSpeed = 5.0f; // szybsze spadanie

        // Aktywacja efektu obrażeń
        damageEffectActive = true;
        damageEffectTimer = 0.0f;
        cameraShakeIntensity = 0.3f; // początkowa intensywność trzęsienia

        // Inicjalizacja generatora liczb losowych
        shakeRng.seed(static_cast<unsigned>(time(nullptr)));
    }
}

void checkForFall() {
    // Sprawdź czy gracz stoi na dziurze (ale najpierw sprawdź czy nie ma drabiny)
    int currentCellType = getCellType(player.gridX, player.gridY, player.floor);

    // Jeśli jest drabina, ignoruj dziurę
    if (currentCellType == 2 || currentCellType == 3) {
        return;
    }

    // Jeśli stoi na dziurze i nie ma drabiny, spadaj
    if (currentCellType == -1) {
        startFall();
    }
}

void startMove(int angle) {
    if (player.isMoving || player.isRotating || player.isClimbing || birdViewMode) return;

    int nx = player.gridX;
    int ny = player.gridY;
    switch (angle)
    {
    case 0:
        ny -= 1;
        break;
    case 90:
        nx -= 1;
        break;
    case 180:
        ny += 1;
        break;
    case 270:
        nx += 1;
        break;
    }

    if (isPositionValid(nx, ny, player.floor)) {
        player.targetX = nx;
        player.targetY = ny;
        player.moveProgress = 0.0f;
        player.isMoving = true;
    }
    else {
        // Sprawdź czy próbujemy wejść w drabinę
        int cellType = getCellType(nx, ny, player.floor);
        if (cellType == 2 && player.floor + 1 < MAX_LEVELS) {
            // Próba wejścia w drabinę w górę
            player.targetFloor = player.floor + 1;
            player.floorProgress = 0.0f;
            player.isClimbing = true;
            player.isFalling = false;
            player.climbSpeed = 2.0f; // normalna prędkość wspinaczki
        }
        else if (cellType == 3 && player.floor - 1 >= 0) {
            // Próba wejścia w drabinę w dół
            player.targetFloor = player.floor - 1;
            player.floorProgress = 0.0f;
            player.isClimbing = true;
            player.isFalling = false;
            player.climbSpeed = 2.0f; // normalna prędkość wspinaczki
        }
    }
}

void startRotation(bool right) {
    if (player.isMoving || player.isRotating || player.isClimbing || birdViewMode) return;

    if (right) {
        player.targetAngle = player.currentAngle - 90.0f;
    }
    else {
        player.targetAngle = player.currentAngle + 90.0f;
    }

    player.rotationProgress = 0.0f;
    player.isRotating = true;
}

float normalizeAngle(float angle) {
    while (angle < 0.0f) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;
    return angle;
}

// Funkcja generująca losowy offset dla trzęsienia kamery
glm::vec3 getCameraShakeOffset() {
    if (!damageEffectActive || cameraShakeIntensity <= 0.0f) {
        return glm::vec3(0.0f);
    }

    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    float shakeX = dist(shakeRng) * cameraShakeIntensity;
    float shakeY = dist(shakeRng) * cameraShakeIntensity;
    float shakeZ = dist(shakeRng) * cameraShakeIntensity;

    return glm::vec3(shakeX, shakeY, shakeZ);
}

// Funkcja sprawdzająca czy pozycja jest dostępna dla ducha (może przechodzić przez ściany)
bool isGhostPositionValid(int gx, int gy, int level) {
    if (level < 0 || level >= MAX_LEVELS) return false;
    if (gy < 0 || gy >= MAZE_HEIGHT || gx < 0 || gx >= MAZE_WIDTH) return false;
	if (labirynt[level][gy][gx] != -1 && labirynt[level][gy][gx] != 0) return false;
    return true;  // duch może być wszędzie w granicach labiryntu
}

// Funkcja losowego ruchu ducha
void moveGhostRandomly() {
    const int directions[4][2] = { {0, -1}, {-1, 0}, {0, 1}, {1, 0} };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dirDist(0, 3);
    std::uniform_int_distribution<> floorDist(0, MAX_LEVELS - 1);

    // 20% szans na zmianę piętra
    //std::uniform_int_distribution<> floorChance(0, 4);
    //if (floorChance(gen) == 0) {
    //    int newFloor = floorDist(gen);
    //    if (newFloor != ghost.floor) {
    //        ghost.targetFloor = newFloor;
    //        ghost.floorProgress = 0.0f;
    //        ghost.isClimbing = true;
    //        return;
    //    }
    //}

    // Wybierz losowy kierunek
    int dir = dirDist(gen);
    int nx = ghost.gridX + directions[dir][0];
    int ny = ghost.gridY + directions[dir][1];
    
    if (isGhostPositionValid(nx, ny, ghost.floor)) {
        ghost.targetX = nx;
        ghost.targetY = ny;
        ghost.moveProgress = 0.0f;
        ghost.isMoving = true;
        if (directions[dir][0] == -1 && directions[dir][1] == 0) {
			ghost.targetAngle = 1.5f * PI;
        }
        else if (directions[dir][0] == 1 && directions[dir][1] == 0) {
			ghost.targetAngle = 0.5f * PI;
		}
        else if (directions[dir][0] == 0 && directions[dir][1] == -1) {
			ghost.targetAngle = PI;
        }
        else {
			ghost.targetAngle = 0;

        }
    }

}

// Funkcja sprawdzająca kolizję z graczem
void checkGhostPlayerCollision() {
    if (ghost.floor == player.floor &&
        ghost.gridX == player.gridX &&
        ghost.gridY == player.gridY) {

        // Przywróć gracza na start
        player.gridX = 1;
        player.gridY = 1;
        player.targetX = 1;
        player.targetY = 1;
        player.floor = 0;
        player.targetFloor = 0;
        player.moveProgress = 1.0f;
        player.floorProgress = 1.0f;
        player.isMoving = false;
        player.isClimbing = false;
        player.isFalling = false;

		// przywróć pozycję ducha
		ghost.gridX = 5;
		ghost.gridY = 5;
		ghost.targetX = 5;
		ghost.targetY = 5;
		ghost.floor = 0;
		ghost.targetFloor = 0;
		ghost.moveProgress = 1.0f;
		ghost.floorProgress = 1.0f;
		ghost.isMoving = false;
		ghost.isClimbing = false;
		ghost.moveTimer = 0.0f; // resetuj timer ruchu ducha


        // Aktywuj efekt obrażeń
        damageEffectActive = true;
        damageEffectTimer = 0.0f;
        cameraShakeIntensity = 0.5f;

        // Przelicz pozycję gracza w świecie
        gridToWorld(player.gridX, player.gridY, player.worldX, player.worldZ);
    }
}

void update(float dt) {
    // Aktualizacja efektu obrażeń
    if (damageEffectActive) {
        damageEffectTimer += dt;

        // Zmniejszanie intensywności trzęsienia z czasem
        float timeRatio = damageEffectTimer / DAMAGE_EFFECT_DURATION;
        cameraShakeIntensity = 0.3f * (1.0f - timeRatio); // od 0.3 do 0

        if (damageEffectTimer >= DAMAGE_EFFECT_DURATION) {
            damageEffectActive = false;
            damageEffectTimer = 0.0f;
            cameraShakeIntensity = 0.0f;
        }
    }

    if (player.isMoving) {
        player.moveProgress += player.moveSpeed * dt;
        if (player.moveProgress >= 1.0f) {
            player.moveProgress = 1.0f;
            player.isMoving = false;
            player.gridX = player.targetX;
            player.gridY = player.targetY;

            // Po zakończeniu ruchu sprawdź czy gracz nie stoi na dziurze
            checkForFall();
        }
    }

    if (player.isRotating) {
        player.rotationProgress += player.rotationSpeed * dt;
        if (player.rotationProgress >= 1.0f) {
            player.rotationProgress = 1.0f;
            player.isRotating = false;
            player.currentAngle = normalizeAngle(player.targetAngle);
        }
    }

    if (player.isClimbing) {
        player.floorProgress += player.climbSpeed * dt;
        if (player.floorProgress >= 1.0f) {
            player.floorProgress = 1.0f;
            player.isClimbing = false;
            player.floor = player.targetFloor;
            player.isFalling = false;
        }
    }



    float sx, sz, tx, tz;
    gridToWorld(player.gridX, player.gridY, sx, sz);
    gridToWorld(player.targetX, player.targetY, tx, tz);
    float t = player.moveProgress;
    float smooth = t * t * (3 - 2 * t);
    player.worldX = sx + (tx - sx) * smooth;
    player.worldZ = sz + (tz - sz) * smooth;

    // Aktualizacja ducha
    ghost.moveTimer += dt;
    if (ghost.moveTimer >= ghost.MOVE_INTERVAL && !ghost.isMoving && !ghost.isClimbing) {
        ghost.moveTimer = 0.0f;
        moveGhostRandomly();
    }

    if (ghost.isMoving) {
        ghost.moveProgress += ghost.moveSpeed * dt;
        if (ghost.moveProgress >= 1.0f) {
            ghost.moveProgress = 1.0f;
            ghost.isMoving = false;
            ghost.gridX = ghost.targetX;
            ghost.gridY = ghost.targetY;
        }
    }

    if (ghost.isClimbing) {
        ghost.floorProgress += ghost.moveSpeed * dt;
        if (ghost.floorProgress >= 1.0f) {
            ghost.floorProgress = 1.0f;
            ghost.isClimbing = false;
            ghost.floor = ghost.targetFloor;
        }
    }

    // Aktualizacja pozycji ducha w świecie
    float gsx, gsz, gtx, gtz;
    gridToWorld(ghost.gridX, ghost.gridY, gsx, gsz);
    gridToWorld(ghost.targetX, ghost.targetY, gtx, gtz);
    float gt = ghost.moveProgress;
    float gsmooth = gt * gt * (3 - 2 * gt);
    ghost.worldX = gsx + (gtx - gsx) * gsmooth;
    ghost.worldZ = gsz + (gtz - gsz) * gsmooth;

    // Sprawdź kolizję z graczem
    checkGhostPlayerCollision();
}

// Funkcja renderująca czerwoną nakładkę na cały ekran
void renderDamageOverlay() {
    if (!damageEffectActive) return;

    // Zapisz bieżący stan OpenGL
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Przełącz na macierze ortograficzne dla 2D overlay
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Oblicz intensywność czerwieni na podstawie czasu i trzęsienia
    float timeRatio = damageEffectTimer / DAMAGE_EFFECT_DURATION;
    float flashIntensity = (1.0f - timeRatio) * 0.6f; // od 0.6 do 0

    // Dodaj efekt pulsowania
    float pulseEffect = sin(damageEffectTimer * 15.0f) * 0.2f + 0.8f;
    flashIntensity *= pulseEffect;

    // Renderuj czerwony quad na cały ekran
    glColor4f(1.0f, 0.0f, 0.0f, flashIntensity);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(1.0f, 0.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(0.0f, 1.0f);
    glEnd();

    // Przywróć poprzednie macierze
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    // Przywróć poprzedni stan OpenGL
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // przywróć biały kolor
}

// funkcja znajdujace wszystkie swiatla dla jednego obiektu

void findlight(int level, int cubes_row, int cubes_column) {

    const int dir1[6][3] = { {0, 0, 0}, { 0, 1, 0 }, {0, -1, 0}, {-1, 0, 0}, {1, 0, 0}, {0, 0, -1} };
    const int dir[5][3] = { { 0, 1, 0}, {0, -1, 0}, {-1, 0, 0}, {1, 0, 0}, { 0, 0, -1} };
	//const int kazdy_kierunek[8][3] = { { 0, 1, 0 }, { 0, -1, 0 }, { -1, 0, 0 }, { 1, 0, 0 }, {1, 1, 0}, {1, -1, 0}, {-1, 1, 0}, {-1, -1, 0} };
    const int rozmiar = sizeof(dir) / sizeof(dir[0]); 
	const int rozmiar1 = sizeof(dir1) / sizeof(dir1[0]);
	//const int rozmiar_kazdy_kierunek = sizeof(kazdy_kierunek) / sizeof(kazdy_kierunek[0]);

    for (int d = 0; d < rozmiar1; ++d) {
        int nx = cubes_column + dir1[d][0];
        int ny = cubes_row + dir1[d][1];
        int nz = level + dir1[d][2];
        // Sprawdź, czy przy ścianie jest pole przejściowe
        bool source = false;
        
        
		if (nx >= 0 && nx < MAZE_WIDTH && ny >= 0 && ny < MAZE_HEIGHT && nz >= 0 && nz < MAX_LEVELS &&
            (labirynt[nz][ny][nx] != 1 && labirynt[nz][ny][nx] != 4 )) {
            
            for (int i = 0; i < rozmiar1; ++i) {
                int tx = nx + dir1[i][0];
                int ty = ny + dir1[i][1];
                int tz = nz;
                if (nz != 0) {
                    tz = nz + dir1[i][2];
                }
                
                if (labirynt[tz][ty][tx] == 4) {
                    // Jeśli pole przejściowe jest obok ściany, to jest źródło światła
                    source = true;
                    break;
                }
            }
            if (source) {
                
                // Jeśli znaleziono światło w ścianie, dodaj je do lightPositions
                glm::vec4 newLight(
                    nx * UNIT_SIZE + UNIT_SIZE / 2.0f - MAZE_WIDTH * UNIT_SIZE / 2.0f,
                    nz * FLOOR_HEIGHT + UNIT_SIZE/2.0f -0.4,
                    ny * UNIT_SIZE + UNIT_SIZE / 2.0f - MAZE_HEIGHT * UNIT_SIZE / 2.0f,
                    1.0f
                );

                bool alreadyExists = false;
                const float epsilon = 0.01f;
                for (const auto& lp : lightPositions) {
                    if (glm::length(lp - newLight) < epsilon) {
                        alreadyExists = true;
                        break;
                    }
                }
                if (!alreadyExists) {
                    lightPositions.push_back(newLight);
                    ++light_count;
                }
            }
            // Od tego pola (przejście) próbuj iść w każdym kierunku
            for (int i = 0; i < rozmiar; ++i) {
                int x = nx;
                int y = ny;
                int z = nz;
                
                while (true) {
                    x += dir[i][0];
                    y += dir[i][1];
					z += dir[i][2];
					
                    // Wyjście poza labirynt
                    if (x < 0 || x >= MAZE_WIDTH || y < 0 || y >= MAZE_HEIGHT || z < 0) 
                        break;
                        

                    // Zakończ, jeśli natrafimy na coś innego niż przejście
                    int cell = labirynt[z][y][x];
                    if (cell == 1 || cell == 4) break;

					// Sprawdź sąsianie komórki
                    for (int j = 0; j < rozmiar; ++j) {
                        int sx = x + dir[j][0];
                        int sy = y + dir[j][1];
						int sz = z + dir[j][2];

                        if (sx < 0 || sx >= MAZE_WIDTH || sy < 0 || sy >= MAZE_HEIGHT || sz < 0)
                            continue;

                        if (labirynt[sz][sy][sx] == 4) {
                            
                            // Znalazłeś światło w ścianie obok ścieżki
                            // Przed dodaniem nowego światła sprawdź, czy już istnieje takie światło w lightPositions
                            glm::vec4 newLight(
                                x * UNIT_SIZE + UNIT_SIZE / 2.0f - MAZE_WIDTH * UNIT_SIZE / 2.0f,
                                z * FLOOR_HEIGHT + UNIT_SIZE/2.0f - 0.4,
                                y * UNIT_SIZE + UNIT_SIZE / 2.0f - MAZE_HEIGHT * UNIT_SIZE / 2.0f,
                                1.0f
                            );

							/*cout << "Znaleziono światło na poziomie " << level << " w pozycji (" << sx << ", " << sy << ")" << endl;
							cout << sx + x << " " << sy + y << endl;
							cout << sx << " " << sy << endl;
							cout << x << " " << y << endl;*/

                            // Sprawdź, czy już istnieje takie światło (porównanie pozycji z tolerancją)
                            bool alreadyExists = false;
                            const float epsilon = 0.01f;
                            for (const auto& lp : lightPositions) {
                                if (glm::length(lp - newLight) < epsilon) {
                                    alreadyExists = true;
                                    break;
                                }
                            }
                            if (!alreadyExists) {
                                lightPositions.push_back(newLight);
                                ++light_count;
                            }
                        }
                    }
                }
            }
        }
    }
}



// Funkcja renderująca pojedynczy sześcian z teksturą
void renderCube(const glm::mat4& P, const glm::mat4& V, float x, float y, float z, GLuint texture, int level, int row, int column, float scaleX = 1.0f, float scaleY = 1.0f, float scaleZ = 1.0f) {
    glm::mat4 M = glm::mat4(1.0f);
    M = glm::translate(M, glm::vec3(x, y, z));
    M = glm::scale(M, glm::vec3(UNIT_SIZE / 2.0f * scaleX, UNIT_SIZE / 2.0f * scaleY, UNIT_SIZE / 2.0f * scaleZ));
    
    sp->use();
    findlight(level, row, column); // Znajdź światła dla tego poziomu i pozycji
    /*lightPositions.push_back(glm::vec4(0, 0, 0, 1)); 
    light_count++;*/
    
  //  if ((light_count > 0) && ((level == 2 && row == 2 && column == 2) )) {
	 //   cout << "===============================" << endl;
  //      std::cout << "Pozycja srodka cube'a w world space: ("
  //          << x << ", "
  //          << y << ", "
  //          << z << ")" << std::endl;
	 //   	//// Debugowanie pozycji światła
  //      for (int i = 0; i < light_count; ++i) {
  //          std::cout << "Pozycja swiatla " << i << ": ("
  //              << lightPositions[i].x << ", "
  //              << lightPositions[i].y << ", "
  //              << lightPositions[i].z << ", "
  //              << lightPositions[i].w << ")\n";
		//	// odleglosc obiektu od swiatla
  //          float distance = glm::length(glm::vec3(lightPositions[i]) - glm::vec3(x, y, z));
		//	std::cout << "Odleglosc od swiatla " << i << ": " << distance << std::endl;
		//}
  //      cout << "Znaleziono " << light_count << " swiatel na poziomie " << level << " w pozycji (" << row << ", " << column << ")" << endl;
  //      glm::vec4 camera_model_pos = inverse(V) * glm::vec4(0, 0, 0, 1);
  //  
  //      std::cout << "Pozycja kamery w przestrzeni swiata: ("
  //          << camera_model_pos.x << ", "
  //          << camera_model_pos.y << ", "
  //          << camera_model_pos.z << ")\n";
  //       

  //      cout << "===============================" << endl;
  //  } 
    
	
    glUniform1i(sp->u("numLights"), light_count);

    // Wysyłanie pozycji świateł jako vec4[]
    if (light_count > 0) {
        glUniform4fv(sp->u("lp"), light_count, glm::value_ptr(lightPositions[0]));
    }
  

    glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));
    glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));


    //glUniform4f(sp->u("lp"), 0, 0, 0, 1);
    glEnableVertexAttribArray(sp->a("vertex"));
    glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices);

    glEnableVertexAttribArray(sp->a("normal"));
    glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, myCubeNormals);

    glEnableVertexAttribArray(sp->a("texCoord0"));
    glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, myCubeTexCoords);

    glEnableVertexAttribArray(sp->a("color"));  //Włącz przesyłanie danych do atrybutu color
    glVertexAttribPointer(sp->a("color"), 4, GL_FLOAT, false, 0, myCubeColors); //Wskaż tablicę z danymi dla atrybutu color



    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(sp->u("tex"), 0);

    glDrawArrays(GL_TRIANGLES, 0, myCubeVertexCount);

    glDisableVertexAttribArray(sp->a("vertex"));
    glDisableVertexAttribArray(sp->a("normal"));
    glDisableVertexAttribArray(sp->a("texCoord0"));
    lightPositions.clear();
    light_count = 0;
    
}

// Funkcja renderująca ducha
void renderGhost(const glm::mat4& P, const glm::mat4& V, float x, float y, float z, GLuint texture, int level, int row, int column, float rotation = 0.0f) {
    glm::mat4 M = glm::mat4(1.0f);
    M = glm::translate(M, glm::vec3(x, y, z));
	M = glm::rotate(M, rotation, glm::vec3(0.0f, 1.0f, 0.0f)); 
    M = glm::scale(M, glm::vec3(0.007f *UNIT_SIZE / 2.0f, 0.007f * UNIT_SIZE / 2.0f, 0.007f * UNIT_SIZE / 2.0f));

    sp->use();
    findlight(level, row, column);

    glUniform1i(sp->u("numLights"), light_count);

    if (light_count > 0) {
        glUniform4fv(sp->u("lp"), light_count, glm::value_ptr(lightPositions[0]));
    }

    glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));
    glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));

    // Użyj danych ducha zamiast kostki
    glEnableVertexAttribArray(sp->a("vertex"));
    glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, myGhostVertices);

    glEnableVertexAttribArray(sp->a("normal"));
    glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, myGhostNormals);

    glEnableVertexAttribArray(sp->a("texCoord0"));
    glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, myGhostTexCoords);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(sp->u("tex"), 0);

    glDrawArrays(GL_TRIANGLES, 0, myGhostVertexCount);

    glDisableVertexAttribArray(sp->a("vertex"));
    glDisableVertexAttribArray(sp->a("normal"));
    glDisableVertexAttribArray(sp->a("texCoord0"));

    lightPositions.clear();
    light_count = 0;
}

void renderTorch(const glm::mat4& P, const glm::mat4& V, float x, float y, float z, float rotX, float rotY, GLuint texture) {
    glm::mat4 M = glm::mat4(1.0f);
    M = glm::translate(M, glm::vec3(x, y, z));
	float rotZ = 90.0f; // Obrót w osi Z, aby pochodnia była pionowo
    M = glm::rotate(M, glm::radians(rotZ), glm::vec3(0.0f, 0.0f, 1.0f));
    M = glm::rotate(M, glm::radians(rotX), glm::vec3(1.0f, 0.0f, 0.0f));
    M = glm::rotate(M, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
    

    M = glm::scale(M, glm::vec3(0.5f, 0.5f, 0.5f)); // Skala pochodni

    sp->use();
    glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));
    glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));

    glEnableVertexAttribArray(sp->a("vertex"));
    glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, myTorchVertices);

    glEnableVertexAttribArray(sp->a("normal"));
    glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, myTorchNormals);

    glEnableVertexAttribArray(sp->a("texCoord0"));
    glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, myTorchTexCoords);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(sp->u("tex"), 0);

    glDrawArrays(GL_TRIANGLES, 0, myTorchVertexCount);

    glDisableVertexAttribArray(sp->a("vertex"));
    glDisableVertexAttribArray(sp->a("normal"));
    glDisableVertexAttribArray(sp->a("texCoord0"));
}


// Funkcja renderująca labirynt ze wszystkimi poziomami
void renderMaze(const glm::mat4& P, const glm::mat4& V) {
    float offsetX = -(MAZE_WIDTH * UNIT_SIZE) / 2.0f + UNIT_SIZE / 2.0f;
    float offsetZ = -(MAZE_HEIGHT * UNIT_SIZE) / 2.0f + UNIT_SIZE / 2.0f;
    // Renderuj wszystkie poziomy
    for (int level = 0; level < MAX_LEVELS; level++) {
        float levelHeight = level * FLOOR_HEIGHT;

        for (int row = 0; row < MAZE_HEIGHT; row++) {
            for (int col = 0; col < MAZE_WIDTH; col++) {
                float x = col * UNIT_SIZE + offsetX;
                float z = row * UNIT_SIZE + offsetZ;
                int cellType = labirynt[level][row][col];
                
				
                switch (cellType) {
                case 1: // Ściana
                    
                    renderCube(P, V, x, levelHeight, z, wallTexture, level, row, col, 1.0f, 2.0f, 1.0f);
                    // Podłoga pod ścianą
                    renderCube(P, V, x, levelHeight - UNIT_SIZE, z, floorTexture, level, row, col);
                    break;

                case 2: // Drabina w górę
                    renderCube(P, V, x, levelHeight, z, ladderTexture, level, row, col, 1.0f, 2.0f, 1.0f);
                    // Podłoga pod drabiną
                    renderCube(P, V, x, levelHeight - UNIT_SIZE, z, floorTexture, level, row, col);
                    break;

                case 3: // Drabina w dół
                    renderCube(P, V, x, levelHeight, z, ladderTexture, level, row, col, 1.0f, 2.0f, 1.0f);
                    // Podłoga pod drabiną
                    renderCube(P, V, x, levelHeight - UNIT_SIZE, z, floorTexture, level, row, col);
                    break;

                case -1: // Dziura - nie renderuj nic!
                    // Celowo puste - dziura to brak geometrii
                    // Sprawdź czy nie ma drabiny na tym samym miejscu na wyższym poziomie
                    if (level + 1 < MAX_LEVELS) {
                        int upperCellType = labirynt[level + 1][row][col];
                        if (upperCellType == 3) { // drabina w dół z wyższego poziomu
                            renderCube(P, V, x, levelHeight, z, ladderTexture, 1.0f, 2.0f, 1.0f);
                            // Ale nie renderuj podłogi - to wciąż dziura
                        }
                    }
                    break;

                case 4: // swiatlo
                    renderCube(P, V, x, levelHeight, z, wallTexture, level, row, col, 1.0f, 2.0f, 1.0f);
                    // Podłoga pod ścianą
                    renderCube(P, V, x, levelHeight - UNIT_SIZE, z, floorTexture, level, row, col);

                    renderTorch(P, V, x, levelHeight + UNIT_SIZE * 0.3f, z - UNIT_SIZE * 0.55f, 0, 90, torchTexture); 
                    renderTorch(P, V, x - UNIT_SIZE * 0.55f, levelHeight + UNIT_SIZE * 0.3f, z, 0, 90, torchTexture); 
                    renderTorch(P, V, x, levelHeight + UNIT_SIZE * 0.3f, z + UNIT_SIZE * 0.55f, 0, 90, torchTexture); 
                    renderTorch(P, V, x + UNIT_SIZE * 0.55f, levelHeight + UNIT_SIZE * 0.3f, z, 0, 90, torchTexture); 
                    break;
                case 0: // Puste pole
                default:
                    // Tylko podłoga
                    renderCube(P, V, x, levelHeight - UNIT_SIZE, z, floorTexture, level, row, col);
                    break;
                }
            }
        }
    }
    float ghostHeight = ghost.floor * FLOOR_HEIGHT;
    if (ghost.isClimbing) {
        float climbT = ghost.floorProgress;
        float smoothClimbT = climbT * climbT * (3 - 2 * climbT);
        float targetHeight = ghost.targetFloor * FLOOR_HEIGHT;
        ghostHeight = ghostHeight + (targetHeight - ghostHeight) * smoothClimbT;
		float rotation = ghost.targetAngle;
    }
	// czy może dojść do błędu przy obracaniu ducha? (chyba samo będzie się poprawiać)
    if (ghost.targetAngle != ghost.currentAngle){
		ghost.currentAngle += (ghost.targetAngle - ghost.currentAngle) * ghost.moveProgress/3;
	}
    // rusza sie w gore dol
    if (ghost.ghostfloat == 2 * PI) {
        ghost.ghostfloat = 0;
    }
    else {
        ghost.ghostfloat += ghost.moveProgress * 0.025f * PI; // Zmniejsz prędkość unoszenia
    }
    renderGhost(P, V, ghost.worldX, ghostHeight - UNIT_SIZE/4 + 0.25 * sin(ghost.ghostfloat), ghost.worldZ, ghostTexture, ghost.floor, ghost.gridY, ghost.gridX, ghost.currentAngle);
    //renderGhost(P, V, ghost.worldX, ghostHeight, ghost.worldZ, ghostTexture, ghost.floor, ghost.gridY, ghost.gridX);
    
}

void drawScene() {
    // Normalny kolor tła - nie czerwony
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 P, V;

    if (birdViewMode) {
        float mazeWidth = MAZE_WIDTH * UNIT_SIZE;
        float mazeHeight = MAZE_HEIGHT * UNIT_SIZE;
        float maxDimension = std::max(mazeWidth, mazeHeight);
        float cameraHeight = maxDimension * 1.5f + MAX_LEVELS * FLOOR_HEIGHT;

        glm::vec3 shakeOffset = getCameraShakeOffset();

        V = glm::lookAt(
            glm::vec3(shakeOffset.x, cameraHeight + shakeOffset.y, shakeOffset.z),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -1.0f)
        );

        P = glm::perspective(
            glm::radians(60.0f),
            1.0f,
            1.0f,
            cameraHeight * 2.0f
        );
    }
    else {
        // Widok gracza z uwzględnieniem piętra i wspinaczki/spadania
        float rotT = player.rotationProgress;
        float smoothRotT = rotT * rotT * (3 - 2 * rotT);
        float viewAngle = player.currentAngle + (player.targetAngle - player.currentAngle) * smoothRotT;

        // Wysokość kamery zależna od piętra i postępu wspinaczki/spadania
        float baseHeight = player.floor * FLOOR_HEIGHT + UNIT_SIZE * 0.3f;
        if (player.isClimbing) {
            float climbT = player.floorProgress;
            float smoothClimbT;

            if (player.isFalling) {
                // Podczas spadania użyj innej krzywej dla bardziej gwałtownego efektu
                smoothClimbT = climbT * climbT;
            }
            else {
                smoothClimbT = climbT * climbT * (3 - 2 * climbT);
            }

            float targetHeight = player.targetFloor * FLOOR_HEIGHT + UNIT_SIZE * 0.3f;
            baseHeight = baseHeight + (targetHeight - baseHeight) * smoothClimbT;
        }

        // Dodaj trzęsienie kamery
        glm::vec3 shakeOffset = getCameraShakeOffset();
        glm::vec3 pos(player.worldX + shakeOffset.x, baseHeight + shakeOffset.y, player.worldZ + shakeOffset.z);


        float radians = glm::radians(viewAngle);
        glm::vec3 dir(-sin(radians), 0, -cos(radians));

        V = glm::lookAt(pos, pos + dir, glm::vec3(0, 1, 0));
        P = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 50.0f);
    }

    renderMaze(P, V);

    // Renderuj czerwoną nakładkę obrażeń na końcu
    renderDamageOverlay();
}

void key_callback(GLFWwindow* win, int key, int, int action, int) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_P) {
            birdViewMode = true;
        }
        else if (key == GLFW_KEY_W) {
            float angle = player.currentAngle;
            startMove(angle);
        }
        else if (key == GLFW_KEY_A) startRotation(false);
        else if (key == GLFW_KEY_D) startRotation(true);
        else if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, true);
    }
    else if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_P) {
            birdViewMode = false;
        }
    }
}

void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

void initOpenGLProgram(GLFWwindow* window) {
    initShaders();

    glClearColor(0.2f, 0.3f, 0.8f, 1);
    glEnable(GL_DEPTH_TEST);
    glfwSetKeyCallback(window, key_callback);

    // Wczytaj tekstury
    wallTexture = readTexture("bricks.png");
    floorTexture = readTexture("floor.png");
    ladderTexture = readTexture("ladder.png");
    ghostTexture = readTexture("duch.png");
    torchTexture = readTexture("pochodnia1.png");

    sp = new ShaderProgram("v_simplest.glsl", NULL, "f_simplest.glsl");
}

void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();
    glDeleteTextures(1, &wallTexture);
    glDeleteTextures(1, &floorTexture);
    glDeleteTextures(1, &ladderTexture);
    glDeleteTextures(1, &ghostTexture);
    glDeleteTextures(1, &torchTexture);

    delete sp;
}

int main() {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        fprintf(stderr, "Nie można zainicjować GLFW.\n");
        exit(EXIT_FAILURE);
    }

    GLFWwindow* window = glfwCreateWindow(800, 800, "Labirynt 3D - Wielopoziomowy", NULL, NULL);

    if (!window) {
        fprintf(stderr, "Nie można utworzyć okna.\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Nie można zainicjować GLEW.\n");
        exit(EXIT_FAILURE);
    }

    initOpenGLProgram(window);
    gridToWorld(player.gridX, player.gridY, player.worldX, player.worldZ);
    gridToWorld(ghost.gridX, ghost.gridY, ghost.worldX, ghost.worldZ);
    glfwSetTime(0);

    while (!glfwWindowShouldClose(window)) {
        float deltaTime = glfwGetTime();
        glfwSetTime(0);

        update(deltaTime);
        drawScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    freeOpenGLProgram(window);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}