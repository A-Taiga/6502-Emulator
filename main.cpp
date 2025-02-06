
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
.BYTE $00
.BYTE $85
.BYTE $34
.BYTE $a9
.BYTE $ff
.BYTE $8d
.BYTE $30
.BYTE $01
.BYTE $a9
.BYTE $99
.BYTE $8d
.BYTE $9d
.BYTE $01
.BYTE $a9
.BYTE $db
.BYTE $8d
.BYTE $99
.BYTE $01
.BYTE $a9
.BYTE $2f
.BYTE $85
.BYTE $32
.BYTE $a9
.BYTE $32
.BYTE $85
.BYTE $4f
.BYTE $a9
.BYTE $30
.BYTE $85
.BYTE $33
.BYTE $a9
.BYTE $70
.BYTE $85
.BYTE $af
.BYTE $a9
.BYTE $18
.BYTE $85
.BYTE $30
.BYTE $c9
.BYTE $18
.BYTE $f0
.BYTE $02
.BYTE $29
.BYTE $00
.BYTE $09
.BYTE $01
.BYTE $c5
.BYTE $30
.BYTE $d0
.BYTE $02
.BYTE $29
.BYTE $00
.BYTE $a2
.BYTE $00
.BYTE $cd
.BYTE $30
.BYTE $01
.BYTE $f0
.BYTE $04
.BYTE $85
.BYTE $40
.BYTE $a6
.BYTE $40
.BYTE $d5
.BYTE $27
.BYTE $d0
.BYTE $06
.BYTE $09
.BYTE $84
.BYTE $85
.BYTE $41
.BYTE $a6
.BYTE $41
.BYTE $29
.BYTE $db
.BYTE $dd
.BYTE $00
.BYTE $01
.BYTE $f0
.BYTE $02
.BYTE $29
.BYTE $00
.BYTE $85
.BYTE $42
.BYTE $a4
.BYTE $42
.BYTE $29
.BYTE $00
.BYTE $d9
.BYTE $00
.BYTE $01
.BYTE $d0
.BYTE $02
.BYTE $09
.BYTE $0f
.BYTE $85
.BYTE $43
.BYTE $a6
.BYTE $43
.BYTE $09
.BYTE $24
.BYTE $c1
.BYTE $40
.BYTE $f0
.BYTE $02
.BYTE $09
.BYTE $7f
.BYTE $85
.BYTE $44
.BYTE $a4
.BYTE $44
.BYTE $49
.BYTE $0f
.BYTE $d1
.BYTE $33
.BYTE $d0
.BYTE $04
.BYTE $a5
.BYTE $44
.BYTE $85
.BYTE $15

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


