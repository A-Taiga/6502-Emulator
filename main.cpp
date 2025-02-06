
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


/*

*=$8000
BRK
*=$F000
.BYTE $a9
.BYTE $a5
.BYTE $85
.BYTE $20
.BYTE $8d
.BYTE $20
.BYTE $01
.BYTE $a9
.BYTE $5a
.BYTE $85
.BYTE $21
.BYTE $a2
.BYTE $a5
.BYTE $e0
.BYTE $a5
.BYTE $f0
.BYTE $02
.BYTE $a2
.BYTE $01
.BYTE $e4
.BYTE $20
.BYTE $f0
.BYTE $02
.BYTE $a2
.BYTE $02
.BYTE $ec
.BYTE $20
.BYTE $01
.BYTE $f0
.BYTE $02
.BYTE $a2
.BYTE $03
.BYTE $86
.BYTE $30
.BYTE $a4
.BYTE $30
.BYTE $c0
.BYTE $a5
.BYTE $f0
.BYTE $02
.BYTE $a0
.BYTE $04
.BYTE $c4
.BYTE $20
.BYTE $f0
.BYTE $02
.BYTE $a0
.BYTE $05
.BYTE $cc
.BYTE $20
.BYTE $01
.BYTE $f0
.BYTE $02
.BYTE $a0
.BYTE $06
.BYTE $84
.BYTE $31
.BYTE $a5
.BYTE $31
.BYTE $24
.BYTE $20
.BYTE $d0
.BYTE $02
.BYTE $a9
.BYTE $07
.BYTE $2c
.BYTE $20
.BYTE $01
.BYTE $d0
.BYTE $02
.BYTE $a9
.BYTE $08
.BYTE $24
.BYTE $21
.BYTE $d0
.BYTE $02
.BYTE $85
.BYTE $42


*=$FFFC
.DBYTE $F000
*/


void cpu_thread_handler (MOS_6502::CPU& cpu, MOS_6502::CPU_Trace& trace, GUI& gui);

int main()
{

    /* I don't like these */
    Memory::RAM ram{};
    Memory::ROM rom{};

    Bus bus (rom, ram);

    MOS_6502::CPU cpu (
        [&bus] (const auto address) {return bus.read(address);},
        [&bus] (const auto address, const auto data) {bus.write(address, data);}
    );

    MOS_6502::CPU_Trace trace (cpu, rom.get_rom());

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


