#include "emulator.hpp"
#include "common.hpp"
#include "imgui.h"
#include <format>
#include <cstring>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <array>
#include "imgui_internal.h"
#include "window.hpp"
#include "SDL_events.h"


#define WINDOW_W 1000
#define WINDOW_H 1000
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

    void draw_text_rect(std::string_view str, ImU32 color) 
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        assert (window != nullptr);

        ImDrawList* drawList = window->DrawList;

        ImVec2 max = ImGui::CalcTextSize(str.data());
        ImVec2 min = ImGui::GetCursorScreenPos();

        drawList->AddRectFilled(min, ImVec2(min.x + max.x, min.y + max.y), color);
        
        ImGui::TextUnformatted(str.data());
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

    // srand (time(NULL));
    // static std::array<char, 65536> bus.ram = {0};
    
    // for (auto& i : bus.ram)
    //     i = rand() % 255;

    OS_Window window ("test", WINDOW_W, WINDOW_H);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL2_InitForOpenGL(window.get_window(), window.get_glContext());
    ImGui_ImplOpenGL3_Init(window.get_glslVersion());


    float textSize = 20.0f;
    ImFont* font = io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/Cousine-Regular.ttf", textSize, nullptr, io.Fonts->GetGlyphRangesDefault());

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool running = true;

    while (running)
    {

        window.poll ([&](SDL_Event& event) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
            
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == window.get_windowID())
                running = false;
        });
        if (!running) break;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        bool popup = false;

        static ImVec2 menuBarSize;
        if (ImGui::BeginMainMenuBar())
        {
            menuBarSize = ImGui::GetWindowSize();
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Settings")) 
                {
                    popup = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        if (popup) 
            ImGui::OpenPopup("Settings");

        if (ImGui::BeginPopupModal("Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::InputFloat("Text size", &textSize, .5, 0, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
            {
                font->FontSize = textSize;
            }
            if (ImGui::Button("Close")) {ImGui::CloseCurrentPopup();}
            ImGui::EndPopup();
        }
        
//////////////////////////////////////////////////////////////////////////////////////////////// memory window

        static ImU32 highLightColor = IM_COL32(255,0,0,255);
        static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        static ImGuiListClipper clipper;
        static char buffer[5] = {0};
        static int scroll_val = -1;
        bool input_address = false;


        ImGui::SetNextWindowSize({0,0});
        ImGui::SetNextWindowPos({0,menuBarSize.y});
        ImGui::Begin("Memory", nullptr, windowFlags);

        if (ImGui::InputText("##page input", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
            input_address = true;
        ImGui::SameLine();
        if (ImGui::Button("Go")) 
            input_address = true;

        ImGui::Dummy (ImGui::CalcTextSize("0000:"));
        ImGui::SameLine();
        for (std::uint8_t i = 0; i < 16; i++)
        {
            if (i != 0 && i % 8 == 0)
            {
                ImGui::TextUnformatted(" ");
                ImGui::SameLine();
            }
            ImGui::Text ("%02X", i);
            ImGui::SameLine();
        }
        ImGui::Dummy ({ImGui::CalcTextSize("  ").x, 0});
        ImGui::SameLine();
        ImGui::Dummy ({ImGui::CalcTextSize(" ").x * 16, 0});
        ImGui::Separator();
        ImGui::BeginChild ("list",{0, ImGui::GetTextLineHeightWithSpacing()* 16});
        clipper.Begin(RAM_SIZE / 16, ImGui::GetTextLineHeightWithSpacing());
        if (input_address)
        {
            int val = std::strtol(buffer, nullptr, 16);
            ImGui::SetScrollY((int)(val / 16) * ImGui::GetTextLineHeightWithSpacing());
            scroll_val = val;
        }
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {

                ImGui::Text("%04X:", (std::uint16_t)row*16);
                for (std::size_t i = 0; i < 16; i++)
                {
                    std::size_t index = (row*16) + i;
                    std::uint8_t value = bus.ram[index];
                    
                    if (i != 0 && i % 8 == 0)
                    {
                        ImGui::SameLine();
                        ImGui::Text (" ");
                    }
                    ImGui::SameLine();
                    if (index == (std::size_t)scroll_val)
                        draw_text_rect(std::format("{:02X}", value), highLightColor);
                    else
                        ImGui::Text ("%02X", value);
                }
                ImGui::SameLine();
                ImGui::Dummy(ImGui::CalcTextSize(" "));
                ImGui::SameLine();
                for (std::size_t i = 0; i < 16; i++)
                {
                    ImGui::SameLine(0,0);
                    if ((row*16) + i == (std::size_t)scroll_val)
                    {
                        if (bus.ram[(row*16) + i] >= 32)
                            draw_text_rect(std::format("{:}", (char)bus.ram[(row*16)+i]), highLightColor);
                        else
                            draw_text_rect(".", IM_COL32(255,0,0,255));
                    }
                    else
                    {
                        if (bus.ram[(row*16) + i] >= 32)
                            ImGui::Text("%c", bus.ram[(row*16)+i]);
                        else
                            ImGui::Text(".");
                    }
                }
            }
        }
        ImGui::EndChild();
        ImGui::End();

        /* debugger */
        ImGui::Begin("Debugger", nullptr);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text ("clipper.DisplayStart : %d", clipper.DisplayStart);
        ImGui::Text ("clipper.DisplayEnd   : %d", clipper.DisplayEnd);
        ImGui::Text ("scroll_val : %d", scroll_val);
        ImGui::Text ("ImGui::GetTextLineHeightWithSpacing() : %f", ImGui::GetTextLineHeightWithSpacing());
        ImGui::End();
        /* end debugger */

        // Rendering
        ImGui::Render();
        window.render(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y, clear_color);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
        
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }
        // window.swap_window();
        SDL_GL_SwapWindow(window.get_window());

        bus.cpu.run();

    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

}