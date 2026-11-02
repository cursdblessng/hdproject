// saving and loading: each world slot is a .txt (settings and player) and a .bin (blocks),
// and the player's settings go in settings.txt
#pragma once

#include "world.h"
#include "../game/input.h" // Player
#include "../game/settings.h"

// writes the slot's files, replacing whatever was saved there before
void save_world(int slot, const WorldSettings &settings, const World &world, const Camera &cam, const Player &player);

// reads the slot's blocks, position and look direction back in.
// false if the files are missing or damaged
bool load_world(int slot, World &world, Camera &cam, Player &player);

// empties the slot so it shows as an empty slot and a new world can be made in it
void delete_world(int slot);

// which slots have a saved world and what they're called, read when the game opens
SaveSlots load_save_slots();

// writes the chosen option for every setting to settings.txt
void save_settings(const Settings &settings);

// reads settings.txt, anything missing or broken stays on its default
Settings load_settings();
