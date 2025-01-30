#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "window.h"
#include <condition_variable>



struct File_info
{
    std::string file_name;
    std::string file_path;
    std::size_t file_size;
};

struct Emulator_state
{
    std::span <File_info>    roms;
    std::span <std::uint8_t> rom;
    std::span <std::uint8_t> ram;
    std::span <std::string>  code;
    std::span <const char*>  register_names;
    std::span <const char*>  format_strings;

    std::span <std::function<std::uint16_t(void)>> register_callbacks;
    std::vector<std::vector<std::string>>& trace;
    File_info* current_rom = nullptr;

};

class GUI
{

public:
    std::condition_variable cv;
    std::mutex mu;
    bool is_paused = true;
    bool step = false;


    GUI (const char* title, const int width, const int height, Emulator_state& data);
    void run ();
    bool is_running() {return window.is_running();}

private:
    Window window;
    Emulator_state& emu_data;

};


#endif