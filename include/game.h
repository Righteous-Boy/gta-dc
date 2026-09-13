#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>

#define MAX_VEHICLES 128
#define MAX_NPCS 256
#define MAX_MISSIONS 10
#define MAX_WEAPONS 16
#define MAX_WORLD_OBJECTS 512

typedef struct Player Player;
typedef struct World World;
typedef struct Camera Camera;
typedef struct Vehicle Vehicle;
typedef struct NPC NPC;
typedef struct MissionSystem MissionSystem;
typedef struct UI UI;
typedef struct Physics Physics;

typedef struct {
    Player *player;
    World *world;
    Camera *camera;
    MissionSystem *missions;
    UI *ui;
    Vehicle **vehicles;
    NPC **npcs;
    Physics *physics;
    
    int num_vehicles;
    int num_npcs;
    int current_mission;
    float time_of_day;
    int game_paused;
    int game_running;
    int wanted_level;
    float wanted_timer;
    int money;
} GameState;

void create_world_npcs(GameState *state);
void create_traffic(GameState *state);
void update_traffic(GameState *state, float dt);
void update_police_ai(GameState *state, float dt);
void update_wanted_level(GameState *state, float dt);

#endif
