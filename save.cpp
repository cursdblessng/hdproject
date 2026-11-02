#include "save.h"
#include <fstream> // ofstream, ifstream
#include <iomanip> // setprecision

// start of every save file name, the files go next to where the game is run from
const string SAVE_FILE_PREFIX = "save_slot";

// one file for the settings, shared by every world
const string SETTINGS_FILE = "settings.txt";

// a run's length is stored in one byte, which can't go above 255
const int MAX_RUN_LENGTH = 255;

// e.g. slot 0 and ".bin" -> "save_slot1.bin". slots are 0-2 in code but 1-3 in file names
string slot_path(int slot, const string &extension)
{
    return SAVE_FILE_PREFIX + std::to_string(slot + 1) + extension;
}

/**
 * one run is two bytes: how many blocks in a row, then which type.
 * put writes a single byte, so no pointers are needed
 */
void write_run(std::ofstream &file, int length, BlockType type)
{
    file.put((char)length);
    file.put((char)type);
}

/**
 * run length encoding: instead of every block, write how many of the same type come in a row.
 * a column of 40 stone, 3 dirt, 1 grass and 84 air is 4 runs (8 bytes) instead of 128 bytes.
 * goes up each column (y last) because columns are mostly long runs of the same block.
 * load_blocks has to go through the blocks in exactly this order
 */
void save_blocks(const string &path, const World &world)
{
    // binary, otherwise windows changes every byte that happens to be 10 (a new line) into 13 10
    std::ofstream file(path, std::ios::binary);

    int run_length = 0;
    BlockType run_type = AIR;

    for (int x = 0; x < WORLD_SIZE; x++)
    {
        for (int z = 0; z < WORLD_SIZE; z++)
        {
            for (int y = 0; y < WORLD_HEIGHT; y++)
            {
                BlockType type = get_block(world, x, y, z);

                // the run so far ends when the type changes or it can't get any longer
                if (run_length > 0 && (type != run_type || run_length == MAX_RUN_LENGTH))
                {
                    write_run(file, run_length, run_type);
                    run_length = 0;
                }

                run_type = type;
                run_length++;
            }
        }
    }

    // the last run has no block after it to end it, so it's written here
    write_run(file, run_length, run_type);
}

/**
 * reads the runs back in the same order save_blocks wrote them.
 * a new run is read whenever the last one has been used up.
 * every block gets set, so the world doesn't need clearing first
 */
bool load_blocks(const string &path, World &world)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    int run_length = 0; // blocks left in the current run
    BlockType run_type = AIR;

    for (int x = 0; x < WORLD_SIZE; x++)
    {
        for (int z = 0; z < WORLD_SIZE; z++)
        {
            for (int y = 0; y < WORLD_HEIGHT; y++)
            {
                // <= so a damaged run of 0 reads the next one instead of going negative
                if (run_length <= 0)
                {
                    // get gives back a byte as 0 to 255, or EOF (-1) once the file has run out
                    int length_byte = file.get();
                    int type_byte = file.get();

                    // ran out before every block was set, so the file is cut short
                    if (type_byte == EOF)
                    {
                        return false;
                    }

                    run_length = length_byte;
                    run_type = (BlockType)type_byte;
                }

                set_block(world, x, y, z, run_type);
                run_length--;
            }
        }
    }

    return true;
}

/**
 * the first three lines of a slot's .txt, used by load_save_slots and load_world.
 * getline for the name because >> stops at the first space
 */
bool read_settings(std::ifstream &file, WorldSettings &settings)
{
    std::getline(file, settings.name);
    file >> settings.seed;
    file >> settings.is_superflat;

    // fail is set if any of them couldn't be read
    return !file.fail();
}

/**
 * blocks first, so the .txt that marks the slot as used only changes once the blocks are written.
 * velocity and on_ground aren't saved, the player just drops back onto the ground on load
 */
void save_world(int slot, const WorldSettings &settings, const World &world, const Camera &cam, const Player &player)
{
    save_blocks(slot_path(slot, ".bin"), world);

    std::ofstream file(slot_path(slot, ".txt"));

    // doubles are only written to 6 digits by default, 256.123456 would come back as 256.123.
    // that's enough to put your feet slightly inside the ground and get you stuck, 17 is exact
    file << std::setprecision(17);

    file << settings.name << "\n";
    file << settings.seed << "\n";
    file << settings.is_superflat << "\n";
    file << cam.position.x << " " << cam.position.y << " " << cam.position.z << "\n";
    file << cam.yaw << " " << cam.pitch << "\n";
    file << player.is_flying << "\n";
}

/**
 * reads the .txt in the same order save_world wrote it, then the blocks.
 * false if either file is missing or something couldn't be read
 */
bool load_world(int slot, World &world, Camera &cam, Player &player)
{
    std::ifstream file(slot_path(slot, ".txt"));
    if (!file.is_open())
    {
        return false;
    }

    // already in save_slots from when the game opened, only read here to get past them
    WorldSettings settings;
    read_settings(file, settings);

    file >> cam.position.x >> cam.position.y >> cam.position.z;
    file >> cam.yaw >> cam.pitch;
    file >> player.is_flying;

    if (file.fail())
    {
        return false;
    }

    return load_blocks(slot_path(slot, ".bin"), world);
}

/**
 * empties both of the slot's files instead of removing them.
 * removing a file needs std::remove, which only takes a c style string (a pointer).
 * an empty .txt can't be read, so load_save_slots sees the slot as empty,
 * and making a new world there writes over both files
 */
void delete_world(int slot)
{
    // opening a file for writing empties it straight away,
    // and they close at the end of the function with nothing written
    std::ofstream bin_file(slot_path(slot, ".bin"), std::ios::binary);
    std::ofstream txt_file(slot_path(slot, ".txt"));
}

/**
 * a slot is used if its .txt opens and has settings in it.
 * only the .txt is read, the blocks aren't loaded until the world is opened
 */
SaveSlots load_save_slots()
{
    SaveSlots save_slots;

    for (int i = 0; i < NUM_SAVE_SLOTS; i++)
    {
        std::ifstream file(slot_path(i, ".txt"));

        save_slots.slots[i].is_used = false;
        if (file.is_open())
        {
            save_slots.slots[i].is_used = read_settings(file, save_slots.slots[i].settings);
        }
    }

    return save_slots;
}

// one number per line, the chosen option for each setting in SettingId order
void save_settings(const Settings &settings)
{
    std::ofstream file(SETTINGS_FILE);

    for (int i = 0; i < NUM_SETTINGS; i++)
    {
        file << settings.choice[i] << "\n";
    }
}

/**
 * starts from the defaults and replaces each one that reads in properly.
 * a setting added later isn't in older files, so reading it fails and it keeps its default.
 * each number is checked, so a file edited by hand can't pick an option that doesn't exist
 */
Settings load_settings()
{
    Settings settings = default_settings();

    std::ifstream file(SETTINGS_FILE);
    if (!file.is_open())
    {
        return settings;
    }

    for (int i = 0; i < NUM_SETTINGS; i++)
    {
        int option;
        file >> option;

        // ran out of numbers, everything after this keeps its default
        if (file.fail())
        {
            return settings;
        }

        if (is_valid_option(i, option))
        {
            settings.choice[i] = option;
        }
    }

    return settings;
}
