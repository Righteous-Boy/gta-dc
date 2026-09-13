#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "game.h"
#include "player.h"
#include "world.h"
#include "camera.h"
#include "vehicle.h"
#include "npc.h"
#include "mission.h"
#include "ui.h"
#include "physics.h"
#include "input.h"
#include "audio.h"
#include "save.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define TARGET_FPS 60
#define FRAME_TIME (1000.0f / TARGET_FPS)

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_GameController *controller;
    int running;
    float delta_time;
    uint32_t current_ticks;
    uint32_t last_ticks;
} GameEngine;

GameEngine engine;
GameState game_state;

void initialize_engine() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        exit(1);
    }

    engine.window = SDL_CreateWindow(
        "GTA DC - Grand Theft Auto: District Capital",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!engine.window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    engine.renderer = SDL_CreateRenderer(
        engine.window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!engine.renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(engine.window);
        SDL_Quit();
        exit(1);
    }

    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
    
    if (SDL_NumJoysticks() > 0) {
        engine.controller = SDL_GameControllerOpen(0);
        if (engine.controller) {
            printf("Controller connected: %s\n", SDL_GameControllerName(engine.controller));
        }
    }

    engine.running = 1;
    engine.current_ticks = 0;
    engine.last_ticks = SDL_GetTicks();
}

void initialize_game() {
    game_state.player = malloc(sizeof(Player));
    game_state.world = malloc(sizeof(World));
    game_state.camera = malloc(sizeof(Camera));
    game_state.missions = malloc(sizeof(MissionSystem));
    game_state.ui = malloc(sizeof(UI));
    game_state.vehicles = malloc(sizeof(Vehicle*) * MAX_VEHICLES);
    game_state.npcs = malloc(sizeof(NPC*) * MAX_NPCS);
    game_state.physics = malloc(sizeof(Physics));

    game_state.num_vehicles = 0;
    game_state.num_npcs = 0;
    game_state.current_mission = -1;
    game_state.time_of_day = 12.0f;
    game_state.game_paused = 0;
    game_state.game_running = 1;

    player_init(game_state.player);
    world_init(game_state.world);
    camera_init(game_state.camera, game_state.player);
    mission_system_init(game_state.missions);
    ui_init(game_state.ui);
    physics_init(game_state.physics);
    audio_init();

    for (int i = 0; i < MAX_VEHICLES; i++) {
        game_state.vehicles[i] = NULL;
    }
    for (int i = 0; i < MAX_NPCS; i++) {
        game_state.npcs[i] = NULL;
    }

    create_world_npcs(&game_state);
    create_traffic(&game_state);

    printf("Game initialized successfully\n");
}

void handle_input() {
    SDL_Event event;
    InputState input = {0};

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                engine.running = 0;
                break;
            case SDL_KEYDOWN:
                input_handle_keyboard(&input, event.key.keysym.sym, 1);
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    game_state.game_paused = !game_state.game_paused;
                }
                if (event.key.keysym.sym == SDLK_F5) {
                    save_game("savegame.dat", &game_state);
                }
                if (event.key.keysym.sym == SDLK_F9) {
                    load_game("savegame.dat", &game_state);
                }
                break;
            case SDL_KEYUP:
                input_handle_keyboard(&input, event.key.keysym.sym, 0);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                input_handle_controller_button(&input, event.cbutton.button, 1);
                break;
            case SDL_CONTROLLERBUTTONUP:
                input_handle_controller_button(&input, event.cbutton.button, 0);
                break;
            case SDL_CONTROLLERAXISMOTION:
                input_handle_controller_axis(&input, event.caxis.axis, event.caxis.value);
                break;
        }
    }

    if (!game_state.game_paused) {
        player_handle_input(game_state.player, &input, game_state.camera);
    }
}

