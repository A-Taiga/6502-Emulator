#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "mos6502.h"
#include "window.h"
#include <array>
#include <condition_variable>
#include <cstdint>
#include <filesystem>


namespace MOS_6502 {class CPU;}

struct emulator_data
{
    std::array <std::uint8_t, 65534>& memory;
    MOS_6502::CPU& cpu;
    MOS_6502::CPU_Trace& trace;
    std::vector <std::string> code;

    std::filesystem::directory_entry* current_rom = nullptr;
    std::vector <std::filesystem::directory_entry> roms{};
};

class GUI
{

public:
    std::condition_variable cv;
    std::mutex mu;
    bool is_paused = true;
    bool step = false;


    GUI (const char* title, const int width, const int height, emulator_data data);
    void run ();
    bool is_running() {return window.is_running();}

private:
    Window window;
    emulator_data emu_data;

};


#endif