#include "emulator.hpp"
#include "SDL2/SDL_events.h"
#include "common.hpp"
#include "imgui_impl_sdl2.h"
#include "window.hpp"
#include "debug.hpp"
#include <format>
#include <thread>
#include <cstring>


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
    static Window window(1000, 800, "Debugger");

    window.push_event_cb([&](void*, SDL_Event* event){
        if (event->type == SDL_QUIT)
            running = false;
    });

    window.push_event_cb([](void*, SDL_Event* event){
        ImGui_ImplSDL2_ProcessEvent(event);
    });

    window.push_event_cb([&](void*, SDL_Event* event)
    {
         if (event->window.event == SDL_WINDOWEVENT_RESIZED)
            window.set_window_size();
    });

    debug::init(window);

    std::thread emulator_thread = std::thread([&]()
    {
        while (running)
        {
            bus.cpu.run();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    while (running)
    {
        debug::test_demo(window, bus);
        window.poll();
    }
}