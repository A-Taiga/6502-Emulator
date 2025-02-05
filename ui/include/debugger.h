#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "mem.h"
#include "window.h"
#include <condition_variable>


namespace MOS_6502
{
    class CPU;
    class CPU_Trace;
}

namespace Memory
{
    class ROM;
    class RAM;
}

struct File_info
{
    std::string file_name;
    std::string file_path;
    std::size_t file_size;
};

class GUI
{

public:
    std::condition_variable cv;
    std::mutex mu;

    bool is_paused = true;
    bool step = false;


    // GUI (Emulator_state& data);
    GUI (MOS_6502::CPU& _cpu, MOS_6502::CPU_Trace& _trace, Memory::ROM& _rom, Memory::RAM& _ram);
    void run ();
    bool is_running() {return window.is_running();}

private:

    void registers (void);
    void code_window (void);
    void trace_window (void);
    void rom_select_box (void);
    void action_bar (void);

    Window window;
    MOS_6502::CPU& cpu;
    MOS_6502::CPU_Trace& trace;
    Memory::ROM& rom;
    Memory::RAM& ram;
    std::vector <std::string> code;
    File_info* current_rom;
    std::vector <File_info> roms;
    std::array <std::function<std::uint16_t(void)>, 14> register_callbacks;
};


#endif