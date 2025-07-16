#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <thread>
#include <chrono>
#include <conio.h>
#include <windows.h>

using namespace std;

const int MAP_SIZE = 50;
const int VIEW_SIZE = 20;
const int NUM_MAPS = 5;
const int TOTAL_HIDDEN = 15;
const int TIME_BONUS = 40; // Ahora 40 segundos por T

struct Position {
    int x, y;
};

struct GameObject {
    Position pos;
    bool collected = false;
};

struct Map {
    char grid[MAP_SIZE][MAP_SIZE];
    Position player;
    GameObject key;
    vector<GameObject> hiddenItems;
    vector<GameObject> visibleItems;
    bool unlocked = false;
};

// Función para limpiar la pantalla
void clearScreen() {
    system("cls");
}

// Función para configurar el cursor en la consola
void setCursorPosition(int x, int y) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { static_cast<SHORT>(y), static_cast<SHORT>(x) };
    SetConsoleCursorPosition(hConsole, pos);
}

// Función para generar un laberinto
void generateMaze(Map& map) {
    // Inicializar con muros
    for (int i = 0; i < MAP_SIZE; ++i) {
        for (int j = 0; j < MAP_SIZE; ++j) {
            map.grid[i][j] = '#';
        }
    }

    // Crear caminos usando DFS
    srand(time(nullptr));
    vector<Position> stack;
    Position start = { 1, 1 };
    map.grid[start.x][start.y] = ' ';
    stack.push_back(start);

    int dx[] = { -2, 2, 0, 0 };
    int dy[] = { 0, 0, -2, 2 };

    while (!stack.empty()) {
        Position current = stack.back();
        stack.pop_back();

        vector<int> directions = { 0, 1, 2, 3 };
        for (int i = 0; i < 4; ++i) {
            int idx = rand() % directions.size();
            int dir = directions[idx];
            directions.erase(directions.begin() + idx);

            int nx = current.x + dx[dir];
            int ny = current.y + dy[dir];

            if (nx > 0 && nx < MAP_SIZE - 1 && ny > 0 && ny < MAP_SIZE - 1 && map.grid[nx][ny] == '#') {
                map.grid[nx][ny] = ' ';
                map.grid[current.x + dx[dir] / 2][current.y + dy[dir] / 2] = ' ';
                stack.push_back(current);
                stack.push_back({ nx, ny });
                break;
            }
        }
    }
}

// Función para inicializar un mapa
void initializeMap(Map& map, int mapIndex) {
    generateMaze(map);
    map.player = { 1, 1 };

    // Colocar llave
    do {
        map.key.pos = { rand() % (MAP_SIZE - 2) + 1, rand() % (MAP_SIZE - 2) + 1 };
    } while (map.grid[map.key.pos.x][map.key.pos.y] != ' ');
    map.key.collected = false;
    map.grid[map.key.pos.x][map.key.pos.y] = 'P';

    // Colocar objetos ocultos (3 por mapa)
    for (int i = 0; i < 3; ++i) {
        GameObject item;
        do {
            item.pos = { rand() % (MAP_SIZE - 2) + 1, rand() % (MAP_SIZE - 2) + 1 };
        } while (map.grid[item.pos.x][item.pos.y] != ' ');
        map.hiddenItems.push_back(item);
    }

    // Colocar objetos visibles (T) - 1 por mapa
    for (int i = 0; i < 1; ++i) {
        GameObject item;
        do {
            item.pos = { rand() % (MAP_SIZE - 2) + 1, rand() % (MAP_SIZE - 2) + 1 };
        } while (map.grid[item.pos.x][item.pos.y] != ' ');
        map.visibleItems.push_back(item);
        map.grid[item.pos.x][item.pos.y] = 'T';
    }

    // El primer mapa está desbloqueado por defecto
    if (mapIndex == 0) {
        map.unlocked = true;
    }
}

// Función para mostrar el mapa en la vista
void displayMap(Map& map, int timeLeft, int collectedHidden) {
    clearScreen();

    // Calcular los límites de la vista
    int startX = max(0, map.player.x - VIEW_SIZE / 2);
    int startY = max(0, map.player.y - VIEW_SIZE / 2);
    int endX = min(MAP_SIZE, startX + VIEW_SIZE);
    int endY = min(MAP_SIZE, startY + VIEW_SIZE);

    // Mostrar vista del mapa
    for (int i = startX; i < endX; ++i) {
        for (int j = startY; j < endY; ++j) {
            if (i == map.player.x && j == map.player.y) {
                cout << '@';
            }
            else {
                // Verificar si hay un objeto oculto no recogido en esta posición
                bool hiddenItemHere = false;
                for (auto& item : map.hiddenItems) {
                    if (item.pos.x == i && item.pos.y == j && !item.collected) {
                        hiddenItemHere = true;
                        break;
                    }
                }

                if (hiddenItemHere) {
                    cout << 'H';
                }
                else {
                    cout << map.grid[i][j];
                }
            }
        }
        cout << endl;
    }

    // Mostrar información del juego
    cout << "\nMapa: " << (map.unlocked ? "Desbloqueado" : "Bloqueado") << endl;
    cout << "Tiempo: " << timeLeft << " segundos" << endl;
    cout << "Objetos encontrados: " << collectedHidden << "/" << TOTAL_HIDDEN << endl;
    cout << "Controles: WASD (movimiento), C (cambiar mapa), Q (salir)" << endl;
}

