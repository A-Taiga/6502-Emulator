#include "emulator.hpp"
#include "SDL2/SDL_events.h"
#include "common.hpp"
#include "imgui_impl_sdl2.h"
#include "window.hpp"
#include "debug.hpp"


_6502::Emulator::Emulator(const char* filePath, bool& _running)
: mem(filePath)
, cpu()
, running(_running)
{}

void _6502::Emulator::run()
{
    static debug::Data data = {cpu, mem};
    static Window window(600, 600, "Debugger");

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

    // cpu.start();
    while (running)
    {
        debug::test_demo(window, data);
        window.poll();
    }
}