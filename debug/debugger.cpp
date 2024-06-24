#include "debugger.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <chrono>
#include <cstdio>
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
#include <sys/types.h>
#include <type_traits>
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

    struct Registers_Window
    {
        const word &PC;
        const byte &AC;
        const byte &X;
        const byte &Y;
        const byte &SP;
        const byte &SR;
        
        struct 
        {
            ImVec4 set = {0,255,0,255};
            ImVec4 notSet = {255,255,255,0};
        } colors;
        
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
    };
}

template <class T>
concept is_container = requires {{typename std::remove_reference <decltype (*std::declval <T>().begin())>::type()} -> std::integral;};

template <class T, class Address_Type, std::size_t Size>
requires is_container <T> && std::is_integral_v<Address_Type>
class Memory_Window
{
    protected:
    using dataType = typename std::remove_reference <decltype (*std::declval <T>().begin())>::type;
    using span = std::span <dataType, Size>;
    span view;
    static const int addressPadding {sizeof(Address_Type) * 8 / 4};
    struct
    {
        float glyphWidth;
        float glyphHeight;
        float addressTextWidth;
        float byteTextWidth;
        float dataColWidth;
        float asciiColWidth;
        float addressEnd;
        float minWindowWidth;
        float minWindowHeight;
        float scrollBarWidth;
        float windowSize;
        float colSpacing;
        int   rowWidth;
    } sizes;

    Memory_Window (const T::iterator& offset)
    : view {offset, offset + Size}
    {}

    void calc ()
    {
        sizes.glyphWidth       = ImGui::CalcTextSize("F").x + 1; // monospace 
        sizes.glyphHeight      = ImGui::GetTextLineHeight();
        sizes.addressTextWidth = (sizes.glyphWidth * addressPadding) + sizes.glyphWidth;
        sizes.byteTextWidth    = sizes.glyphWidth * 2;
        sizes.dataColWidth     = (((sizes.byteTextWidth) + sizes.glyphWidth) * sizes.rowWidth) + (sizes.glyphWidth * (std::ceil(sizes.rowWidth / 8.f) - 1));
        sizes.asciiColWidth    = (sizes.glyphWidth * sizes.rowWidth);
        sizes.minWindowWidth   =  sizes.windowSize = sizes.addressTextWidth +  sizes.dataColWidth  + sizes.asciiColWidth + (sizes.scrollBarWidth*3);
    }
    
    void draw_column_labels ()
    {
        ImGui::BeginGroup();
        ImVec2 pos = ImGui::GetCursorPos();
        ImGui::Dummy({sizes.addressTextWidth, sizes.glyphHeight});
        for (int i = 0; i < 16; i++)
        {
            float byte_pos_x = ((sizes.addressTextWidth + sizes.glyphWidth)) + (sizes.byteTextWidth +  sizes.glyphWidth) * i;
            if (i >= 8)
                byte_pos_x += sizes.glyphWidth;
            ImGui::SameLine(byte_pos_x);
            ImGui::Text("%02X", i);
        }
        ImGui::SameLine(((sizes.addressTextWidth + sizes.glyphWidth) - pos.x) + (sizes.byteTextWidth +  sizes.glyphWidth) * 16 + sizes.byteTextWidth);
        ImGui::Text("ASCII");
        ImGui::EndGroup();
        ImGui::Separator();
    }
};

/* 
    I used this as a guide on developing mine https://github.com/ocornut/imgui_club
*/
template <class T, class Address_Type, std::size_t Size>
requires is_container <T> && std::is_integral_v<Address_Type>
class Hex_Editor : protected Memory_Window<T, Address_Type, Size>
{
    const char* name;
    using dataType = Memory_Window<T, Address_Type, Size>::dataType;
    static const ImGuiInputTextFlags inpuTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways;
    static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
    char lookupBuffer [array_size<Address_Type>()];
    bool lookup;
    bool isShowing;
    dataType selectedValue = 0;
    Address_Type selectedIndex;
    
    struct
    {
        const ImVec4 scrollbar_backGroundColor {0.2f, 0.2f, 0.2f, 1.0f};
        const ImVec4 scrollbar_grabber         {0.4f, 0.4f, 0.4f, 1.0f};
        const ImVec4 scrollbar_grabberHover    {0.6f, 0.6f, 0.6f, 1.0f};
        const ImVec4 scrollbar_grabberActive   {0.8f, 0.0f, 0.0f, 1.0f};
        const ImVec4 white                     {1,1,1,1};

    } colors;

    struct User_Data
    {
        bool set = false;
        bool selected;
        char buffer[3];
    };


    public:
    Hex_Editor (const char* name, const T::iterator& offset)
    : Memory_Window <T, Address_Type, Size> (offset)
    , name (name)
    {
        this->sizes.rowWidth = 16;
        this->sizes.scrollBarWidth = 20.f;
    }

