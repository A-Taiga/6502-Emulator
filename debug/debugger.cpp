#include "debugger.hpp"
#include "SDL_render.h"
#include "imgui_impl_sdl2.h"
#include "window.hpp"
#include <cstdio>
#include <imgui.h>
#include "imgui_impl_sdlrenderer2.h"
#include <cstddef>
#include "IconsFontAwesome6.h"
#include <sys/types.h>
#include "imgui.h"
#include <format>
#include <vector>
#include <cstring>

static constexpr ImVec4 clear_color {0.45f, 0.55f, 0.60f, 1.00f};

UI::Memory_window::Sizes::Sizes (){std::memset(this, 0, sizeof (*this));}

UI::Memory_window::Memory_window (void * const buffer, const std::size_t totalMemSize, const std::size_t begin, const std::size_t end, const std::size_t typeSize)
: sizes {}
{
    offset = begin;
    std::uint8_t (*data)[] = static_cast <std::uint8_t (*)[]> (buffer);
    view = {*data + (typeSize * begin), end};

    for (std::size_t i =  totalMemSize-1;  i > 0; i >>= 4)
        sizes.addressPadding++;

}

void UI::Memory_window::calc ()
{
    sizes.glyphWidth       = ImGui::CalcTextSize("F").x + 1; // monospace 
    sizes.glyphHeight      = ImGui::GetTextLineHeight();
    sizes.addressTextWidth = (sizes.glyphWidth * sizes.addressPadding) + sizes.glyphWidth;
    sizes.byteTextWidth    = sizes.glyphWidth * 2;
    sizes.dataColWidth     = (((sizes.byteTextWidth) + sizes.glyphWidth) * sizes.rowWidth) + (sizes.glyphWidth * (std::ceil(sizes.rowWidth / 8.f) - 1));
    sizes.asciiColWidth    = (sizes.glyphWidth * sizes.rowWidth);
    sizes.minWindowWidth   =  sizes.windowSize = sizes.addressTextWidth +  sizes.dataColWidth  + sizes.asciiColWidth + (sizes.scrollBarWidth*3);
}

void UI::Memory_window::draw_column_labels ()
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

UI::Hex_editor::Hex_editor (const char * windowName, void * const buffer, const std::size_t totalMemSize, const std::size_t begin, const std::size_t end, const std::size_t typeSize)
: Memory_window {buffer, totalMemSize, begin, end, typeSize}
, name {windowName}
{

    lookupBuffer = std::vector<char> (sizes.addressPadding);
    
    this->sizes.rowWidth = 16;
    this->sizes.scrollBarWidth = 20.f;
    selected_value = 0;
}

void UI::Hex_editor::draw ()
{
    [[maybe_unused]] static float scrollY = 0.0f;
    this->calc();
    ImGui::SetNextWindowSize({this->sizes.minWindowWidth, 0});
    ImGui::Begin(name, &is_showing, window_flags);
    this->draw_column_labels();
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, colors.scrollbar_backGroundColor);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, colors.scrollbar_grabber);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, colors.scrollbar_grabberHover);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, colors.scrollbar_grabberActive);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, this->sizes.scrollBarWidth);
    ImGui::BeginChild ("list",{0, ImGui::GetTextLineHeightWithSpacing() * 16});
    ImGuiListClipper clipper;

    clipper.Begin(this->view.size() / this->sizes.rowWidth, ImGui::GetTextLineHeightWithSpacing());
    if (lookup)
    {
        int val = std::strtol (lookupBuffer.data(), nullptr, 16);
        ImGui::SetScrollY((std::floor(val / this->sizes.rowWidth)) * ImGui::GetTextLineHeightWithSpacing());
        lookup = false;
    }

    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            ImGui::Text ("%.*zX:", this->sizes.addressPadding, (row * this->sizes.rowWidth) + offset);
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

                User_data uData;
                snprintf(uData.buffer, sizeof(uData.buffer), "%02X", value);
                ImGui::PushStyleColor(ImGuiCol_Text, this->view[index] == 0x00 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : colors.white);
                if(ImGui::InputText("##input", uData.buffer, sizeof(uData.buffer), input_text_flags, Hex_editor::input_callback, &uData))
                {
                    auto value = std::strtol(uData.buffer, NULL, 16);
                    this->view[row * this->sizes.rowWidth + col] = value;
                }
                if(uData.set)
                {
                    selected_value = this->view[index];
                    selected_index = index;
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
    if (ImGui::InputText("##address lookup", lookupBuffer.data(), this->sizes.addressPadding + 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
    {
        lookup = true;
    }
    ImGui::NewLine();
    ImGui::TextUnformatted(std::format ("Hex     {:{}}{:02X}", " ", 5, selected_value).c_str());
    ImGui::TextUnformatted(std::format ("Binary  {:{}}{:08b}", " ", 5, selected_value).c_str());
    ImGui::TextUnformatted(std::format ("Octal   {:{}}{:o}", " ", 5, selected_value).c_str());
    ImGui::TextUnformatted(std::format ("uint8   {:{}}{:}", " ", 5, selected_value).c_str());
    ImGui::TextUnformatted(std::format ("int8    {:{}}{:}", " ", 5, (std::int8_t)selected_value).c_str());
    if (selected_index + 1 >= view.size())
    {
        ImGui::TextUnformatted(std::format ("uint16  {:{}}EOF", " ", 5).c_str());
        ImGui::TextUnformatted(std::format ("int16   {:{}}EOF", " ", 5).c_str());
    }
    else
    {
        /* the 6502 is little-endian so display the values that way */
        std::uint16_t u16v = (this->view[selected_index+1] << 8) | (selected_value );
        std::int16_t  i16v = (this->view[selected_index+1] << 8) | (selected_value );
        ImGui::TextUnformatted(std::format ("uint16  {:{}}{:}", " ", 5, u16v).c_str());
        ImGui::TextUnformatted(std::format ("int16   {:{}}{:}", " ", 5, i16v).c_str());
    }
    ImGui::TextUnformatted(std::format ("ACII    {:{}}{:}", " ", 5, (char)selected_value).c_str());
    ImGui::End();
}

int UI::Hex_editor::input_callback(ImGuiInputTextCallbackData* data)
{
    User_data* uData = (User_data*)data->UserData;
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

UI::Debugger::Debugger (UI::Window_interface& win)
: window {win}
{
    assert (win.get_window() != nullptr);
    assert (win.get_renderer() != nullptr);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window.get_window(), window.get_renderer());
    ImGui_ImplSDLRenderer2_Init(window.get_renderer());

    text_size = 15.0f;
    font = io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/ProggyClean.ttf", text_size, nullptr, io.Fonts->GetGlyphRangesDefault());
    float iconFontSize = text_size * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
    // // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config; 
    icons_config.MergeMode = true; 
    icons_config.PixelSnapH = true; 
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges );
};

UI::Debugger::~Debugger ()
{
    puts("UI::end()");
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void UI::Debugger::run (void (*callback)(void* uData), void* uData)
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    (*callback)(uData);
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::Begin("info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
    ImGui::Render();
    SDL_RenderSetScale(window.get_renderer(), io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(window.get_renderer(), (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(window.get_renderer());
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), window.get_renderer());
    SDL_RenderPresent(window.get_renderer());
}