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

UI::Memory_window::Memory_window (void * const buffer, const std::size_t total_mem_size, const std::size_t begin, const std::size_t end, const std::size_t type_size)
: sizes {}
{
    offset = begin;
    std::uint8_t (*data)[] = static_cast <std::uint8_t (*)[]> (buffer);
    view = {*data + (type_size * begin), end};

    for (std::size_t i =  total_mem_size-1;  i > 0; i >>= 4)
        sizes.address_padding++;

}

void UI::Memory_window::calc ()
{
    sizes.glyph_width       = ImGui::CalcTextSize("F").x + 1; // monospace 
    sizes.glyph_height      = ImGui::GetTextLineHeight();
    sizes.address_text_width = (sizes.glyph_width * sizes.address_padding) + sizes.glyph_width;
    sizes.byte_text_width    = sizes.glyph_width * 2;
    sizes.data_col_width     = (((sizes.byte_text_width) + sizes.glyph_width) * sizes.row_width) + (sizes.glyph_width * (std::ceil(sizes.row_width / 8.f) - 1));
    sizes.ascii_col_width    = (sizes.glyph_width * sizes.row_width);
    sizes.min_window_width   =  sizes.window_size = sizes.address_text_width +  sizes.data_col_width  + sizes.ascii_col_width + (sizes.scroll_bar_width*3);
}

void UI::Memory_window::draw_column_labels ()
{
    ImGui::BeginGroup();
    ImVec2 pos = ImGui::GetCursorPos();
    ImGui::Dummy({sizes.address_text_width, sizes.glyph_height});
    for (int i = 0; i < 16; i++)
    {
        float byte_pos_x = ((sizes.address_text_width + sizes.glyph_width)) + (sizes.byte_text_width +  sizes.glyph_width) * i;
        if (i >= 8)
            byte_pos_x += sizes.glyph_width;
        ImGui::SameLine(byte_pos_x);
        ImGui::Text("%02X", i);
    }
    ImGui::SameLine(((sizes.address_text_width + sizes.glyph_width) - pos.x) + (sizes.byte_text_width +  sizes.glyph_width) * 16 + sizes.byte_text_width);
    ImGui::Text("ASCII");
    ImGui::EndGroup();
    ImGui::Separator();
}

UI::Hex_editor::Hex_editor (const char * windowName, void * const buffer, const std::size_t totalMemSize, const std::size_t begin, const std::size_t end, const std::size_t typeSize)
: Memory_window {buffer, totalMemSize, begin, end, typeSize}
, name {windowName}
{

    lookup_buffer = std::vector<char> (sizes.address_padding);
    
    this->sizes.row_width = 16;
    this->sizes.scroll_bar_width = 20.f;
    selected_value = 0;
}

void UI::Hex_editor::draw ()
{
    [[maybe_unused]] static float scrollY = 0.0f;
    this->calc();
    ImGui::SetNextWindowSize({this->sizes.min_window_width, 0});
    ImGui::Begin(name, &is_showing, window_flags);
    this->draw_column_labels();
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, colors.scrollbar_backGroundColor);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, colors.scrollbar_grabber);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, colors.scrollbar_grabberHover);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, colors.scrollbar_grabberActive);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, this->sizes.scroll_bar_width);
    ImGui::BeginChild ("list",{0, ImGui::GetTextLineHeightWithSpacing() * 16});
    ImGuiListClipper clipper;

    clipper.Begin(this->view.size() / this->sizes.row_width, ImGui::GetTextLineHeightWithSpacing());
    if (lookup)
    {
        int val = std::strtol (lookup_buffer.data(), nullptr, 16);
        ImGui::SetScrollY((std::floor(val / this->sizes.row_width)) * ImGui::GetTextLineHeightWithSpacing());
        lookup = false;
    }

    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
        {
            ImGui::Text ("%.*zX:", this->sizes.address_padding, (row * this->sizes.row_width) + offset);
            for (std::size_t col = 0; col < 16; col++)
            {
                std::size_t index = row * this->sizes.row_width + col;
                float byte_pos_x = (this->sizes.address_text_width + this->sizes.glyph_width) + (this->sizes.byte_text_width +  this->sizes.glyph_width) * col;
                if (col >= 8)
                    byte_pos_x += this->sizes.glyph_width;

                ImGui::PushID(index);
                auto value = this->view[index];
                ImGui::SameLine(byte_pos_x);
                ImGui::SetNextItemWidth(this->sizes.byte_text_width);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0,0});
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));

                User_data uData;
                snprintf(uData.buffer, sizeof(uData.buffer), "%02X", value);
                ImGui::PushStyleColor(ImGuiCol_Text, this->view[index] == 0x00 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : colors.white);
                if(ImGui::InputText("##input", uData.buffer, sizeof(uData.buffer), input_text_flags, Hex_editor::input_callback, &uData))
                {
                    auto value = std::strtol(uData.buffer, NULL, 16);
                    this->view[row * this->sizes.row_width + col] = value;
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
            ImGui::SameLine(0, this->sizes.glyph_width*2);
            for (std::size_t i = 0; i < (std::size_t)this->sizes.row_width; i++)
            {
                char value = this->view[row * this->sizes.row_width + i];
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
    if (ImGui::InputText("##address lookup", lookup_buffer.data(), this->sizes.address_padding + 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
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
        ImGui::TextUnformatted(std::format ("uint16  {:{}}{:}", " ", 5, (this->view[selected_index+1] << 8) | (selected_value )).c_str());
        ImGui::TextUnformatted(std::format ("int16   {:{}}{:}", " ", 5, (this->view[selected_index+1] << 8) | (selected_value )).c_str());
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