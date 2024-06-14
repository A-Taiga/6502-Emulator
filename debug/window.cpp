#include "window.hpp"
#include "SDL_events.h"
#include "SDL_hints.h"
#include "SDL_video.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdexcept>
#include <imgui.h>
#include <functional>
#include "bus.hpp"
#include "imgui_internal.h"
#include <format>

#define WINDOW_FLAGS SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI

OS_Window::OS_Window (const char* title, int w, int h)
: width (w), height (h)
{

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) throw std::runtime_error (SDL_GetError());

    #if defined (__APPLE__)
        // GL 3.2 Core + GLSL 150
        glslVersion = "#version 150";
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 2);
    #else
        glslVersion = "#version 130";
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #endif

    #ifdef SDL_HINT_IME_SHOW_UI
        SDL_SetHint (SDL_HINT_IME_SHOW_UI, "1");
    #endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, WINDOW_FLAGS);
    if (window == nullptr) throw std::runtime_error (SDL_GetError());

    glContext = SDL_GL_CreateContext (window);
    SDL_GL_MakeCurrent (window, glContext);
    SDL_GL_SetSwapInterval (1); // enables vsync
}

OS_Window::~OS_Window ()
{
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void OS_Window::render (int a, int b, int x, int y, const ImVec4& color)
{
    glViewport(a, b, x, y);
    glClearColor(color.x * color.w, color.y * color.w, color.z * color.w, color.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OS_Window::poll (std::function<void(SDL_Event&)> callback)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            SDL_GetWindowSize(window, &width, &height);
        callback (event);
    }
}

void OS_Window::swap_window () { SDL_GL_SwapWindow(window);}
SDL_Window* OS_Window::get_window () { return window;}
SDL_GLContext OS_Window::get_glContext () { return glContext;}
const char* OS_Window::get_glslVersion () { return glslVersion;}
std::uint32_t OS_Window::get_windowID () { return SDL_GetWindowID (window);}




namespace
{
    ImFont* font;
    float textSize;
    ImVec4 clear_color;

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

void UI::init (OS_Window& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
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


    textSize = 20.0f;
    font = io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/Cousine-Regular.ttf", textSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

}

void UI::end ()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}




void UI::debug(OS_Window& window, _6502::Bus& bus, bool& running)
{
    static ImGuiIO& io = ImGui::GetIO(); (void)io;


    window.poll ([&](SDL_Event& event) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            running = false;
        
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == window.get_windowID())
            running = false;
    });
    if (!running) return;

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
        static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        static ImGuiListClipper clipper;
        static char buffer[5] = {0};
        static int scroll_val = -1;
        bool input_address = false;


        ImGui::SetNextWindowSize({0,0});
        // ImGui::SetNextWindowPos({0,menuBarSize.y});
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
        /* window debugger */
        ImGui::Begin("Debugger", nullptr);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text ("clipper.DisplayStart : %d", clipper.DisplayStart);
        ImGui::Text ("clipper.DisplayEnd   : %d", clipper.DisplayEnd);
        ImGui::Text ("scroll_val : %d", scroll_val);
        ImGui::Text ("ImGui::GetTextLineHeightWithSpacing() : %f", ImGui::GetTextLineHeightWithSpacing());
        ImGui::End();
        /* end window debugger */

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
}
