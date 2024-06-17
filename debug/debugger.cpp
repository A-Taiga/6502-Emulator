#include "debugger.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <X11/X.h>
#include <chrono>
#include <format>
#include <imgui.h>
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_internal.h"
#include <cstddef>
#include <stdexcept>
#include <string>
#include "common.hpp"
#include "bus.hpp"
#include "IconsFontAwesome6.h"


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
    puts ("~OS_Window");
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

void OS_Window::poll (bool& running)
{

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            SDL_GetWindowSize(window, &width, &height);
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            running = false;
        
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID (window))
            running = false;
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
    [[maybe_unused]] ImFont* font;
    [[maybe_unused]] float textSize;
    [[maybe_unused]] ImVec4 clear_color;
    OS_Window window ("Debugger", 1000, 1000);
/*
    template <class ADDRESS_TYPE, class DATA_TYPE, std::size_t SIZE>
    struct Memory_Window
    {
        using memoryBuffer = std::array <DATA_TYPE, SIZE>;
        
        memoryBuffer* buffer;
        char lookupBuffer [array_size<ADDRESS_TYPE>()];
        bool  lookup;
        float scrollY;
        ImVec2 windowSize;
        struct
        {
            bool showBoundingRects;
        } db;

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
            float lineHeight;
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


        Memory_Window (memoryBuffer* memoryBuffer)
        : buffer {memoryBuffer}
        {
            assert (buffer != nullptr);
            sizes.addressPadding = sizeof(ADDRESS_TYPE) * 8 / 4;
            scrollY = 0.0f;
            lookupBuffer[0] = '0';
        }

        Memory_Window() {sizes.addressPadding = sizeof(ADDRESS_TYPE) * 8 / 4;}

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

        void draw ()
        {
            calc();
            ImGui::SetNextWindowSize({sizes.windowSize, 0});
            ImGui::Begin ("Memory View", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);
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
            windowSize = ImGui::GetWindowSize();
            ImGui::End();
        }

        void calc ()
        {
            sizes.glyphWidth       = ImGui::CalcTextSize("F").x;
            sizes.glyphHeight      = ImGui::CalcTextSize("F").y;
            sizes.addressTextWidth = (ImGui::CalcTextSize("0").x * sizes.addressPadding) + ImGui::CalcTextSize(":").x + sizes.glyphWidth;
            sizes.byteTextWidth    = ImGui::CalcTextSize("FF").x;
            sizes.dataColWidth     = (((sizes.byteTextWidth) + sizes.glyphWidth) * sizes.rowWidth) + (sizes.glyphWidth * (std::ceil(sizes.rowWidth / 8.f) - 1));
            sizes.asciiColWidth    = (sizes.glyphWidth * sizes.rowWidth);
            sizes.lineHeight       = ImGui::GetTextLineHeightWithSpacing();
            sizes.minWindowWidth   =  sizes.windowSize = sizes.addressTextWidth +  sizes.dataColWidth  + sizes.asciiColWidth + (sizes.scrollBarWidth*3);
        }

        void draw_column_labels ()
        {
            ImGui::BeginGroup();
            ImGui::Dummy({sizes.addressTextWidth - sizes.glyphWidth, ImGui::GetTextLineHeightWithSpacing()});
            for (DATA_TYPE i = 0; i < sizes.rowWidth; i++)
            {
                if (i != 0 && i % 8 == 0)
                {
                    ImGui::SameLine(0, 0);
                    ImGui::Dummy({sizes.glyphWidth, ImGui::GetTextLineHeight()});
                }
                ImGui::SameLine(0,sizes.glyphWidth);
                ImGui::Text("%02X", i);
            }
            ImGui::SameLine(0,sizes.glyphWidth*2);
            ImGui::TextUnformatted("ASCII");
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
                ImGui::SetScrollY((std::floor(val / sizes.rowWidth)) * ImGui::GetTextLineHeightWithSpacing());
                lookup = false;
            }
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    if (db.showBoundingRects)
                    {
                        ImGui::GetWindowDrawList()->AddRect({pos.x, pos.y}, {pos.x + sizes.addressTextWidth, pos.y + sizes.lineHeight}, IM_COL32(255,0,0,255));
                        ImGui::GetWindowDrawList()->AddRect({pos.x + sizes.addressTextWidth, pos.y}, {pos.x + sizes.addressTextWidth + sizes.dataColWidth, pos.y + sizes.lineHeight}, IM_COL32(0,255,0,255));
                        ImGui::GetWindowDrawList()->AddRect({pos.x + sizes.addressTextWidth + sizes.dataColWidth + sizes.glyphWidth, pos.y}, {pos.x + sizes.addressTextWidth + sizes.dataColWidth + sizes.asciiColWidth + sizes.glyphWidth, pos.y + sizes.lineHeight}, IM_COL32(0,255,255,255));
                    }
                
                    ImGui::Text ("%.*X:", sizes.addressPadding, row*sizes.rowWidth);
                    ImGui::SameLine(sizes.addressTextWidth);
                    for (ADDRESS_TYPE i = 0; i < sizes.rowWidth; i++)
                    {
                        DATA_TYPE value = (*buffer)[row * sizes.rowWidth + i];
                        if (i != 0 && i % 8 == 0)
                        {
                            ImGui::SameLine(0, 0);
                            ImGui::Dummy({sizes.glyphWidth, ImGui::GetTextLineHeight()});
                        }
                        ImGui::SameLine(0,sizes.glyphWidth);
                        ImGui::TextColored (value == 0 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : ImVec4(255,255,255,255),"%02X", value);
                    }
                    ImGui::SameLine(0, sizes.glyphWidth*2);
                    for (ADDRESS_TYPE i = 0; i < sizes.rowWidth; i++)
                    {
    
                        char value = (*buffer)[row * sizes.rowWidth + i];
                        if (value > 32)
                            ImGui::Text ("%c", value);
                        else
                            ImGui::Text (".");
                        ImGui::SameLine(0,0);
                    }
                    ImGui::NewLine();
                    scrollY = ImGui::GetScrollY();
                }
            }
            ImGui::EndChild();
           
            //  ImGui::GetWindowDrawList()->AddRect({pos.x, pos.y}, {pos.x + sizes.addressTextWidth, pos.y + sizes.lineHeight}, IM_COL32(255,0,0,255));
            ImGui::PopStyleColor(4); // Pop the colors we pushed
            ImGui::PopStyleVar(2); // Pop the style variables we pushed
        }
    };
*/

    template <class ADDRESS_TYPE>
    struct Program_Window
    {
        using programBuffer = std::vector<std::pair<ADDRESS_TYPE, std::string>>;
        const programBuffer& buffer;
        ADDRESS_TYPE& pc;
        ImVec2 windowSize;
        char findBuffer[array_size<ADDRESS_TYPE>()];
        struct
        {
            int addressPadding;
        } sizes;
        struct
        {
            ADDRESS_TYPE current_row;
        }db;

        Program_Window (const programBuffer& pb, ADDRESS_TYPE& programCounter)
        : buffer {pb}
        , pc (programCounter)
        {
            sizes.addressPadding = sizeof(ADDRESS_TYPE) * 8 / 4;
        }
    
        void draw ()
        {
            ImGui::Begin("Program", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            draw_data();
            ImGui::GetWindowSize();
            ImGui::End();
        };

        void draw_data ()
        {
            ImGui::BeginChild("programClipper", {0, ImGui::GetTextLineHeightWithSpacing() * 18});
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
    };

/*
    template <class ADDRESS_TYPE, class DATA_TYPE, class CONTAINER, std::size_t SIZE>
    struct Page_Window : Memory_Window<ADDRESS_TYPE, DATA_TYPE, SIZE>
    {
        using memoryPage = std::span<DATA_TYPE, SIZE>;
        memoryPage page;
        const char* windowName;
        const ADDRESS_TYPE startingAddress;

        Page_Window (const char* name, ADDRESS_TYPE startAddress, CONTAINER::iterator begin, CONTAINER::iterator end)
        : Memory_Window <ADDRESS_TYPE, DATA_TYPE, SIZE> ()
        , page {begin + startAddress, end + startAddress + SIZE}
        , windowName (name)
        , startingAddress (startAddress)
        {
            this->sizes.rowWidth = 16;
        }
        void draw ()
        {
            this->calc();
            // ImGui::Begin (windowName, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            this->draw_column_labels();
            draw_data();
            // ImGui::End();
        }
        void draw_data ()
        {
            for (ADDRESS_TYPE row = 0; row < SIZE; row += this->sizes.rowWidth)
            {
                ImGui::Text ("%.*X:", this->sizes.addressPadding, startingAddress + row);
                ImGui::SameLine(this->sizes.addressTextWidth);
                for (ADDRESS_TYPE i = 0; i < this->sizes.rowWidth; i++)
                {
                    ADDRESS_TYPE index = row + i;
                    if (i != 0 && i % 8 == 0)
                    {
                        ImGui::SameLine(0, 0);
                        ImGui::Dummy({this->sizes.glyphWidth, ImGui::GetTextLineHeight()});
                    }
                    ImGui::SameLine(0, this->sizes.glyphWidth);
                    ImGui::TextColored (page[index] == 0 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : ImVec4(255,255,255,255),"%02X", page[index]);
                }
                ImGui::SameLine(0, this->sizes.glyphWidth*2);
                for (ADDRESS_TYPE i = 0; i < this->sizes.rowWidth; i++)
                {

                    ADDRESS_TYPE index = row + i;
                    if (page[index] > 32)
                        ImGui::Text ("%c", page[index]);
                    else
                        ImGui::Text (".");
                    ImGui::SameLine(0,0);
                }
                ImGui::NewLine();
                // scrollY = ImGui::GetScrollY();
            }
        }
    };
*/
    
    template <class CONTAINER_TYPE, class ADDRESS_TYPE, class DATA_TYPE, std::size_t SIZE>
    struct Memory_Window
    {
        using Memory_Span = std::span <DATA_TYPE, SIZE>;
        Memory_Span data;
        
        struct
        {
            const char* windowName;
            bool isShowing;
            ADDRESS_TYPE startingAddress;
            ImGuiWindowFlags windowFlags;
        } settings;

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
            float scrollBarWidth;
            float windowSize;
            float lineHeight;
            int   addressPadding;
            int   rowWidth;
        } sizes;

        Memory_Window (const char* name, CONTAINER_TYPE container, ADDRESS_TYPE offset)
        : data {container.begin() + offset, SIZE}
        {
            settings.windowName = name;
            sizes.addressPadding = sizeof(ADDRESS_TYPE) * 8 / 4;
        }

        void calc ()
        {
            sizes.glyphWidth       = ImGui::CalcTextSize("F").x;
            sizes.glyphHeight      = ImGui::CalcTextSize("F").y;
            sizes.addressTextWidth = (ImGui::CalcTextSize("0").x * sizes.addressPadding) + ImGui::CalcTextSize(":").x + sizes.glyphWidth;
            sizes.byteTextWidth    = ImGui::CalcTextSize("FF").x;
            sizes.dataColWidth     = (((sizes.byteTextWidth) + sizes.glyphWidth) * sizes.rowWidth) + (sizes.glyphWidth * (std::ceil(sizes.rowWidth / 8.f) - 1));
            sizes.asciiColWidth    = (sizes.glyphWidth * sizes.rowWidth);
            sizes.lineHeight       = ImGui::GetTextLineHeightWithSpacing();
            sizes.minWindowWidth   =  sizes.windowSize = sizes.addressTextWidth +  sizes.dataColWidth  + sizes.asciiColWidth + (sizes.scrollBarWidth*3);
        }
        
        void draw_column_labels ()
        {
            ImGui::BeginGroup();
            ImGui::Dummy({sizes.addressTextWidth - sizes.glyphWidth, ImGui::GetTextLineHeightWithSpacing()});
            for (DATA_TYPE i = 0; i < sizes.rowWidth; i++)
            {
                if (i != 0 && i % 8 == 0)
                {
                    ImGui::SameLine(0, 0);
                    ImGui::Dummy({sizes.glyphWidth, ImGui::GetTextLineHeight()});
                }
                ImGui::SameLine(0,sizes.glyphWidth);
                ImGui::Text("%02X", i);
            }
            ImGui::SameLine(0,sizes.glyphWidth*2);
            ImGui::TextUnformatted("ASCII");
            ImGui::EndGroup();
            ImGui::Separator();
        }
    };

    template <class CONTAINER_TYPE, class ADDRESS_TYPE, class DATA_TYPE, std::size_t SIZE>
    struct Page_View : Memory_Window <CONTAINER_TYPE, ADDRESS_TYPE, DATA_TYPE, SIZE>
    {
        Page_View (const char* name, CONTAINER_TYPE container, ADDRESS_TYPE offset = 0)
        : Memory_Window <CONTAINER_TYPE, ADDRESS_TYPE, DATA_TYPE, SIZE> {name, container, offset}
        {
            this->sizes.scrollBarWidth = 20.f;
            this->sizes.rowWidth = 16;
            this->settings.isShowing = true;
            this->settings.windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
        }

        void draw ()
        {
            this->calc();
            if (this->settings.isShowing)
            {
                ImGui::Begin (this->settings.windowName, &this->settings.isShowing, this->settings.windowFlags);
                this->draw_column_labels();
                for (ADDRESS_TYPE row = 0; row < SIZE; row += this->sizes.rowWidth)
                {
                    ImGui::Text ("%.*X:", this->sizes.addressPadding, this->settings.startingAddress + row);
                    ImGui::SameLine(this->sizes.addressTextWidth);
                    for (ADDRESS_TYPE i = 0; i < this->sizes.rowWidth; i++)
                    {
                        ADDRESS_TYPE index = row + i;
                        if (i != 0 && i % 8 == 0)
                        {
                            ImGui::SameLine(0, 0);
                            ImGui::Dummy({this->sizes.glyphWidth, ImGui::GetTextLineHeight()});
                        }
                        ImGui::SameLine(0, this->sizes.glyphWidth);
                        ImGui::TextColored (this->data[index] == 0 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : ImVec4(255,255,255,255),"%02X", this->data[index]);
                    }
                    ImGui::SameLine(0, this->sizes.glyphWidth*2);
                    for (ADDRESS_TYPE i = 0; i < this->sizes.rowWidth; i++)
                    {

                        ADDRESS_TYPE index = row + i;
                        if (this->data[index] > 32)
                            ImGui::Text ("%c", this->data[index]);
                        else
                            ImGui::Text (".");
                        ImGui::SameLine(0,0);
                    }
                    ImGui::NewLine();
                }
                ImGui::End();
            }
        }
    };
}


