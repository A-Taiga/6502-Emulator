
#include "bus.h"
#include "mos6502.h"
#include "debugger.h"
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>
#include "mem.h"



void cpu_handler (MOS_6502::CPU& cpu, MOS_6502::CPU_Trace& trace, GUI& gui);

int main()
{

    /* I don't like these */
    Memory::RAM ram{};
    Memory::ROM rom{};

    Bus bus (rom, ram);


    std::vector <File_info> roms{};

    for (const auto& entry : std::filesystem::recursive_directory_iterator("roms/"))
    {
        roms.emplace_back (
                            entry.path().filename().string(), 
                            entry.path().string(), 
                            entry.file_size()
                          );
    }

    rom.load(roms.back().file_path, roms.back().file_size);

    MOS_6502::CPU cpu (
        [&bus] (const auto address) {return bus.read(address);},
        [&bus] (const auto address, const auto data) {bus.write(address, data);}
    );

    std::array <const char*, 14> register_names = {"PC", "AC", "XR", "YR", "SP", " ", "C", "Z", "I", "D", "B", "_", "V", "N"};
    std::array <const char*, 14> format_strings = {"%04X", "%02X", "%02X", "%02X", "%02X", "%c", "%d", "%d", "%d", "%d", "%d", "%d", "%d", "%d"};
    std::array <std::function<std::uint16_t(void)>, 14> register_callbacks
    {
        [&cpu](){return cpu.get_PC();},
        [&cpu](){return cpu.get_AC();},
        [&cpu](){return cpu.get_XR();},
        [&cpu](){return cpu.get_YR();},
        [&cpu](){return cpu.get_SP();},
        [](){return 32;},
        [&cpu](){return (cpu.get_SR() >> 0) & 1;},
        [&cpu](){return (cpu.get_SR() >> 1) & 1;},
        [&cpu](){return (cpu.get_SR() >> 2) & 1;},
        [&cpu](){return (cpu.get_SR() >> 3) & 1;},
        [&cpu](){return (cpu.get_SR() >> 4) & 1;},
        [&cpu](){return (cpu.get_SR() >> 5) & 1;},
        [&cpu](){return (cpu.get_SR() >> 6) & 1;},
        [&cpu](){return (cpu.get_SR() >> 7) & 1;},
    };

    std::vector <std::string> code = MOS_6502::disassembler(rom.get_rom(), 0x7000);
    MOS_6502::CPU_Trace trace (cpu, rom.get_rom());
    static Emulator_state emu_state = 
    {
        roms,
        rom.get_rom(),
        ram.get_ram(),
        code,
        register_names,
        format_strings,
        register_callbacks,
        trace.get_trace_v(),
        &roms.back(),
    };

    GUI gui ("6502 Emulator", 1920, 1080, emu_state);

    // std::thread cpu_thread (cpu_handler, std::ref(cpu), std::ref(trace), std::ref(gui));
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

void cpu_handler (MOS_6502::CPU& cpu, MOS_6502::CPU_Trace& trace, GUI& gui)
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
}