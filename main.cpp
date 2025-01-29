
#include "bus.h"
#include "mos6502.h"
#include "debugger.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>


int main()
{

    Bus bus;
    MOS_6502::CPU cpu (
        [&bus] (const auto address) {return bus.read(address);},
        [&bus] (const auto address, const auto data) {bus.write(address, data);}
    );

    MOS_6502::CPU_Trace trace (cpu, bus.get_memory());

    GUI gui ("6502 Debugger", 1920, 1080, 
                {
                    bus.get_memory(),
                    cpu,
                    trace,
                    MOS_6502::disassembler(bus.get_memory(), cpu.get_PC())
                }
            );

    std::thread cpu_thread ([&]()
    {
        while (gui.is_running())
        {   
            {
                std::unique_lock <std::mutex> lock (gui.mu);
                gui.cv.wait(lock, [&gui](){return !gui.is_paused;});
            }
            
            cpu.update();
            trace.trace();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    std::thread cpu_step_thread ([&]()
    {
        while (gui.is_running())
        {
            {
                std::unique_lock <std::mutex> lock (gui.mu);
                gui.cv.wait(lock, [&gui](){return gui.step;});
            }
            cpu.update();
            trace.trace();
            {
                std::lock_guard <std::mutex> lock(gui.mu);
                gui.step = false;
            }
            gui.cv.notify_all();

        }
    });

    gui.run();
    cpu_thread.join();
    cpu_step_thread.join();

    return 0;
}