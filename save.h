// saving and loading worlds: each slot is a .txt (settings and player) and a .bin (blocks)
#pragma once

#include "world.h"
#include "input.h" // Player

// writes the slot's files, replacing whatever was saved there before
void save_world(int slot, const WorldSettings &settings, const World &world, const Camera &cam, const Player &player);

// reads the slot's blocks, position and look direction back in.
// false if the files are missing or damaged
bool load_world(int slot, World &world, Camera &cam, Player &player);

// which slots have a saved world and what they're called, read when the game opens
SaveSlots load_save_slots();