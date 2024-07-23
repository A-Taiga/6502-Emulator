#include "emulator.hpp"
#include "SDL2/SDL.h"
#include "SDL2/SDL_events.h"
#include "SDL_scancode.h"
#include "SDL_video.h"
#include "bus.hpp"
#include "common.hpp"
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include "cpu.hpp"
#include "debugger.hpp"
#include "imgui_impl_sdl2.h"
#include "imgui_internal.h"
#include "window.hpp"
#include <filesystem>
#include <fstream>
#include "imgui.h"
#include "IconsFontAwesome6.h"
#include <ios>
#include <fstream>
#include <observer.hpp>

#define WINDOW_W 1920
#define WINDOW_H 1080

namespace
{
    struct callback_data
    {
        UI::Window_interface& window;
        MOS_6502::Emulator& emu;
    };

    template<std::size_t N>
    std::size_t load_rom (const char* path, const std::array<byte, N>& buffer, const std::size_t offset) noexcept
    {
        try
        {
            const std::size_t fileSize = std::filesystem::file_size(path);
            std::fstream file (path, std::ios::in); 
            file >> std::hex;
            file.read((char*)buffer.data()+offset, fileSize);
            file.close();
            return fileSize;
        }
        catch (const std::exception& e) 
        {
            std::cerr << e.what() << '\n';
            std::exit (EXIT_FAILURE);
        }
        return 0;
    }

    template <class ADDRESS_TYPE>
    constexpr std::size_t array_size ()
    {
        return sizeof(ADDRESS_TYPE) * 8 / 4;
    }

    struct Program_Window : public UI::MSG::Observer
    {
        using ADDRESS_TYPE = std::uint16_t;
        using programBuffer = std::vector<std::pair<ADDRESS_TYPE, std::string>>;
        const programBuffer& buffer;
        MOS_6502::CPU* cpu;
        ADDRESS_TYPE pc;
        ImVec2 windowSize;
        char findBuffer[array_size<ADDRESS_TYPE>()];
        struct
        {
            int addressPadding;
        } sizes;
        struct
        {
            ADDRESS_TYPE current_row;
        } db;

        Program_Window (const programBuffer& pb, MOS_6502::CPU* cpu)
        : Observer {}
        , buffer {pb}
        , cpu {cpu}
        , pc (0)
        {
            cpu->attach("Program_Window", this);
            sizes.addressPadding = sizeof(ADDRESS_TYPE) * 8 / 4;
        }
        ~Program_Window()
        {
            cpu->detach("Program_Window");
        }
    
        void draw ()
        {
            ImGui::Begin("Program", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            draw_data();
            ImGui::End();
        };

        void draw_data ()
        {
            auto s = ImGui::GetWindowSize();
            ImGui::BeginChild("programClipper", {0, s.y - ImGui::GetTextLineHeightWithSpacing() * 2}, ImGuiChildFlags_Border);
            ImGuiListClipper clipper;
            clipper.Begin(buffer.size(), ImGui::GetTextLineHeightWithSpacing());
            while (clipper.Step())
            {
                for (ADDRESS_TYPE row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    db.current_row = row;
                    ImGui::TextColored ( buffer[row].first != pc ? ImVec4(255,255,255,255): ImVec4(255,0,0,255),"%s ", buffer[row].second.c_str());
                }
            }
            ImGui::EndChild();
        }

        virtual void Update (UI::MSG::Subject* subject)
        {
            if (subject == cpu)
            {
                pc = cpu->get_PC();
            }
        }
    };

    class Registers_Window : public UI::MSG::Observer
    {
        public:
        Registers_Window (MOS_6502::CPU* cpu)
        : UI::MSG::Observer ()
        , cpu {cpu}
        {
            cpu->attach("registers", this);
        } 
        ~Registers_Window() 
        {
            cpu->detach("registers");
        }

