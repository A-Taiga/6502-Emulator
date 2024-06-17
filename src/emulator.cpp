#include "emulator.hpp"
#include "common.hpp"
#include <chrono>
#include <format>
#include <cstring>
#include "debugger.hpp"

#include <thread>
#define WINDOW_W 1920
#define WINDOW_H 1080

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

_6502::Emulator::Emulator(const char* filePath, bool& _running)
: bus ()
, running(_running)
{
    load_rom (filePath, bus.ram.data(), ROM_BEGIN, ROM_END);
}

void _6502::Emulator::run()
{
    OS_Window window ("test", WINDOW_W, WINDOW_H);
    std::chrono::milliseconds delay(100);
    bool running = true;
    bool pause = true;
    UI::init(window);
    bus.cpu.decompiler();

    static std::thread t ([&](){
        while (running)
        {
            if (!pause)
            {
                bus.cpu.run();
                std::this_thread::sleep_for(delay);
            }
        }
    });
    
    while (running)
    {
        window.poll(running);
        UI::debug(window, bus, delay, pause);
    }

    t.join();
    UI::end();
}