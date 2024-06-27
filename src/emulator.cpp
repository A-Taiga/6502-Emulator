#include "emulator.hpp"
#include "common.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include "cpu.hpp"
#include "debugger.hpp"
#include <thread>
#include <filesystem>
#include <fstream>

namespace
{
    template<std::size_t N>
    std::size_t load_rom (std::string_view path, const std::array<byte, N>& buffer, const std::size_t offset)
    {
        try
        {
            std::size_t fileSize = std::filesystem::file_size(path);
            std::fstream file (path, std::ios::hex);
            file.read((char*)buffer.data()+offset, fileSize);
            file.close();
            return fileSize;
        }
        catch (const std::filesystem::filesystem_error& e) { std::cerr << e.what() << '\n';}
        catch (const std::ifstream::failure& e) {std::cerr << e.what() << '\n';}
        catch (...) {std::cerr << "error catch all in " << __PRETTY_FUNCTION__ << '\n';}
        return 0;
    }
}

_6502::Emulator::Emulator(const char* filePath)
: bus ()
, currentFile(filePath)
{
    load_rom (filePath, bus.ram.data(), ROM_BEGIN);
    bus.ram[RESET_VECTOR]     = ROM_BEGIN & 0x00FF;
    bus.ram[RESET_VECTOR + 1] = (ROM_BEGIN & 0xFF00) >> 8;
}

void _6502::Emulator::run()
{
    bus.cpu.reset();
    using namespace std::chrono_literals;
    UI::debug_v debugData = {bus, 100ms, true, true, false,[&](){reset();}};
    UI::init();
    bus.cpu.decompiler();

    static std::thread t ([&](){
        while (debugData.running)
        {
            if (!debugData.pause)
            {
                bus.cpu.run();
                std::this_thread::sleep_for(debugData.delay);
            }
        }
    });
    
    while (debugData.running)
    {
        UI::debug(debugData);
    }
    t.join();
    UI::end();
}

void _6502::Emulator::reset ()
{
    bus.ram.reset();
    load_rom (currentFile.c_str(), bus.ram.data(), ROM_BEGIN);
    bus.ram[RESET_VECTOR]     = (byte)0x00;
    bus.ram[RESET_VECTOR + 1] = (byte)0xF0;
    bus.cpu.reset();
}