        void draw ()
        {
            ImGui::Begin ("Registers");
            if (ImGui::BeginTable("Regesters Table", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX))
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();

                ImGui::TextUnformatted("Hex");
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Dec");
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Bin");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("PC"); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:04X}", PC).c_str());
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:d}", PC).c_str()); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:08b} {:08b}", PC >> 8, PC & 0xFF).c_str()); 

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("AC"); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:{}}{:02X}"," ",2,AC).c_str());
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:>5d}",AC).c_str()); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:{}} {:08b}", " ", 8, AC).c_str()); 
            
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("X"); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:{}}{:02X}"," ",2,X).c_str());
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:>5d}",X).c_str()); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:{}} {:08b}", " ", 8, X).c_str()); 

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Y"); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:{}}{:02X}"," ",2,Y).c_str());
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:>5d}",Y).c_str()); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:{}} {:08b}", " ", 8, Y).c_str()); 

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted("SP"); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:{}}{:02X}"," ",2,SP).c_str());
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:>5d}",SP).c_str()); 
                ImGui::TableNextColumn(); 
                ImGui::TextUnformatted(std::format ("{:{}} {:08b}", " ", 8, Y).c_str()); 
                ImGui::EndTable();
            }

            if (ImGui::BeginTable("Flags Table", 8, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX))
            {
                ImGui::TableNextColumn();

                ImGui::Dummy(ImGui::CalcTextSize("C"));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32((SR & (1 << 0) ? colors.set : colors.notSet)));
                ImGui::TableNextColumn();
                ImGui::Dummy(ImGui::CalcTextSize("Z"));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32((SR & (1 << 1) ? colors.set : colors.notSet)));
                ImGui::TableNextColumn();
                ImGui::Dummy(ImGui::CalcTextSize("I"));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32((SR & (1 << 2) ? colors.set : colors.notSet)));
                ImGui::TableNextColumn();
                ImGui::Dummy(ImGui::CalcTextSize("D"));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32((SR & (1 << 3) ? colors.set : colors.notSet)));
                ImGui::TableNextColumn();
                ImGui::Dummy(ImGui::CalcTextSize("B"));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32((SR & (1 << 4) ? colors.set : colors.notSet)));
                ImGui::TableNextColumn();
                ImGui::Dummy(ImGui::CalcTextSize("-"));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32((SR & (1 << 5) ? colors.set : colors.notSet)));
                ImGui::TableNextColumn();
                ImGui::Dummy(ImGui::CalcTextSize("V"));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32((SR & (1 << 6) ? colors.set : colors.notSet)));
                ImGui::TableNextColumn();
                ImGui::Dummy(ImGui::CalcTextSize("N"));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32((SR & (1 << 7) ? colors.set : colors.notSet)));

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("C");
                ImGui::TableNextColumn();
                ImGui::Text("Z");
                ImGui::TableNextColumn();
                ImGui::Text("I");
                ImGui::TableNextColumn();
                ImGui::Text("D");
                ImGui::TableNextColumn();
                ImGui::Text("B");
                ImGui::TableNextColumn();
                ImGui::Text("-");
                ImGui::TableNextColumn();
                ImGui::Text("V");
                ImGui::TableNextColumn();
                ImGui::Text("N");
        
                ImGui::EndTable();
            }
            ImGui::End();
        }
        private:
        MOS_6502::CPU* cpu;
        word PC;
        byte AC;
        byte X;
        byte Y;
        byte SR;
        byte SP;

        struct 
        {
            ImVec4 set = {0,255,0,255};
            ImVec4 notSet = {255,255,255,0};
        } colors;

        void Update (UI::MSG::Subject* subject) override
        {
            if (subject == cpu)
            {
                PC = cpu->get_PC ();
                AC = cpu->get_AC ();
                X  = cpu->get_X  ();
                Y  = cpu->get_Y  ();
                SR = cpu->get_SR ();
                SP = cpu->get_SP ();
            }
        }
    };
}

MOS_6502::Emulator::Emulator(const char* filePath)
: currentFile {filePath}
, bus ()
{
    load_rom (filePath, bus.ram.get_ram(), ROM_BEGIN);
    bus.ram[RESET_VECTOR]     = ROM_BEGIN & 0x00FF;
    bus.ram[RESET_VECTOR + 1] = (ROM_BEGIN & 0xFF00) >> 8;
}

namespace
{
    struct Callback_Data
    {
        UI::Window_interface& window;
        MOS_6502::Emulator& emu;
        bool& running;
        bool& pause;
        bool& step;
    };

    struct Poll_Data
    {
        UI::Window_interface& window;
        MOS_6502::Emulator& emu;
    };

