#pragma once 

#include <stdint.h>
#include <set>
#include <vector>
#include "board_properties.hpp"

constexpr int TILE_SIZE  = 20; 
constexpr int MAP_WIDTH  = 24;
constexpr int MAP_HEIGHT = 16;

enum TILE_TYPE {
    BLACK       = 0,
    GRASS       = 1, 
    BRICKS_WALL = 2,
    BEDROCK     = 3,
    SPECIAL     = 4,
    SPAWN_POINTS= 5,
    SPAWN_P1    = 6,
    SHIELD      = 7, //default sheild
    SHIELD_ON_GRASS   = 8,
    SHIELD_ON_BEDROCK = 9,
    SHIELD_ON_BRICK   = 10,
    OUB = 11, //out of bounds
};

struct spawnPlace {
    int x, y;
};

struct Level {
    uint8_t (*map)[MAP_WIDTH];
    spawnPlace spawn_place_p1;
    int amount_of_bots_to_spawn;
    std::vector<spawnPlace> bot_spawns; // x, y
    uint16_t time_between_spawns_tick; 
};

extern uint8_t map_level_1[MAP_HEIGHT][MAP_WIDTH];
extern uint8_t map_level_2[MAP_HEIGHT][MAP_WIDTH];
extern uint8_t map_level_3[MAP_HEIGHT][MAP_WIDTH];