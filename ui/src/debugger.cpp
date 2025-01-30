#include "debugger.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <immintrin.h>
#include <mutex>
#include "hex_editor.h"
#include "imgui_internal.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif


void registers (const Emulator_state& data)
{
    
    static constexpr int table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
    const auto n = data.register_names.size();
    ImGui::Begin("Registers");
    // const std::uint8_t SR = data.cpu.get_SR();

    if (ImGui::BeginTable("Registers Table", n, table_flags))
    {
        ImGui::TableNextRow();
        for (std::size_t i = 0; i < n; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImGui::Text("%s", data.register_names[i]);
        }

        ImGui::TableNextRow();

        for (std::size_t i = 0; i < n; ++i)
        {
            ImGui::TableSetColumnIndex(i);
            ImGui::Text(data.format_strings[i], data.register_callbacks[i]());
        }

        ImGui::EndTable();
    }
    ImGui::End();
}

void code_window (const Emulator_state& data)
{
    ImGui::Begin("code", 0);
    ImGuiListClipper clipper;

    clipper.Begin(data.code.size(), ImGui::GetTextLineHeightWithSpacing());

    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
        {
            ImGui::Text("%s",data.code[row].c_str());
        }
    }
    ImGui::End();
}

void trace_window (const Emulator_state& data)
{

    static std::size_t prev_size = 0;

    static constexpr std::array <const char*, 6> columns = {" PC ", " AC ", " XR ", " YR ", " SP ", " Code "};
    ImGui::Begin("trace", 0, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    
    if (ImGui::BeginTable("##trace table", 6, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
    
    ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
    for (const auto& c : columns)
    {
        ImGui::TableSetupColumn(c);
    }
    ImGui::TableHeadersRow();
    ImGuiListClipper clipper;
    clipper.Begin(data.trace.size());
    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            const auto& t = data.trace[row];
            // auto a =data.trace[row][0];
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(t[0].c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(t[1].c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(t[2].c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(t[3].c_str());

            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(t[4].c_str());

            ImGui::TableSetColumnIndex(5);
            ImGui::TextUnformatted(t[5].c_str());
        }
    }

    // scroll to bottom when running
    if (data.trace.size() != prev_size)
    {

        ImGui::SetScrollY(data.trace.size() * 255);
        prev_size = data.trace.size();
    }
    
        ImGui::EndTable();
    }
    ImGui::End();
}


void rom_select_box (Emulator_state& data)
{
    const std::string preview_value = !data.current_rom ? "" : data.current_rom->file_name;
    if (ImGui::BeginCombo("##", preview_value.c_str(), ImGuiComboFlags_PopupAlignLeft))
    {
        for (auto& entry : data.roms)
        {
            if (ImGui::Selectable(entry.file_name.c_str(), &entry == data.current_rom))
            {
                data.current_rom = &entry;
            }

            if (&entry == data.current_rom)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void action_bar (GUI& gui, Emulator_state& data)
{
    static std::string button_label = "PLAY";
    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);
    ImGui::Begin("action bar", 0,ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
    
    rom_select_box(data);

    ImGui::SameLine();


    if(ImGui::Button(button_label.c_str()))
    {
        {
            std::lock_guard<std::mutex> lock (gui.mu);
            gui.is_paused = !gui.is_paused;
            if (gui.is_paused)
                button_label = "PLAY";
            else
                button_label = "PAUSE";
        }
        gui.cv.notify_all();
    }

    ImGui::SameLine();

    if (ImGui::Button(">"))
    {
        std::lock_guard <std::mutex> lock (gui.mu);
        if (gui.is_paused)
            gui.step = true;
    }
    gui.cv.notify_all();

    ImGui::End();
}

GUI::GUI (const char* title, const int width, const int height, Emulator_state& data)
: window {title, width, height}
, emu_data {data}
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
}

void GUI::run ()
{
    auto& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.Fonts->AddFontFromFileTTF("/home/anthony/Workspace/cpp/6502/ui/Cousine-Regular.ttf", 25);

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window.get_window (), window.get_gl_context ());
    ImGui_ImplOpenGL3_Init(window.get_glsl_version ().c_str());

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    

    const auto rom_n = emu_data.rom.size();
    const auto ram_n = emu_data.ram.size();

    Hex_Editor rom_data ("ROM", rom_n, 0, rom_n, sizeof(std::uint8_t), emu_data.rom.data());
    Hex_Editor ram_data ("RAM", ram_n, 0, ram_n, sizeof(std::uint8_t), emu_data.ram.data());
    
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

        action_bar(*this, emu_data);
        rom_data.present();
        ram_data.present();
        code_window(emu_data);
        registers(emu_data);
        trace_window(emu_data);


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