    void ui_callback (SDL_Event *event, void *uData)
    {
        ImGui_ImplSDL2_ProcessEvent(event);
        static word index = KEY_BEGIN+2;
        Poll_Data * data = static_cast <Poll_Data*> (uData);
        UI::Window_interface &window = data->window;

        if (event->type == SDL_KEYDOWN)
        {
            const char key = window.to_ASCII(event->key.keysym.scancode);
            if (key == 8 && index)
            {
                index -= 1;
                if (index == KEY_BEGIN+1)
                    index = KEY_BEGIN+2;
                data->emu.bus.ram[KEY_BEGIN] = index & 0x00FF;
                data->emu.bus.ram[KEY_BEGIN+1] =  (index >> 8) & 0x00FF;
                data->emu.bus.ram[index] = 0x0000;
            }
            else
            {
                data->emu.bus.ram[KEY_BEGIN] = index & 0x00FF;
                data->emu.bus.ram[KEY_BEGIN+1] =  (index >> 8) & 0x00FF;
                data->emu.bus.ram[index++] = key;
            }

            data->emu.bus.cpu.set_irq_pin();
        }
    }
}

void MOS_6502::Emulator::run()
{
    bool running = true;
    bool pause = true;
    bool step = false;
    UI::OS_window window ("Debugger", WINDOW_W, WINDOW_H, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_INIT_EVERYTHING);
    UI::Debugger debugger (window);
    Callback_Data callbackData {window, *this, running, pause, step};
    Poll_Data pollData {window, *this};
    bus.cpu.decompiler();
    bus.cpu.reset();

    while (running)
    {
        running = UI::poll(window, ui_callback, &pollData);
        bus.cpu.run();
        debugger.run(&MOS_6502::Emulator::impl_ui, &callbackData);
        if (!pause)
        {
            bus.cpu.run();
            bus.cpu.notify();   
        }
    }
}

void MOS_6502::Emulator::reset ()
{
    bus.ram.reset();
    load_rom (currentFile.data(), bus.ram.get_ram(), ROM_BEGIN);
    bus.ram[RESET_VECTOR]     = (byte)0x00;
    bus.ram[RESET_VECTOR + 1] = (byte)0xF0;
    bus.cpu.reset();
}

void MOS_6502::Emulator::impl_ui (void* uData)
{
    static Callback_Data* data = static_cast <Callback_Data*> (uData);
    static Program_Window   programWindow {data->emu.bus.cpu.decompiled_code, &data->emu.bus.cpu};
    static UI::Hex_editor   Memory        {"RAM", data->emu.bus.ram.get_ram().data(), RAM_SIZE, 0, RAM_SIZE, sizeof(std::uint8_t)};
    static UI::Hex_editor   zeroPage      {"Zero Page", data->emu.bus.ram.get_ram().data(), RAM_SIZE, 0, PAGE_SIZE, sizeof (std::uint8_t)};
    static UI::Hex_editor   Page1         {"Page 1", data->emu.bus.ram.get_ram().data(), RAM_SIZE, 0x0200, PAGE_SIZE, sizeof (std::uint8_t)};
    static UI::Hex_editor   StackPage     {"Stack", data->emu.bus.ram.get_ram().data(), RAM_SIZE, 0x0100, PAGE_SIZE, sizeof (std::uint8_t)};
    static Registers_Window registers     {&data->emu.bus.cpu};
    
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ParentViewportId, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    Memory.draw();
    zeroPage.draw();
    Page1.draw();
    StackPage.draw();
    programWindow.draw();
    registers.draw();

    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);
    ImGui::SetNextWindowSize({static_cast<float>(data->window.get_width()),0});
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    static int v = 100;
    ImGui::SetNextItemWidth(100);
    if (ImGui::DragInt("##milliseconds delay", &v, 1, 1, 1000, "%d", ImGuiSliderFlags_AlwaysClamp))
    {
        // delay = std::chrono::milliseconds(v);
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_ROTATE_LEFT))
    {
        data->emu.reset();
    }
    ImGui::SameLine();
    if (ImGui::Button((data->pause ? ICON_FA_PLAY: ICON_FA_PAUSE)))
    {
        if (data->pause)
        {
            data->pause = false;
            data->step = false;
        }
        else
            data->pause = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_RIGHT))
    {
        data->step = true;
        if (!data->pause)
            data->pause = true;
        data->emu.bus.cpu.run();
    }
    ImGui::End();
}