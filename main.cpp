
#include "bus.h"
#include "mos6502.h"
#include "debugger.h"
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include "mem.h"
#include <chrono>
#include <thread>

void cpu_thread_handler (MOS_6502::CPU& cpu, GUI& gui, MOS_6502::trace_type& traces, const MOS_6502::code_map_type& map);

int main()
{
    Memory rom {UINT16_MAX/2};
    Memory ram {UINT16_MAX};

    MOS_6502::trace_type traces;
    MOS_6502::code_map_type code_map;

    Bus bus (rom, ram);

    MOS_6502::CPU cpu (
        [&bus] (const auto address) {return bus.read(address);},
        [&bus] (const auto address, const auto data) {bus.write(address, data);}
    );

    GUI gui (cpu, rom, ram, traces, code_map);

    std::thread cpu_thread (cpu_thread_handler, std::ref(cpu), std::ref(gui), std::ref(traces), std::cref(code_map));

    gui.run();
    cpu_thread.join();

    return 0;
}


void cpu_thread_handler (MOS_6502::CPU& cpu, GUI& gui, MOS_6502::trace_type& traces, const MOS_6502::code_map_type& map)
{
    while (gui.is_running())
    {   
        {
            std::unique_lock <std::mutex> lock (gui.mu);
            gui.cv.wait(lock, [&gui](){return !gui.is_paused || gui.step;});
        }
    
        auto begin = std::chrono::high_resolution_clock::now();

        int cycles = cpu.update();

        // idk if this is how you actually emulate cpu time
        auto end = std::chrono::high_resolution_clock::now();
        auto target = std::chrono::nanoseconds(cycles * 559);

        if (target > (end - begin))
        {
            std::this_thread::sleep_for(target - (end - begin));
        }
        

        if(!MOS_6502::trace(traces, map, cpu))
            std::cerr << map.size() << " " << "did not trace" << std::endl;


        {
            std::lock_guard <std::mutex> lock(gui.mu);
            gui.step = false;
        }
        gui.cv.notify_all();
    }
}


