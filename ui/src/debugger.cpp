#include "debugger.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <filesystem>
#include <immintrin.h>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include "hex_editor.h"
#include "imgui_internal.h"
#include "mos6502.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif


namespace
{
    // idk how else to do this
    static auto roms_path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().string() + "/roms/";
    static auto font_path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().string() + "/imgui/misc/fonts/Cousine-Regular.ttf";
    static constexpr std::array <const char*, 14> register_names = {"PC", "AC", "XR", "YR", "SP", " ", "C", "Z", "I", "D", "B", "_", "V", "N"};
    static constexpr std::array <const char*, 14> format_strings = {"%04X", "%02X", "%02X", "%02X", "%02X", "%c", "%d", "%d", "%d", "%d", "%d", "%d", "%d", "%d"};
}

void GUI::registers ()
{
    
    static constexpr int table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
    const auto n = register_names.size();
    ImGui::Begin("Registers");

    if (ImGui::BeginTable("Registers Table", n, table_flags))
    {
        ImGui::TableNextRow();
        for (std::size_t i = 0; i < n; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImGui::Text("%s", register_names[i]);
        }
       
        ImGui::TableNextRow();
        for (std::size_t i = 0; i < n; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImGui::Text(format_strings[i], register_callbacks[i]());
        }

        ImGui::EndTable();
    }
    ImGui::End();
}

void GUI::code_window ()
{
    ImGui::Begin("code", 0);
    if (ImGui::BeginTable("##code table", 1,  ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg))
    {
        ImGuiListClipper clipper;
        clipper.Begin(code.size(), ImGui::GetTextLineHeightWithSpacing());

        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (code[row].first == cpu.get_PC())
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 255, 0, 100));
                ImGui::Text("%s", code[row].second.c_str());
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void GUI::trace_window ()
{

    static std::size_t prev_size = 0;

    ImGui::Begin("trace", 0, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    
    if (ImGui::BeginTable("##trace table", 6, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
    
        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
        for (const auto& c : std::span (register_names.cbegin(), register_names.cend()).subspan(0, 5))
            ImGui::TableSetupColumn(c);
        ImGui::TableSetupColumn("Code");
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin(trace.size());
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                try 
                {
                    const auto& t = trace.get_trace_v().at(row);
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(t.at(0).c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(t.at(1).c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(t.at(2).c_str());

                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(t.at(3).c_str());

                    ImGui::TableSetColumnIndex(4);
                    ImGui::TextUnformatted(t.at(4).c_str());

                    ImGui::TableSetColumnIndex(5);
                    ImGui::TextUnformatted(t.at(5).c_str());
                } 
                catch (std::out_of_range& e)
                {
                    std::cerr << e.what() << '\n';
                }
            }
        }

        // scroll to bottom when running
        if (trace.size() != prev_size)
        {

            ImGui::SetScrollY(trace.size() * 255);
            prev_size = trace.size();
        }
        
        ImGui::EndTable();
    }
    ImGui::End();
}


void GUI::rom_select_box ()
{
    const std::string preview_value = !current_rom ? "" : current_rom->file_name;
    if (ImGui::BeginCombo("##", preview_value.c_str(), ImGuiComboFlags_PopupAlignLeft))
    {
        for (auto& entry : roms)
        {
            if (ImGui::Selectable(entry.file_name.c_str(), &entry == current_rom))
            {
                current_rom = &entry;
            }

            if (&entry == current_rom)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void GUI::action_bar ()
{
    static std::string button_label = "PLAY";
    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);
    ImGui::Begin("action bar", 0,ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
    
    rom_select_box();

    ImGui::SameLine();

    if (ImGui::Button("Reset"))
    {
        rom.reset();
        ram.reset();
        rom.load(current_rom->file_path, current_rom->file_size);
        cpu.reset();
        trace.reset();

        if (rom.is_loaded())
            code = MOS_6502::disassembler(rom.get_rom(), 0x7000);
        else
            printf("ERROR\n");
    }

    ImGui::SameLine();

    if(ImGui::Button(button_label.c_str()))
    {
        {
            std::lock_guard<std::mutex> lock (mu);
            is_paused = !is_paused;
            if (is_paused)
                button_label = "PLAY";
            else
                button_label = "PAUSE";
        }
        cv.notify_all();
    }

    ImGui::SameLine();

    if (ImGui::Button(">"))
    {
        std::lock_guard <std::mutex> lock (mu);
        if (is_paused)
            step = true;
    }

    cv.notify_all();

    ImGui::End();
}

GUI::GUI (MOS_6502::CPU& _cpu, MOS_6502::CPU_Trace& _trace, Memory::ROM& _rom, Memory::RAM& _ram)
: window {"6502 Emulator", 1920, 1080}
, cpu {_cpu}
, trace {_trace}
, rom {_rom}
, ram {_ram}
, code {MOS_6502::disassembler(rom.get_rom(), 0x7000)}
, current_rom {nullptr}
, roms {}
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    for (const auto& entry : std::filesystem::recursive_directory_iterator(roms_path))
    {
        roms.emplace_back (
                            entry.path().filename().string(), 
                            entry.path().string(), 
                            entry.file_size()
                          );
    }

    register_callbacks =
    {
        [&](){return cpu.get_PC();},
        [&](){return cpu.get_AC();},
        [&](){return cpu.get_XR();},
        [&](){return cpu.get_YR();},
        [&](){return cpu.get_SP();},
        [](){return ' ';},
        [&](){return (cpu.get_SR() >> 0) & 1;},
        [&](){return (cpu.get_SR() >> 1) & 1;},
        [&](){return (cpu.get_SR() >> 2) & 1;},
        [&](){return (cpu.get_SR() >> 3) & 1;},
        [&](){return (cpu.get_SR() >> 4) & 1;},
        [&](){return (cpu.get_SR() >> 5) & 1;},
        [&](){return (cpu.get_SR() >> 6) & 1;},
        [&](){return (cpu.get_SR() >> 7) & 1;},
    };
}

void GUI::run ()
{
    auto& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.Fonts->AddFontFromFileTTF(font_path.c_str(), 25);
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window.get_window (), window.get_gl_context ());
    ImGui_ImplOpenGL3_Init(window.get_glsl_version ().c_str());

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    

    const auto rom_n = rom.get_rom().size();
    const auto ram_n = ram.get_ram().size();

    Hex_Editor rom_data ("ROM", rom_n, 0, rom_n, sizeof(std::uint8_t), rom.get_rom().data());
    Hex_Editor ram_data ("RAM", ram_n, 0, ram_n, sizeof(std::uint8_t), ram.get_ram().data());

    Hex_Editor stack_page ("Stack page", ram_n, 0x0100, 256, sizeof(std::uint8_t), ram.get_ram().data());
    Hex_Editor zero_page  ("Zero page", ram_n, 0x0, 256, sizeof(std::uint8_t), ram.get_ram().data());
    
    while (window.is_running())
    {
        window.poll ([&] (auto& event) {
            ImGui_ImplSDL2_ProcessEvent  (&event);
        });

        if (SDL_GetWindowFlags(window.get_window ()) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay (10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport();

        action_bar();
        rom_data.present();
        ram_data.present();
        stack_page.present();
        zero_page.present();
        code_window();
        registers();
        trace_window();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window.get_window ());
    }

    {
        std::lock_guard<std::mutex> lock (mu);
        is_paused = false;
        step = true;
    }
    cv.notify_all();
}