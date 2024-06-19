#include "debugger.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <chrono>
#include <format>
#include <imgui.h>
#include "cpu.hpp"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "imgui_internal.h"
#include <cstddef>
#include <stdexcept>
#include <string>
#include "common.hpp"
#include "bus.hpp"
#include "IconsFontAwesome6.h"
#include <span>
#include <utility>

#define WINDOW_W 1920
#define WINDOW_H 1080

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
    ImFont* font;
    float textSize;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    OS_Window window ("Debugger", WINDOW_W, WINDOW_H);


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
            auto s = ImGui::GetWindowSize();
            ImGui::End();

            ImGui::Begin("debug");
            ImGui::Text("%f %f", s.x, s.y);
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
    };
    
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


        struct
        {
            bool showBoundingRects;
        } db;

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

    template <class CONTAINER_TYPE, class ADDRESS_TYPE, class DATA_TYPE, std::size_t SIZE>
    struct Hex_Editor : Memory_Window<CONTAINER_TYPE, ADDRESS_TYPE, DATA_TYPE, SIZE>
    {

        char lookupBuffer [array_size<ADDRESS_TYPE>()];
        bool  lookup;
        float scrollY;

        struct
        {
            ImVec4 scrollbar_backGroundColor {0.2f, 0.2f, 0.2f, 1.0f};
            ImVec4 scrollbar_grabber         {0.4f, 0.4f, 0.4f, 1.0f};
            ImVec4 scrollbar_grabberHover    {0.6f, 0.6f, 0.6f, 1.0f};
            ImVec4 scrollbar_grabberActive   {0.8f, 0.0f, 0.0f, 1.0f};

        } colors;

        Hex_Editor (CONTAINER_TYPE container, ADDRESS_TYPE offset = 0)
        : Memory_Window <CONTAINER_TYPE, ADDRESS_TYPE, DATA_TYPE, SIZE> ("Hex Editor", container, offset)
        {
            this->sizes.rowWidth = 16;
            this->sizes.scrollBarWidth = 20.f;
            this->settings.isShowing = true;
            this->settings.windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
            lookupBuffer[0] = '0';
            scrollY = 0.0f;

        }

        void draw ()
        {
            this->calc();
            ImGui::SetNextWindowSize({this->sizes.minWindowWidth, 0});
            ImGui::Begin("Hex Editor", nullptr, this->settings.windowFlags);
            this->draw_column_labels();
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, colors.scrollbar_backGroundColor);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, colors.scrollbar_grabber);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, colors.scrollbar_grabberHover);
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, colors.scrollbar_grabberActive);
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, this->sizes.scrollBarWidth);
            ImGui::BeginChild ("list",{0, ImGui::GetTextLineHeightWithSpacing() * 16});
            ImGuiListClipper clipper;
            clipper.Begin(SIZE / this->sizes.rowWidth, ImGui::GetTextLineHeightWithSpacing());
            if (lookup)
            {
                int val = std::strtol (lookupBuffer, nullptr, 16);
                ImGui::SetScrollY((std::floor(val / this->sizes.rowWidth)) * ImGui::GetTextLineHeightWithSpacing());
                lookup = false;
            }
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    if (this->db.showBoundingRects)
                    {
                        ImGui::GetWindowDrawList()->AddRect({pos.x, pos.y}, {pos.x + this->sizes.addressTextWidth, pos.y + this->sizes.lineHeight}, IM_COL32(255,0,0,255));
                        ImGui::GetWindowDrawList()->AddRect({pos.x + this->sizes.addressTextWidth, pos.y}, {pos.x + this->sizes.addressTextWidth + this->sizes.dataColWidth, pos.y + this->sizes.lineHeight}, IM_COL32(0,255,0,255));
                        ImGui::GetWindowDrawList()->AddRect({pos.x + this->sizes.addressTextWidth + this->sizes.dataColWidth + this->sizes.glyphWidth, pos.y}, {pos.x + this->sizes.addressTextWidth + this->sizes.dataColWidth + this->sizes.asciiColWidth + this->sizes.glyphWidth, pos.y + this->sizes.lineHeight}, IM_COL32(0,255,255,255));
                    }
                
                    ImGui::Text ("%.*X:", this->sizes.addressPadding, row * this->sizes.rowWidth);
                    ImGui::SameLine(this->sizes.addressTextWidth);
                    for (ADDRESS_TYPE i = 0; i < this->sizes.rowWidth; i++)
                    {
                        DATA_TYPE value = this->data[row * this->sizes.rowWidth + i];
                        if (i != 0 && i % 8 == 0)
                        {
                            ImGui::SameLine(0, 0);
                            ImGui::Dummy({this->sizes.glyphWidth, ImGui::GetTextLineHeight()});
                        }
                        ImGui::SameLine(0, this->sizes.glyphWidth);
                        ImGui::TextColored (value == 0 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : ImVec4(255,255,255,255),"%02X", value);
                    }
                    ImGui::SameLine(0, this->sizes.glyphWidth*2);
                    for (ADDRESS_TYPE i = 0; i < this->sizes.rowWidth; i++)
                    {
    
                        char value = this->data[row * this->sizes.rowWidth + i];
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
            ImGui::PopStyleColor(4); // Pop the colors we pushed
            ImGui::PopStyleVar(2); // Pop the style variables we pushed
            ImGui::Separator();
            ImGui::SetNextWindowSize({100,0});
            ImGui::SetNextItemWidth(150);
            ImGui::DragInt("###column dragger", &(this->sizes).rowWidth, 0.1f, 8, 32, "%d columns");
            if (this->sizes.rowWidth < 0) this->sizes.rowWidth = 1;
            if (this->sizes.rowWidth > 32) this->sizes.rowWidth = 32;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            if (ImGui::InputText("##address lookup", lookupBuffer, array_size<ADDRESS_TYPE>() + 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
            {
                lookup = true;
            }
            ImGui::End();
        }
    };


    struct Registers_Window
    {
        const word &PC;
        const byte &AC;
        const byte &X;
        const byte &Y;
        const byte &SP;
        const byte &SR;
        
        Registers_Window (const _6502::CPU& cpu)
        : PC (cpu.PC)
        , AC (cpu.AC)
        , X (cpu.X)
        , Y (cpu.Y)
        , SP (cpu.SP)
        , SR (cpu.SR)
        {}

        void draw ()
        {

            ImGui::Begin ("Registers");
            if (ImGui::BeginTable("Regesters Table", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders))
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

            // [[maybe_unused]] const word C = 1 << 0;
            // [[maybe_unused]] const word Z = 1 << 1;
            // [[maybe_unused]] const word I = 1 << 2;
            // [[maybe_unused]] const word D = 1 << 3;
            // [[maybe_unused]] const word B = 1 << 4;
            // [[maybe_unused]] const word V = 1 << 6;
            // [[maybe_unused]] const word N = 1 << 7;

            // [NV-BDIZC]	(8 bit)
            // if (SR & 0b00000001)
            // {

            // }
            // else if (SR & 0b00000010)
            // {

            // }
            // else if (SR & 0b00000100)
            std::bitset<8> flags(SR);
            // ImVec4 set = {255,0,0,255};
            // ImVec4 notSet = {255,255,255,255};
            ImGui::TextUnformatted("NV-BDIZC");
            ImGui::TextUnformatted("CZIDB-VN");

            for (int i = 0; i < 8; i++)
            {   
                ImGui::Text("%d", static_cast<int>((SR >> i) & 0x1));
                ImGui::SameLine(0,0);
            }

            // ImGui::TextColored(flags.test(0) ? set : notSet, "C"); ImGui::SameLine(0,0);
            // ImGui::TextColored(flags.test(1) ? set : notSet, "Z"); ImGui::SameLine(0,0);
            // ImGui::TextColored(flags.test(2) ? set : notSet, "I"); ImGui::SameLine(0,0);
            // ImGui::TextColored(flags.test(3) ? set : notSet, "D"); ImGui::SameLine(0,0);
            // ImGui::TextColored(flags.test(4) ? set : notSet, "B"); ImGui::SameLine(0,0);
            // ImGui::TextColored(flags.test(5) ? set : notSet, "-"); ImGui::SameLine(0,0);
            // ImGui::TextColored(flags.test(6) ? set : notSet, "V"); ImGui::SameLine(0,0);
            // ImGui::TextColored(flags.test(7) ? set : notSet, "N"); ImGui::SameLine(0,0);


            
            ImGui::End();
        }
    };

    struct Flags_Window
    {

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
    textSize = 15.0f;
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
    static Program_Window<word> programWindow {bus.cpu.decompiledCode, bus.cpu.PC};
    static Page_View<decltype(bus.ram.data()), word, byte, 256> zeroPage("Zero Page", bus.ram.data());
    static Page_View<decltype(bus.ram.data()), word, byte, 256> Page1("Page 1", bus.ram.data(), 0x0200);
    static Hex_Editor<decltype(bus.ram.data()), word, byte, RAM_SIZE> HexEditor (bus.ram.data());
    static Registers_Window registers (bus.cpu);

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
    programWindow.draw();
    HexEditor.draw();

    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);
    ImGui::SetNextWindowSize({static_cast<float>(window.width),0});
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
    static int v = 100;
    if (ImGui::DragInt("##milliseconds delay", &v, 1, 1, 1000, "%d", ImGuiSliderFlags_AlwaysClamp))
    {
        delay = std::chrono::milliseconds(v);
    }
    ImGui::SameLine();
    if (ImGui::Button((pause ? ICON_FA_PLAY: ICON_FA_PAUSE)))
    {
        if (pause)
            pause = false;
        else
            pause = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FORWARD))
    {

    }
    ImGui::End();
    registers.draw();



    

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
