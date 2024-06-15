#include "debugger.hpp"
#include "SDL_events.h"
#include "SDL_hints.h"
#include "SDL_video.h"
#include "common.hpp"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cfloat>
#include <cstddef>
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


template <class ADDRESS_TYPE>
constexpr std::size_t array_size ()
{
    return sizeof(ADDRESS_TYPE) * 8 / 4;
}

namespace
{
    ImFont* font;
    float textSize;
    ImVec4 clear_color;
    
    template <class ADDRESS_TYPE, class DATA_TYPE, std::size_t SIZE>
    struct Memory_view
    {
        using memoryBuffer = std::array <DATA_TYPE, SIZE>;
        
        memoryBuffer* buffer;
        char lookupBuffer [array_size<ADDRESS_TYPE>()];
        bool  lookup;
        float scrollY;

        struct
        {
            float glyphWidth;
            float glyphHeight;
            float addressTextWidth;
            float byteTextWidth;
            float dataColWidth;
            float asciiColWidth;
            float minWindowWidth;
            float minWindowHeight;
            float scrollBarWidth = 20.f;
            float windowSize;
            int   addressPadding;
            int   rowWidth = 16;
        } sizes; 

        struct
        {
            ImVec4 scrollbar_backGroundColor {0.2f, 0.2f, 0.2f, 1.0f};
            ImVec4 scrollbar_grabber         {0.4f, 0.4f, 0.4f, 1.0f};
            ImVec4 scrollbar_grabberHover    {0.6f, 0.6f, 0.6f, 1.0f};
            ImVec4 scrollbar_grabberActive   {0.8f, 0.0f, 0.0f, 1.0f};

        } colors;


        Memory_view (memoryBuffer* memoryBuffer)
        : buffer {memoryBuffer}
        {
            assert (buffer != nullptr);
            sizes.addressPadding = sizeof(ADDRESS_TYPE) * 8 / 4;
            scrollY = 0.0f;
            lookupBuffer[0] = '0';
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

        void calc ()
        {
            sizes.glyphWidth       = ImGui::CalcTextSize("F").x;
            sizes.glyphHeight      = ImGui::CalcTextSize("F").y;
            sizes.addressTextWidth = (ImGui::CalcTextSize("0").x * sizes.addressPadding) + ImGui::CalcTextSize(":").x + sizes.glyphWidth;
            sizes.byteTextWidth    = ImGui::CalcTextSize("FF").x;
            sizes.dataColWidth     = (sizes.byteTextWidth + sizes.glyphWidth ) * (sizes.rowWidth);
            sizes.asciiColWidth    =  (sizes.glyphWidth * sizes.rowWidth);
            sizes.minWindowWidth   =  sizes.windowSize = sizes.addressTextWidth +  sizes.dataColWidth  + sizes.asciiColWidth + sizes.scrollBarWidth;
        }
        
        void draw ()
        {
            calc();
            ImGui::SetNextWindowSize({sizes.windowSize, 0});
            // ImGui::SetNextWindowSizeConstraints({sizes.minWindowWidth, ImGui::GetTextLineHeightWithSpacing() * 19.5f}, {FLT_MAX, FLT_MAX});
            ImGui::Begin ("view", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            draw_column_labels();
            draw_data();
            ImGui::Separator();
            ImGui::SetNextWindowSize({100,0});
            ImGui::SetNextItemWidth(150);
            ImGui::DragInt("###column dragger", &sizes.rowWidth, 0.1f, 8, 32, "%d columns");
            if (sizes.rowWidth < 0) sizes.rowWidth = 1;
            if (sizes.rowWidth > 32) sizes.rowWidth = 32;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            if (ImGui::InputText("##address lookup", lookupBuffer, array_size<ADDRESS_TYPE>() + 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
            {
                lookup = true;
            }
            ImGui::End();

        }

        void draw_column_labels ()
        {
            ImGui::BeginGroup();
            ImGui::Dummy({sizes.addressTextWidth, 0});
            ImGui::SameLine(sizes.addressTextWidth);
            for (DATA_TYPE i = 0; i < sizes.rowWidth; i++)
            {
                if (i != 0 && i % 8 == 0)
                {
                    ImGui::SameLine(0);
                    ImGui::Text(" ");
                    ImGui::SameLine(0);
                }
                ImGui::Text("%02X", i);
                ImGui::SameLine();
            }
            ImGui::EndGroup();
            ImGui::Separator();
        }

        void draw_data ()
        {
            ImGuiStyle style;
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, colors.scrollbar_backGroundColor);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, colors.scrollbar_grabber);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, colors.scrollbar_grabberHover);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, colors.scrollbar_grabberActive);
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, sizes.scrollBarWidth);
            ImGui::BeginChild ("list",{0, ImGui::GetTextLineHeightWithSpacing() * 16});
            ImGuiListClipper clipper;
            
            clipper.Begin(SIZE / sizes.rowWidth, ImGui::GetTextLineHeightWithSpacing());
            if (lookup)
            {
                int val = std::strtol (lookupBuffer, nullptr, 16);
                ImGui::SetScrollY(((int)(val / sizes.rowWidth)) * (ImGui::GetTextLineHeightWithSpacing()));
                lookup = false;
            }
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::Text ("%.*X:", sizes.addressPadding, row*sizes.rowWidth);
                    ImGui::SameLine(sizes.addressTextWidth);
                    for (DATA_TYPE i = 0; i < sizes.rowWidth; i++)
                    {
                        DATA_TYPE value = (*buffer)[row * sizes.rowWidth + i];
                        if (i != 0 && i % 8 == 0)
                        {
                            ImGui::SameLine(0);
                            ImGui::Text(" ");
                            ImGui::SameLine(0);
                        }
                        ImGui::TextColored (value == 0 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : ImVec4(255,255,255,255),"%02X", value);
                        ImGui::SameLine();
                    }
                    ImGui::Dummy({sizes.glyphWidth, ImGui::GetTextLineHeightWithSpacing()});
                    for (DATA_TYPE i = 0; i < sizes.rowWidth; i++)
                    {
    
                        char value = (*buffer)[row * sizes.rowWidth + i];
                        ImGui::SameLine(0,0);
                        if (value > 32)
                            ImGui::Text ("%c", value);
                        else
                            ImGui::Text (".");
                    }
                    scrollY = ImGui::GetScrollY();
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor(4); // Pop the colors we pushed
            ImGui::PopStyleVar(2); // Pop the style variables we pushed
        }
    };
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


    textSize = 25.0f;

    font = io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/ProggyClean.ttf", textSize, nullptr, io.Fonts->GetGlyphRangesDefault());


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
    static Memory_view <word, byte, RAM_SIZE> view {&bus.ram.data()};


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


        view.draw();
        
        

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
        SDL_GL_SwapWindow(window.get_window());
}