// Función principal del juego
void playGame() {
    vector<Map> maps(NUM_MAPS);
    int currentMap = 0;
    int collectedHidden = 0;
    int timeLeft = 180; // 180 segundos iniciales

    // Inicializar todos los mapas
    for (int i = 0; i < NUM_MAPS; ++i) {
        initializeMap(maps[i], i);
    }

    auto startTime = chrono::steady_clock::now();

    while (true) {
        // Calcular tiempo restante
        auto currentTime = chrono::steady_clock::now();
        int elapsed = chrono::duration_cast<chrono::seconds>(currentTime - startTime).count();
        timeLeft = 180 + TIME_BONUS * collectedHidden;
        // Sumar bonus por cada T recogida
        for (int i = 0; i < NUM_MAPS; ++i) {
            for (auto& item : maps[i].visibleItems) {
                if (item.collected) timeLeft += TIME_BONUS;
            }
        }
        timeLeft -= elapsed;

        if (timeLeft <= 0) {
            clearScreen();
            cout << "¡Tiempo agotado! Has perdido." << endl;
            return;
        }

        // Mostrar mapa actual
        displayMap(maps[currentMap], timeLeft, collectedHidden);

        // Comprobar victoria
        if (collectedHidden >= TOTAL_HIDDEN) {
            clearScreen();
            cout << "¡Felicidades! Has encontrado todos los objetos ocultos." << endl;
            cout << "¡Has ganado el juego!" << endl;
            return;
        }

        // Obtener entrada del usuario
        if (_kbhit()) {
            char input = _getch();
            input = tolower(input);

            int newX = maps[currentMap].player.x;
            int newY = maps[currentMap].player.y;

            // Movimiento
            if (input == 'w') newX--;
            else if (input == 's') newX++;
            else if (input == 'a') newY--;
            else if (input == 'd') newY++;
            // Cambiar de mapa
            else if (input == 'c') {
                clearScreen();
                cout << "Selecciona un mapa (0-4):" << endl;
                for (int i = 0; i < NUM_MAPS; ++i) {
                    cout << i << ": " << (maps[i].unlocked ? "Desbloqueado" : "Bloqueado") << endl;
                }

                int choice;
                cin >> choice;

                if (choice >= 0 && choice < NUM_MAPS && maps[choice].unlocked) {
                    currentMap = choice;
                }
                continue;
            }
            // Salir del juego
            else if (input == 'q') {
                return;
            }
            else {
                continue;
            }

            // Comprobar validez del movimiento
            if (newX >= 0 && newX < MAP_SIZE && newY >= 0 && newY < MAP_SIZE &&
                maps[currentMap].grid[newX][newY] != '#') {

                // Actualizar posición del jugador
                maps[currentMap].player.x = newX;
                maps[currentMap].player.y = newY;

                // Comprobar si se recoge la llave
                if (!maps[currentMap].key.collected &&
                    maps[currentMap].player.x == maps[currentMap].key.pos.x &&
                    maps[currentMap].player.y == maps[currentMap].key.pos.y) {

                    maps[currentMap].key.collected = true;
                    maps[currentMap].grid[maps[currentMap].key.pos.x][maps[currentMap].key.pos.y] = ' ';

                    // Desbloquear el siguiente mapa si existe
                    if (currentMap < NUM_MAPS - 1) {
                        maps[currentMap + 1].unlocked = true;
                    }
                }

                // Comprobar objetos ocultos
                for (auto& item : maps[currentMap].hiddenItems) {
                    if (!item.collected &&
                        maps[currentMap].player.x == item.pos.x &&
                        maps[currentMap].player.y == item.pos.y) {

                        item.collected = true;
                        collectedHidden++;
                    }
                }

                // Comprobar objetos visibles (T)
                for (auto& item : maps[currentMap].visibleItems) {
                    if (!item.collected &&
                        maps[currentMap].player.x == item.pos.x &&
                        maps[currentMap].player.y == item.pos.y) {

                        item.collected = true;
                        maps[currentMap].grid[item.pos.x][item.pos.y] = ' ';
                        // Sumar tiempo extra (ya se suma en el cálculo de timeLeft)
                    }
                }
            }
        }

        // Pequeña pausa para suavizar la experiencia
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

// Menú principal
int main() {
    srand(time(nullptr));

    while (true) {
        clearScreen();
        cout << "LABERINTO DEL TESORO" << endl;
        cout << "1. Iniciar juego" << endl;
        cout << "2. Salir" << endl;
        cout << "Selecciona una opcion: ";

        char choice;
        cin >> choice;

        if (choice == '1') {
            playGame();
        }
        else if (choice == '2') {
            break;
        }
    }

    return 0;
}