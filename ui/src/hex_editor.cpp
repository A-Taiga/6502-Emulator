#include "hex_editor.h"
#include "imgui.h"
#include <cmath>
#include <format>


Hex_Editor::Hex_Editor (const char* window_name, const std::size_t total_mem_size, const std::size_t begin, const std::size_t end, const std::size_t type_size,  void * const buffer)
: sizes {}
, name {window_name}
, offset {begin}
{
    // cast the buffer into a usable type for the span
    std::uint8_t (*data)[] = static_cast <std::uint8_t (*)[]> (buffer);
    view = {*data + (type_size * begin), end};

    // calculate how much padding is needed bassed on the address size
    for (auto i = total_mem_size - 1; i > 0; i >>= 4)
        ++sizes.address_padding;

    lookup_buffer = std::vector <char> (sizes.address_padding);

    this->sizes.row_width = 16;
    this->sizes.scroll_bar_width = 20.f;
    selected_value = 0;
}

// this took a looooooong time of messing around with to figure out
void Hex_Editor::calc (void)
{
    sizes.glyph_width = ImGui::CalcTextSize("F").x + 1;
    sizes.glyph_height = ImGui::GetTextLineHeight();
    sizes.address_text_width = (sizes.glyph_width * sizes.address_padding) + sizes.glyph_width;
    sizes.byte_text_width = sizes.glyph_width * 2;
    sizes.data_col_width = (((sizes.byte_text_width) + sizes.glyph_width) * sizes.row_width) + (sizes.glyph_width * (std::ceil(sizes.row_width / 8.f) - 1));
    sizes.ascii_col_width = sizes.glyph_width * sizes.row_width;
    sizes.min_window_width   =  sizes.window_size = sizes.address_text_width +  sizes.data_col_width  + sizes.ascii_col_width + (sizes.scroll_bar_width*3);
}

void Hex_Editor::draw_column_labels ()
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
    ImGui::SameLine(((sizes.address_text_width + sizes.glyph_width) - pos.x) + (sizes.byte_text_width +  sizes.glyph_width) * 16.2 + sizes.byte_text_width);
    ImGui::Text("ASCII");
    ImGui::EndGroup();
    ImGui::Separator();
}

void Hex_Editor::present (void)
{
    static constexpr ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways;
    static constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;

    static constexpr ImVec4 scrollbar_backGroundColor {0.2f, 0.2f, 0.2f, 1.0f};
    static constexpr ImVec4 scrollbar_grabber         {0.4f, 0.4f, 0.4f, 1.0f};
    static constexpr ImVec4 scrollbar_grabberHover    {0.6f, 0.6f, 0.6f, 1.0f};
    static constexpr ImVec4 scrollbar_grabberActive   {0.8f, 0.0f, 0.0f, 1.0f};
    static constexpr ImVec4 white                     {1,1,1,1};

    [[maybe_unused]] static float scrollY = 0.0f;
    
    calc();

    // ImGui::SetNextWindowSize({this->sizes.min_window_width, 0});
    ImGui::Begin(name.c_str(), nullptr, window_flags);
    draw_column_labels();
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, scrollbar_backGroundColor);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, scrollbar_grabber);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, scrollbar_grabberHover);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, scrollbar_grabberActive);
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

                User_Data user_data{};
                snprintf(user_data.buffer, sizeof(user_data.buffer), "%02X", value);
                ImGui::PushStyleColor(ImGuiCol_Text, this->view[index] == 0x00 ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : white);
                if(ImGui::InputText("##input", user_data.buffer, sizeof(user_data.buffer), input_text_flags, Hex_Editor::input_callback, &user_data))
                {
                    auto value = std::strtol(user_data.buffer, NULL, 16);
                    this->view[row * this->sizes.row_width + col] = value;
                }
                if(user_data.set)
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
        lookup = true;

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
    
    ImGui::TextUnformatted(std::format ("ASCII    {:{}}{:}", " ", 5, (char)selected_value).c_str());
    ImGui::End();
}

int Hex_Editor::input_callback(ImGuiInputTextCallbackData* data)
{
    User_Data* user_data = (User_Data*)data->UserData;
    user_data->set = ImGui::IsItemActive();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, 255));

    if (data->SelectionStart == 0 && data->SelectionEnd == data->BufTextLen)
    {
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, user_data->buffer);
        data->SelectionStart = 0;
        data->SelectionEnd = 2;
        data->CursorPos = 0;
    }
    return 0;
}