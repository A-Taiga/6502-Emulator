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
#include <mutex>
#include "hex_editor.h"
#include "imgui_internal.h"
#include "mos6502.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif


void registers (const emulator_data& data)
{
    
    static constexpr std::array<const char*, 14> cols = {"PC", "AC", "XR", "YR", "SP", " ", "C", "Z", "I", "D", "B", "_", "V", "N"};
    static constexpr int table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
    
    ImGui::Begin("Registers");
    const std::uint8_t SR = data.cpu.get_SR();

    ImGui::BeginTable("Registers Table", cols.size(), table_flags);
    ImGui::TableNextRow();
    for (std::size_t i = 0; i < cols.size(); ++i)
    {
        ImGui::TableSetColumnIndex(i);
        ImGui::Text("%s", cols[i]);
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%04X", data.cpu.get_PC());
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%02X", data.cpu.get_AC());
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%02X", data.cpu.get_XR());
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%02X", data.cpu.get_YR());
    ImGui::TableSetColumnIndex(4);
    ImGui::Text("%02X", data.cpu.get_SP());
    
    int bit = 0;
    for (std::size_t i = 6; i < cols.size(); ++i)
    {
        ImGui::TableSetColumnIndex(i);
        ImGui::Text("%d", (SR >> bit) & 1);
        ++bit;
    }
    ImGui::EndTable();
    ImGui::End();
}

void code_window (const emulator_data& data)
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

void trace_window (const emulator_data& data)
{

    static std::size_t prev_size = 0;

    static constexpr std::array <const char*, 6> columns = {" PC ", " AC ", " XR ", " YR ", " SP ", " Code "};
    ImGui::Begin("trace", 0, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    
    ImGui::BeginTable("##trace table", 6, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

    ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
    for (const auto& c : columns)
    {
        ImGui::TableSetupColumn(c);
    }
    
    ImGui::TableHeadersRow();
    ImGuiListClipper clipper;
    clipper.Begin(data.trace.size());
                // ImGui::Text("%04X %02X %02X %02X %02X | %s", t.PC, t.AC, t.XR, t.YR, t.SP, t.code.c_str());
    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            const auto& t = data.trace[row];
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text(" %04X ", t.PC);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text(" %02X ", t.AC);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text(" %02X ", t.XR);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text(" %02X ", t.YR);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text(" %02X ", t.SP);

            ImGui::TableSetColumnIndex(5);
            ImGui::Text(" %s", t.code.c_str());
        }
    }

    // scroll to bottom when running
    if (data.trace.size() != prev_size)
    {

        ImGui::SetScrollY(data.trace.size() * 255);
        prev_size = data.trace.size();
    }
    
    ImGui::EndTable();
    ImGui::End();
}


// void rom_select_box (emulator_data& data)
// {
//     const std::string preview_value = !data.current_rom ? "" : data.current_rom->path().filename();
//     if (ImGui::BeginCombo("##", preview_value.c_str(), ImGuiComboFlags_PopupAlignLeft))
//     {
//         for (auto& entry : data.roms)
//         {
//             if (ImGui::Selectable(entry.path().filename().c_str(), &entry == data.current_rom))
//             {
//                 data.current_rom = &entry;
//             }

//             if (&entry == data.current_rom)
//                 ImGui::SetItemDefaultFocus();
//         }
//         ImGui::EndCombo();
//     }
// }

void action_bar (GUI& gui, [[maybe_unused]] emulator_data& data)
{
    static std::string button_label = "PLAY";
    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);
    ImGui::Begin("action bar", 0,ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
    
    // rom_select_box(data);

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

GUI::GUI (const char* title, const int width, const int height, emulator_data data)
: window {title, width, height}
, emu_data {data}
{
    std::string roms_folder_path {std::filesystem::current_path().string() + "/roms"};

    if (!std::filesystem::exists(roms_folder_path))
    {
        printf("no roms folder found\n");
        std::exit (EXIT_FAILURE);
    }

    for (const auto & entry : std::filesystem::directory_iterator(roms_folder_path))
        emu_data.roms.push_back (entry);
        

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

    Hex_Editor rom_data ("Memory", emu_data.memory.size(), 0, emu_data.memory.size(), sizeof(std::uint8_t), emu_data.memory.data());

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