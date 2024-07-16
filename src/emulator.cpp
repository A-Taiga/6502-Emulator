#include "emulator.hpp"
#include "SDL2/SDL.h"
#include "SDL_video.h"
#include "common.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include "cpu.hpp"
#include "debugger.hpp"
#include "imgui_impl_sdl2.h"
#include "imgui_internal.h"
#include "window.hpp"
#include <filesystem>
#include <fstream>
#include <span>
#include "imgui.h"
#include "IconsFontAwesome6.h"
#include <thread>





#define WINDOW_W 1920
#define WINDOW_H 1080

namespace
{
    template<std::size_t N>
    std::size_t load_rom (const std::string_view path, const std::array<byte, N>& buffer, const std::size_t offset)
    {
        try
        {
            const std::size_t fileSize = std::filesystem::file_size(path);
            std::fstream file (path, std::ios::hex);
            file.read((char*)buffer.data()+offset, fileSize);
            file.close();
            return fileSize;
        }
        catch (const std::exception& e) 
        {
            std::cerr << e.what() << '\n';
        }
        return 0;
    }

    template <class ADDRESS_TYPE>
    constexpr std::size_t array_size ()
    {
        return sizeof(ADDRESS_TYPE) * 8 / 4;
    }


    template <class T>
    concept is_container = requires {{typename std::decay_t<decltype(*std::declval <T>().begin())>()} -> std::integral;};
    template <class T>
    requires is_container <T>
    struct nested_type {using type = typename std::decay_t <decltype (*std::declval<T>().begin())>;};

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

/* checks if type is a container and if type is an integral type */

template <class T, class Address_Type, std::size_t Size>
requires is_container <T> && std::is_integral_v<Address_Type>
class Memory_Window
{
    protected:
    using type = nested_type <T>::type;
    using span = std::span <type, Size>;
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
    using dataType = nested_type<T>::type;
    static const ImGuiInputTextFlags inpuTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways;
    static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
    char lookupBuffer [array_size<Address_Type>()];
    bool lookup;
    bool isShowing;
    dataType selectedValue;
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
        selectedValue = 0;
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

_6502::Bus _6502::Emulator::bus     = Bus();
bool       _6502::Emulator::running = true;
bool       _6502::Emulator::pause   = true;
bool       _6502::Emulator::step    = false;
_6502::Emulator::Emulator(const char* filePath)
: currentFile {filePath}
{
    load_rom (filePath, bus.ram.data(), ROM_BEGIN);
    bus.ram[RESET_VECTOR]     = ROM_BEGIN & 0x00FF;
    bus.ram[RESET_VECTOR + 1] = (ROM_BEGIN & 0xFF00) >> 8;
}

void _6502::Emulator::run()
{
    bool running = true;
    static UI::OS_Window window ("Debugger", WINDOW_W, WINDOW_H, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_INIT_EVERYTHING);
    UI::Debugger debugger (window);
    bus.cpu.decompiler();
    bus.cpu.reset();


    std::thread t ([&](){
        while (running)
        {
            if (!pause)
            {
                bus.cpu.run();
                // std::this_thread::sleep_for(debugData.delay);
            }
        }
    });
    
    while (running)
    {
        UI::poll([&](const SDL_Event& ev)
        {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT)
                running = false;
            if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE && ev.window.windowID == window.get_windowID())
                running = false;
        });
        debugger.run(&_6502::Emulator::impl_ui, window);
    }
}

void _6502::Emulator::reset ()
{
    bus.ram.reset();
    load_rom (currentFile.c_str(), bus.ram.data(), ROM_BEGIN);
    bus.ram[RESET_VECTOR]     = (byte)0x00;
    bus.ram[RESET_VECTOR + 1] = (byte)0xF0;
    bus.cpu.reset();
}

void _6502::Emulator::impl_ui (const UI::Window_Interface& window)
{
    static Program_Window <word>                                                  programWindow {bus.cpu.decompiledCode, bus.cpu.PC};
    static Hex_Editor     <std::array <byte, RAM_SIZE>, std::uint16_t, RAM_SIZE>  HexEditor ("Editor",bus.ram.data().begin());
    static Hex_Editor     <std::array <byte, RAM_SIZE>, word, 256>                zeroPage ("Zero Page", bus.ram.data().begin());
    static Hex_Editor     <std::array <byte, RAM_SIZE>, word, 256>                Page1 ("Page 1", bus.ram.data().begin()+0x200);
    static Registers_Window                                                       registers (bus.cpu);
    
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
    ImGui::SetNextWindowSize({static_cast<float>(window.get_width()),0});
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    static int v = 100;
    ImGui::SetNextItemWidth(100);
    if (ImGui::DragInt("##milliseconds delay", &v, 1, 1, 1000, "%d", ImGuiSliderFlags_AlwaysClamp))
    {
        // delay = std::chrono::milliseconds(v);
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_ROTATE_LEFT))
    {
        bus.cpu.reset();
        printf("reset\n");
        pause = true;
    }
    ImGui::SameLine();
    if (ImGui::Button((pause ? ICON_FA_PLAY: ICON_FA_PAUSE)))
    {
        if (pause)
        {
            pause = false;
            step = false;
        }
        else
            pause = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_RIGHT))
    {
        step = true;
        if (!pause)
            pause = true;
        bus.cpu.run();
    }
    ImGui::End();
}