    void draw ()
    {
        [[maybe_unused]] static float scrollY = 0.0f;
        this->calc();
        ImGui::SetNextWindowSize({this->sizes.minWindowWidth, 0});
        ImGui::Begin(name, &isShowing, windowFlags);
        this->draw_column_labels();
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, colors.scrollbar_backGroundColor);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, colors.scrollbar_grabber);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, colors.scrollbar_grabberHover);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, colors.scrollbar_grabberActive);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, this->sizes.scrollBarWidth);
        ImGui::BeginChild ("list",{0, ImGui::GetTextLineHeightWithSpacing() * 16});
        ImGuiListClipper clipper;

        clipper.Begin(Size / this->sizes.rowWidth, ImGui::GetTextLineHeightWithSpacing());
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
                ImGui::Text ("%.*X:", this->addressPadding, row * this->sizes.rowWidth);
                for (std::size_t col = 0; col < 16; col++)
                {
                    std::size_t index = row * this->sizes.rowWidth + col;
                    float byte_pos_x = (this->sizes.addressTextWidth + this->sizes.glyphWidth) + (this->sizes.byteTextWidth +  this->sizes.glyphWidth) * col;
                    if (col >= 8)
                        byte_pos_x += this->sizes.glyphWidth;

                    ImGui::PushID(index);
                    auto value = this->view[index];
                    ImGui::SameLine(byte_pos_x);
                    ImGui::SetNextItemWidth(this->sizes.byteTextWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0,0});
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));

                    User_Data uData;
                    snprintf(uData.buffer, sizeof(uData.buffer), "%02X", value);
                    ImGui::PushStyleColor(ImGuiCol_Text, this->view[index] == 0x00 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : colors.white);
                    if(ImGui::InputText("##input", uData.buffer, sizeof(uData.buffer), inpuTextFlags, Hex_Editor::input_callback, &uData))
                    {
                        auto value = std::strtol(uData.buffer, NULL, 16);
                        this->view[row * this->sizes.rowWidth + col] = value;
                    }
                    if(uData.set)
                    {
                        selectedValue = this->view[index];
                        selectedIndex = index;
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                }
                ImGui::SameLine(0, this->sizes.glyphWidth*2);
                for (std::size_t i = 0; i < (std::size_t)this->sizes.rowWidth; i++)
                {
                    char value = this->view[row * this->sizes.rowWidth + i];
                    if (value >= 32)
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
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2); 
        ImGui::SetNextItemWidth(150);
        if (ImGui::InputText("##address lookup", lookupBuffer, array_size<Address_Type>() + 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            lookup = true;
        }
        ImGui::NewLine();
        ImGui::TextUnformatted(std::format ("Hex     {:{}}{:02X}", " ", 5, selectedValue).c_str());
        ImGui::TextUnformatted(std::format ("Binary  {:{}}{:08b}", " ", 5, selectedValue).c_str());
        ImGui::TextUnformatted(std::format ("Octal   {:{}}{:o}", " ", 5, selectedValue).c_str());
        ImGui::TextUnformatted(std::format ("uint8   {:{}}{:}", " ", 5, selectedValue).c_str());
        ImGui::TextUnformatted(std::format ("int8    {:{}}{:}", " ", 5, (std::int8_t)selectedValue).c_str());
        if (selectedIndex + 1 >= Size)
        {
            ImGui::TextUnformatted(std::format ("uint16  {:{}}EOF", " ", 5).c_str());
            ImGui::TextUnformatted(std::format ("int16   {:{}}EOF", " ", 5).c_str());
        }
        else
        {
            /* the 6502 is little-endian so display the values that way */
            std::uint16_t u16v = (this->view[selectedIndex+1] << 8) | (selectedValue );
            std::int16_t  i16v = (this->view[selectedIndex+1] << 8) | (selectedValue );
            ImGui::TextUnformatted(std::format ("uint16  {:{}}{:}", " ", 5, u16v).c_str());
            ImGui::TextUnformatted(std::format ("int16   {:{}}{:}", " ", 5, i16v).c_str());
        }
        ImGui::TextUnformatted(std::format ("ACII    {:{}}{:}", " ", 5, (char)selectedValue).c_str());
        ImGui::End();
    }

    static int input_callback(ImGuiInputTextCallbackData* data)
    {
        User_Data* uData = (User_Data*)data->UserData;
        uData->set = ImGui::IsItemActive();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, 255));
        if (data->SelectionStart == 0 && data->SelectionEnd == data->BufTextLen)
        {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, uData->buffer);
            data->SelectionStart = 0;
            data->SelectionEnd = 2;
            data->CursorPos = 0;
        }
        return 0;
    }
};

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

void UI::debug(debug_v& values)
{
    window.poll(values.running);
    static ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    static Program_Window <word>                                                  programWindow {values.bus.cpu.decompiledCode, values.bus.cpu.PC};
    static Hex_Editor     <std::array <byte, RAM_SIZE>, std::uint16_t, RAM_SIZE>  HexEditor ("Editor",values.bus.ram.data().begin());
    static Hex_Editor     <std::array <byte, RAM_SIZE>, word, 256>                zeroPage ("Zero Page", values.bus.ram.data().begin());
    static Hex_Editor     <std::array <byte, RAM_SIZE>, word, 256>                Page1 ("Page 1", values.bus.ram.data().begin()+0x200);
    static Registers_Window                                                       registers (values.bus.cpu);
    
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
    registers.draw();

    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    ImGui::SetNextWindowClass(&window_class);
    ImGui::SetNextWindowSize({static_cast<float>(window.width),0});
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    static int v = 100;
    ImGui::SetNextItemWidth(100);
    if (ImGui::DragInt("##milliseconds delay", &v, 1, 1, 1000, "%d", ImGuiSliderFlags_AlwaysClamp))
    {
        values.delay = std::chrono::milliseconds(v);
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_ROTATE_LEFT))
    {
        values.resetCallback();
        values.pause = true;
    }
    ImGui::SameLine();
    if (ImGui::Button((values.pause ? ICON_FA_PLAY: ICON_FA_PAUSE)))
    {
        if (values.pause)
        {
            values.pause = false;
            values.step = false;
        }
        else
            values.pause = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_RIGHT))
    {
        values.step = true;
        if (!values.pause)
            values.pause = true;
        values.bus.cpu.run();
    }
    ImGui::End();

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