void UI::init ()
{
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
    textSize = 20.0f;
    font = io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/ProggyClean.ttf", textSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    float iconFontSize = textSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config; 
    icons_config.MergeMode = true; 
    icons_config.PixelSnapH = true; 
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges );

}

void UI::end ()
{
    puts("UI::end()");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}


void UI::debug(bool& running, _6502::Bus& bus, [[maybe_unused]] std::chrono::milliseconds& delay, [[maybe_unused]] bool& pause)
{
    window.poll(running);
    static ImGuiIO& io = ImGui::GetIO(); (void)io;
    // static Memory_Window <word, byte, RAM_SIZE> memoryWindow {&bus.ram.data()};
    static Program_Window<word> programWindow {bus.cpu.decompiledCode, bus.cpu.PC};
    // static Page_Window<word, byte, std::array<byte, RAM_SIZE>, 256> zeroPage ("Zero Page", 0x0, bus.ram.data().begin(), bus.ram.data().begin());
    // static Page_Window<word, byte, std::array<byte, RAM_SIZE>, 256> Page1 ("Page 1", 0x200, bus.ram.data().begin(), bus.ram.data().begin());
    static Page_View<decltype(bus.ram.data()), word, byte, 256> zeroPage("Zero Page", bus.ram.data());
    static Page_View<decltype(bus.ram.data()), word, byte, 256> Page1("Page 1", bus.ram.data(), 0x0200);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ParentViewportId, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    zeroPage.draw();
    Page1.draw();


    // ImGui::SetNextWindowPos({0,0});
    // ImGui::SetNextWindowSize({static_cast<float>(window.width),0});
    // ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
    // static int v = 100;
    // if (ImGui::SliderInt("##milliseconds delay", &v, 1, 1000, "%d", ImGuiSliderFlags_AlwaysClamp))
    // {
    //     delay = std::chrono::milliseconds(v);
    // }
    // ImGui::SameLine();
    // if (ImGui::Button(ICON_FA_BACKWARD))
    // {
    // }
    // ImGui::SameLine();
    // if (ImGui::Button((pause ? ICON_FA_PLAY: ICON_FA_PAUSE)))
    // {
    //     if (pause)
    //         pause = false;
    //     else
    //         pause = true;
    // }
    // ImGui::SameLine();
    // if (ImGui::Button(ICON_FA_FORWARD))
    // {
    // }
    // ImVec2 s = ImGui::GetWindowSize();
    // ImGui::End();
    

    // ImGui::SetNextWindowPos({0, s.y});
    // memoryWindow.draw();
    // ImGui::SetNextWindowPos({memoryWindow.sizes.minWindowWidth, s.y});
    // ImGui::SetNextWindowSize({0, memoryWindow.windowSize.y});
    // ImGui::Begin("tabs", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    // if (ImGui::BeginTabBar("Tabs"))
    // {
    //     if(ImGui::BeginTabItem("ZeroPage"))
    //     {
    //         zeroPage.draw();
    //         ImGui::EndTabItem();
    //     }
    //     if (ImGui::BeginTabItem("Page 1"))
    //     {
    //         Page1.draw();
    //         ImGui::EndTabItem();
    //     }
    //     ImGui::EndTabBar();
    // }
    // ImVec2 s2 = ImGui::GetWindowSize();
    // ImGui::End();
    // ImGui::SetNextWindowPos({memoryWindow.sizes.minWindowWidth + s2.x, s.y});
    // ImGui::SetNextWindowSize({300,memoryWindow.windowSize.y});
    // programWindow.draw();




    ImGui::SetNextWindowSize({200,0});
    ImGui::Begin ("Registers", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text ("PC :%04X", bus.cpu.PC);   
    ImGui::Text ("AC :%d", bus.cpu.AC);   
    ImGui::Text ("X  :%d", bus.cpu.X);    
    ImGui::Text ("Y  :%d", bus.cpu.Y);   
    ImGui::Text ("SP :%d", bus.cpu.SP);
    ImGui::BeginGroup();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::TextUnformatted (std::format ("{:08b}", bus.cpu.SR).c_str());
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::TextUnformatted("NV-BDIZC");
    ImGui::Spacing();
    ImGui::EndGroup();
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 255, 255, 255));
    ImGui::End();

        // ImGui::SetNextWindowSize({0,0});
        // ImGui::Begin("memory window debugger", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        // ImGui::Text("GetTextLineHeightWithSpacing : %f", ImGui::GetTextLineHeightWithSpacing());
        // ImGui::Text("GetTextLineHeight            : %f", ImGui::GetTextLineHeight());
        // ImGui::Text("rowWidth                     : %i", memoryWindow.sizes.rowWidth);
        // ImGui::Text("t                            : %f", (std::ceil((memoryWindow.sizes.rowWidth / 8.0f) - 1)));
        // ImGui::Separator();
        // ImGui::Text ("glyphWidth        : %f", memoryWindow.sizes.glyphWidth);
        // ImGui::Text ("glyphHeight       : %f", memoryWindow.sizes.glyphHeight);
        // ImGui::Text ("addressTextWidth  : %f", memoryWindow.sizes.addressTextWidth);
        // ImGui::Text ("byteTextWidth     : %f", memoryWindow.sizes.byteTextWidth);
        // ImGui::Text ("dataColWidth      : %f", memoryWindow.sizes.dataColWidth);
        // ImGui::Text ("asciiColWidth     : %f", memoryWindow.sizes.asciiColWidth);
        // ImGui::Text ("minWindowWidth    : %f", memoryWindow.sizes.minWindowWidth);
        // ImGui::Text ("minWindowHeight   : %f", memoryWindow.sizes.minWindowHeight);
        // ImGui::Text ("scrollBarWidth    : %f", memoryWindow.sizes.scrollBarWidth);
        // ImGui::Text ("windowSize        : %f", memoryWindow.sizes.windowSize);
        // ImGui::Text ("lineHeight        : %f", memoryWindow.sizes.lineHeight);
        // ImGui::Text ("addressPadding    : %d", memoryWindow.sizes.addressPadding);
        // ImGui::Text ("rowWidth          : %d", memoryWindow.sizes.rowWidth);
        // if (ImGui::Button("Show Bounding Rects"))
        // {
        //     if (memoryWindow.db.showBoundingRects) 
        //         memoryWindow.db.showBoundingRects = false;
        //     else
        //         memoryWindow.db.showBoundingRects = true;
        // }
        // ImGui::End();

        ImGui::Begin("info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

     

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
