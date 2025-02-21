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
#include "mem.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif


namespace
{
    // idk how else to do this...
    static auto roms_path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().string() + "/roms/";
    static auto font_path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().string() + "/imgui/misc/fonts/Cousine-Regular.ttf";

    static auto temp = std::array<std::uint8_t, UINT16_MAX>();
    static Hex_Editor rom_data ("ROM", UINT16_MAX, 0, UINT16_MAX, sizeof(std::uint8_t), temp.data());
    static Hex_Editor ram_data ("RAM",  UINT16_MAX, 0, UINT16_MAX, sizeof(std::uint8_t), temp.data());
    static Hex_Editor stack_page ("Stack page", UINT16_MAX, 0, UINT16_MAX, sizeof(std::uint8_t), temp.data());
    static Hex_Editor zero_page  ("Zero page",  UINT16_MAX, 0, UINT16_MAX, sizeof(std::uint8_t), temp.data());
}

void GUI::registers ()
{
    static constexpr std::array <const char*, 14> register_names = {"XR", "YR", "AC", "SP", "PC", " ", "N","V","_","B","D","I","Z","C"};
    static constexpr std::array <const char*, 14> format_strings = {"%02X", "%02X", "%02X", "%02X", "%04X", "%c", "%d", "%d", "%d", "%d", "%d", "%d", "%d", "%d"};

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
            int val = register_callbacks[i]();
            if (i >= 6 && val == 1)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 255, 0, 100));
            ImGui::Text(format_strings[i], val);
        }

        ImGui::EndTable();
    }
    ImGui::End();
}

void GUI::code_window ()
{
    ImGui::Begin("Code", 0);
    if (ImGui::BeginTable("##code table", 2,  ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("BRK", ImGuiTableColumnFlags_WidthFixed);
        ImGuiListClipper clipper;
        clipper.Begin(code.size());

        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(row);
                ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 0, 0, 255));
                if (ImGui::RadioButton("##xx", std::get<2>(code[row])))
                {
                    auto& brk = std::get<2>(code[row]);
                    brk = !brk;
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
                ImGui::TableSetColumnIndex(1);

                if (std::get<0>(code[row]) == (0x7FFF & cpu.get_PC())) // TODO let user set entry point of 0x7000
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 255, 0, 100));
                ImGui::Text("%s", std::get<1>(code[row]).c_str());
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void GUI::trace_window ()
{
    static constexpr std::array <const char*, 14> cols = {" Code ", " XR ", " YR ", " AC ", " SP ", " PC ", "N","V","_","B","D","I","Z","C"};
    static std::size_t prev_size = 0;


    ImGui::Begin("Trace", 0, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    
    if (ImGui::BeginTable("##trace table", 14, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
    
        ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
        for (const auto& c : cols)
            ImGui::TableSetupColumn(c);
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin(traces.size());
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                try 
                {
                    const auto& t = traces.at(row);
                    ImGui::TableNextRow();
                    for (int i = 0; i < (int)t.size(); ++i)
                    {
                        ImGui::TableSetColumnIndex(i);
                        if (i >= 6 && t.at(i) == "1")
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(0, 255, 0, 100));
                        ImGui::TextUnformatted(t.at(i).c_str());
                    }
                } 
                catch (std::out_of_range& e)
                {
                    std::cerr << e.what() << '\n';
                }
            }
        }

        // scroll to bottom when running
        if (traces.size() != prev_size)
        {
            ImGui::SetScrollY(traces.size() * 255);
            prev_size = traces.size();
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

    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);
    ImGui::Begin("action bar", 0,ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
    
    rom_select_box();

    ImGui::SameLine();


    // TODO: probably make this thread safe
    if (ImGui::Button("Reset"))
    {
        rom.reset();
        ram.reset();
        rom.load(current_rom->file_path, current_rom->file_size);
        cpu.reset();
        if (rom.is_loaded())
        {

            code = MOS_6502::disassemble(rom, 0x7000);  // TODO let user select offset
            code_map = MOS_6502::code_mapper(code);
            traces = {};
        }
        else
            printf("ERROR\n");
        
        rom_data   = Hex_Editor("ROM", rom.size(), 0, rom.size(), sizeof(std::uint8_t), rom.data());
        ram_data   = Hex_Editor("RAM", ram.size(), 0, ram.size(), sizeof(std::uint8_t), ram.data());

        stack_page = Hex_Editor("Stack page", ram.size(), 0x0100, 256, sizeof(std::uint8_t), ram.data());
        zero_page  = Hex_Editor("Zero page",  ram.size(), 0x0, 256, sizeof(std::uint8_t), ram.data());

    }

    ImGui::SameLine();

    if(ImGui::Button(is_paused ? "PLAY" : "PAUSE"))
    {
        if (rom.is_loaded())
        {
            std::lock_guard<std::mutex> lock (mu);
            is_paused = !is_paused;
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

GUI::GUI (MOS_6502::CPU& _cpu, Memory& _rom, Memory& _ram, MOS_6502::trace_type& _traces, MOS_6502::code_map_type& _code_map)
: window {"6502 Emulator", 1920, 1080}
, cpu {_cpu}
, rom {_rom}
, ram {_ram}
, traces {_traces}
, code_map {_code_map}
, code {}
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
        [&](){return cpu.get_XR();},
        [&](){return cpu.get_YR();},
        [&](){return cpu.get_AC();},
        [&](){return cpu.get_SP();},
        [&](){return cpu.get_PC();},
        [](){return ' ';},
        [&](){return (cpu.get_SR() >> 7) & 1;},
        [&](){return (cpu.get_SR() >> 6) & 1;},
        [&](){return (cpu.get_SR() >> 5) & 1;},
        [&](){return (cpu.get_SR() >> 4) & 1;},
        [&](){return (cpu.get_SR() >> 3) & 1;},
        [&](){return (cpu.get_SR() >> 2) & 1;},
        [&](){return (cpu.get_SR() >> 1) & 1;},
        [&](){return (cpu.get_SR() >> 0) & 1;},
    };
}

void GUI::run ()
{
    auto& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.Fonts->AddFontFromFileTTF(font_path.c_str(), 24);
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window.get_window (), window.get_gl_context ());
    ImGui_ImplOpenGL3_Init(window.get_glsl_version ().c_str());

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
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