#include "emulator.hpp"
#include "common.hpp"
#include <chrono>
#include <format>
#include <cstring>
#include "cpu.hpp"
#include "debugger.hpp"
#include <thread>

namespace
{
    template<std::size_t N>
    std::size_t load_rom (const char* path, std::array<byte, N>& buffer, [[maybe_unused]] word begin = 0, word end = 0)
    {
        FILE* file = fopen(path, "rb");
        std::size_t size;

        if (file == nullptr)                           throw std::runtime_error(std::strerror(errno));
        if (std::fseek (file, 0L, SEEK_END) == -1)     throw std::runtime_error(std::strerror(errno));
        if ((size = std::ftell(file)) == -1UL)         throw std::runtime_error(std::strerror(errno));
        if (size > end)                                throw std::runtime_error(std::format("file > {:d}", N));
        if ((std::fseek(file, 0L, SEEK_SET)) == -1L)   throw std::runtime_error(std::strerror(errno));
        if ((std::fread(buffer.data() + ROM_BEGIN, size, 1, file)) == -1UL)  
            throw std::runtime_error(std::strerror(errno));
        fclose (file);
        return size;
    }
}

_6502::Emulator::Emulator(const char* filePath)
: bus ()
{
    load_rom (filePath, bus.ram.data(), ROM_BEGIN, ROM_END);
}

void _6502::Emulator::run()
{

    using namespace std::chrono_literals;
    UI::debug_v debugData = {bus, 100ms, true, true, false};
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

