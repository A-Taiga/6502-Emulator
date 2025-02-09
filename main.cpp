
#include "bus.h"
#include "mos6502.h"
#include "debugger.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "mem.h"
#include <chrono>
#include <thread>

void cpu_thread_handler (MOS_6502::CPU& cpu, MOS_6502::CPU_Trace& trace, GUI& gui);

int main()
{
    Memory rom {UINT16_MAX/2};
    Memory ram {UINT16_MAX};

    Bus bus (rom, ram);

    MOS_6502::CPU cpu (
        [&bus] (const auto address) {return bus.read(address);},
        [&bus] (const auto address, const auto data) {bus.write(address, data);}
    );

    MOS_6502::CPU_Trace trace (cpu, rom);

    GUI gui (cpu, trace, rom, ram);

    std::thread cpu_thread (cpu_thread_handler, std::ref(cpu), std::ref(trace), std::ref(gui));

    gui.run();
    cpu_thread.join();

    return 0;
}


void cpu_thread_handler (MOS_6502::CPU& cpu, MOS_6502::CPU_Trace& trace, GUI& gui)
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

        trace.trace();

        {
            std::lock_guard <std::mutex> lock(gui.mu);
            gui.step = false;
        }
        gui.cv.notify_all();
    }
}