void update_game(float dt) {
    if (game_state.game_paused) {
        return;
    }

    game_state.time_of_day += dt / 3600.0f;
    if (game_state.time_of_day >= 24.0f) {
        game_state.time_of_day = 0.0f;
    }

    player_update(game_state.player, dt);
    camera_update(game_state.camera, game_state.player, game_state.world);

    for (int i = 0; i < game_state.num_vehicles; i++) {
        if (game_state.vehicles[i]) {
            vehicle_update(game_state.vehicles[i], dt, game_state.world);
        }
    }

    for (int i = 0; i < game_state.num_npcs; i++) {
        if (game_state.npcs[i]) {
            npc_update(game_state.npcs[i], dt, game_state.world, game_state.player);
        }
    }

    mission_system_update(game_state.missions, &game_state, dt);
    physics_update(game_state.physics, dt);
    update_traffic(&game_state, dt);
    update_police_ai(&game_state, dt);
    update_wanted_level(&game_state, dt);

    if (game_state.player->current_vehicle) {
        vehicle_update_camera(game_state.player->current_vehicle, game_state.camera);
    }
}

void render_game() {
    SDL_SetRenderDrawColor(engine.renderer, 135, 206, 235, 255);
    SDL_RenderClear(engine.renderer);

    render_world(engine.renderer, game_state.world, game_state.camera);

    for (int i = 0; i < game_state.num_vehicles; i++) {
        if (game_state.vehicles[i]) {
            vehicle_render(engine.renderer, game_state.vehicles[i], game_state.camera);
        }
    }

    for (int i = 0; i < game_state.num_npcs; i++) {
        if (game_state.npcs[i]) {
            npc_render(engine.renderer, game_state.npcs[i], game_state.camera);
        }
    }

    player_render(engine.renderer, game_state.player, game_state.camera);

    ui_render(engine.renderer, game_state.ui, &game_state);

    if (game_state.game_paused) {
        ui_render_pause_menu(engine.renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    }

    SDL_RenderPresent(engine.renderer);
}

void cleanup_engine() {
    if (engine.controller) {
        SDL_GameControllerClose(engine.controller);
    }
    if (engine.renderer) {
        SDL_DestroyRenderer(engine.renderer);
    }
    if (engine.window) {
        SDL_DestroyWindow(engine.window);
    }
    SDL_Quit();
}

void cleanup_game() {
    if (game_state.player) free(game_state.player);
    if (game_state.world) free(game_state.world);
    if (game_state.camera) free(game_state.camera);
    if (game_state.missions) free(game_state.missions);
    if (game_state.ui) free(game_state.ui);
    
    for (int i = 0; i < game_state.num_vehicles; i++) {
        if (game_state.vehicles[i]) free(game_state.vehicles[i]);
    }
    free(game_state.vehicles);

    for (int i = 0; i < game_state.num_npcs; i++) {
        if (game_state.npcs[i]) free(game_state.npcs[i]);
    }
    free(game_state.npcs);

    if (game_state.physics) free(game_state.physics);
    
    audio_cleanup();
}

int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf("GTA DC - Grand Theft Auto: District Capital\n");
    printf("===========================================\n\n");

    initialize_engine();
    initialize_game();

    uint32_t frame_start;
    float frame_accumulator = 0.0f;

    printf("Starting main game loop\n");

    while (engine.running) {
        frame_start = SDL_GetTicks();

        handle_input();
        update_game(engine.delta_time);
        render_game();

        engine.current_ticks = SDL_GetTicks();
        engine.delta_time = (engine.current_ticks - engine.last_ticks) / 1000.0f;
        
        if (engine.delta_time > 0.1f) {
            engine.delta_time = 0.1f;
        }

        engine.last_ticks = engine.current_ticks;

        uint32_t frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_TIME) {
            SDL_Delay(FRAME_TIME - frame_time);
        }
    }

    printf("Shutting down...\n");
    cleanup_game();
    cleanup_engine();
    printf("Goodbye!\n");

    return 0;
}
