
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
.BYTE $4b
.BYTE $4a
.BYTE $0a
.BYTE $85
.BYTE $50
.BYTE $06
.BYTE $50
.BYTE $06
.BYTE $50
.BYTE $46
.BYTE $50
.BYTE $a5
.BYTE $50
.BYTE $a6
.BYTE $50
.BYTE $09
.BYTE $c9
.BYTE $85
.BYTE $60
.BYTE $16
.BYTE $4c
.BYTE $56
.BYTE $4c
.BYTE $56
.BYTE $4c
.BYTE $b5
.BYTE $4c
.BYTE $a6
.BYTE $60
.BYTE $09
.BYTE $41
.BYTE $8d
.BYTE $2e
.BYTE $01
.BYTE $5e
.BYTE $00
.BYTE $01
.BYTE $5e
.BYTE $00
.BYTE $01
.BYTE $1e
.BYTE $00
.BYTE $01
.BYTE $bd
.BYTE $00
.BYTE $01
.BYTE $ae
.BYTE $2e
.BYTE $01
.BYTE $09
.BYTE $81
.BYTE $9d
.BYTE $00
.BYTE $01
.BYTE $4e
.BYTE $36
.BYTE $01
.BYTE $4e
.BYTE $36
.BYTE $01
.BYTE $0e
.BYTE $36
.BYTE $01
.BYTE $bd
.BYTE $00
.BYTE $01
.BYTE $2a
.BYTE $2a
.BYTE $6a
.BYTE $85
.BYTE $70
.BYTE $a6
.BYTE $70
.BYTE $09
.BYTE $03
.BYTE $95
.BYTE $0C
.BYTE $26
.BYTE $c0
.BYTE $66
.BYTE $c0
.BYTE $66
.BYTE $c0
.BYTE $b5
.BYTE $0c
.BYTE $a6
.BYTE $c0
.BYTE $85
.BYTE $d0
.BYTE $36
.BYTE $75
.BYTE $36
.BYTE $75
.BYTE $76
.BYTE $75
.BYTE $a5
.BYTE $d0
.BYTE $a6
.BYTE $d0
.BYTE $9d
.BYTE $00
.BYTE $01
.BYTE $2e
.BYTE $b7
.BYTE $01
.BYTE $2e
.BYTE $b7
.BYTE $01
.BYTE $2e
.BYTE $b7
.BYTE $01
.BYTE $6e
.BYTE $b7
.BYTE $01
.BYTE $bd
.BYTE $00
.BYTE $01
.BYTE $ae
.BYTE $b7
.BYTE $01
.BYTE $8d
.BYTE $dd
.BYTE $01
.BYTE $3e
.BYTE $00
.BYTE $01
.BYTE $7e
.BYTE $00
.BYTE $01
.BYTE $7e
.BYTE $00
.BYTE $01

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


