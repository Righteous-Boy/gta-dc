#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>

typedef enum {
    WEAPON_NONE,
    WEAPON_PISTOL,
    WEAPON_SMG,
    WEAPON_SHOTGUN,
    WEAPON_RIFLE,
    WEAPON_SNIPER,
    WEAPON_MELEE,
    WEAPON_THROWABLE
} WeaponType;

typedef struct {
    WeaponType type;
    int ammo;
    int max_ammo;
    int clip_ammo;
    int max_clip;
    float fire_rate;
    float fire_timer;
    float damage;
    float range;
} Weapon;

typedef enum {
    STATE_IDLE,
    STATE_WALKING,
    STATE_RUNNING,
    STATE_SPRINTING,
    STATE_JUMPING,
    STATE_FALLING,
    STATE_SHOOTING,
    STATE_RELOADING,
    STATE_MELEE,
    STATE_CROUCHING,
    STATE_AIMING,
    STATE_DEAD
} PlayerState;

typedef struct Vehicle Vehicle;

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float yaw, pitch, roll;
    float height;
    float width;
    
    PlayerState state;
    PlayerState prev_state;
    
    float move_speed;
    float run_speed;
    float sprint_speed;
    float current_speed;
    
    int health;
    int armor;
    int wanted_level;
    
    Weapon weapons[MAX_WEAPONS];
    int current_weapon;
    int num_weapons;
    
    int money;
    int experience;
    
    Vehicle *current_vehicle;
    int in_vehicle;
    
    float animation_timer;
    float jump_timer;
    int is_jumping;
    int is_grounded;
    
    int is_aiming;
    int is_shooting;
    int is_reloading;
    
    int crouched;
    float crouch_height;
    
    float last_fire_time;
    float fire_cooldown;
    
    int alive;
} Player;

void player_init(Player *player);
void player_update(Player *player, float dt);
void player_render(SDL_Renderer *renderer, Player *player, struct Camera *camera);
void player_handle_input(Player *player, struct InputState *input, struct Camera *camera);
void player_take_damage(Player *player, int damage);
void player_heal(Player *player, int amount);
void player_add_weapon(Player *player, WeaponType type, int ammo);
void player_switch_weapon(Player *player, int index);
void player_reload_weapon(Player *player);
void player_fire_weapon(Player *player, struct World *world, struct GameState *game_state);
void player_melee_attack(Player *player, struct World *world);
void player_enter_vehicle(Player *player, Vehicle *vehicle);
void player_exit_vehicle(Player *player);
void player_jump(Player *player);
void player_crouch(Player *player);
void player_aim(Player *player, int aiming);
void player_apply_physics(Player *player, struct World *world, float dt);

#endif
