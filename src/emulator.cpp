#include "emulator.hpp"
#include "SDL2/SDL_events.h"
#include "common.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include "cpu.hpp"
#include "debugger.hpp"
#include "imgui_impl_sdl2.h"
#include "window.hpp"
#include <thread>
#include <filesystem>
#include <fstream>

#define WINDOW_W 1920
#define WINDOW_H 1080

namespace
{
    template<std::size_t N>
    std::size_t load_rom (const std::string_view path, const std::array<byte, N>& buffer, const std::size_t offset)
    {
        try
        {
            const std::size_t fileSize = std::filesystem::file_size(path);
            std::fstream file (path, std::ios::hex);
            file.read((char*)buffer.data()+offset, fileSize);
            file.close();
            return fileSize;
        }
        catch (const std::exception& e) {std::cerr << e.what() << '\n';}
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
    OS_Window window ("Debugger", WINDOW_W, WINDOW_H);
    bus.cpu.reset();
    using namespace std::chrono_literals;
    UI::debug_v debugData = {bus, 100ms, true, true, false,[&](){reset();}, window};
    UI::init(window);
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
        window.poll(debugData.running, [&](const SDL_Event& event){
            ImGui_ImplSDL2_ProcessEvent(&event);
            // if (event.type == SDL_KEYDOWN)
            // {
            //     switch (event.key.keysym.scancode)
            //     {
            //         case SDL_SCANCODE_0: bus.ram[keyBufferIndex] = 0; keyBufferIndex++; break;
            //         case SDL_SCANCODE_1: bus.ram[keyBufferIndex] = 1; keyBufferIndex++; break;
            //         case SDL_SCANCODE_2: bus.ram[keyBufferIndex] = 2; keyBufferIndex++; break;
            //         case SDL_SCANCODE_3: bus.ram[keyBufferIndex] = 3; keyBufferIndex++; break;
            //         case SDL_SCANCODE_4: bus.ram[keyBufferIndex] = 4; keyBufferIndex++; break;
            //         case SDL_SCANCODE_5: bus.ram[keyBufferIndex] = 5; keyBufferIndex++; break;
            //         case SDL_SCANCODE_6: bus.ram[keyBufferIndex] = 6; keyBufferIndex++; break;
            //         case SDL_SCANCODE_7: bus.ram[keyBufferIndex] = 7; keyBufferIndex++; break;
            //         case SDL_SCANCODE_8: bus.ram[keyBufferIndex] = 8; keyBufferIndex++; break;
            //         case SDL_SCANCODE_9: bus.ram[keyBufferIndex] = 9; keyBufferIndex++; break;
            //         default:;
            //     }
            // }
        });
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



