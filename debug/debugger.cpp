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

static constexpr ImVec4 clear_color {0.45f, 0.55f, 0.60f, 1.00f};


UI::Memory_Window::Sizes::Sizes (){std::memset(this, 0, sizeof (*this));}

UI::Memory_Window::Memory_Window (void * const buffer, const std::size_t totalMemSize, const std::size_t begin, const std::size_t end, const std::size_t typeSize)
: sizes {}
{
    offset = begin;
    std::uint8_t (*data)[] = static_cast <std::uint8_t (*)[]> (buffer);
    view = {*data + (typeSize * begin), end};

    for (std::size_t i =  totalMemSize-1;  i > 0; i >>= 4)
        sizes.addressPadding++;

}

void UI::Memory_Window::calc ()
{
    sizes.glyphWidth       = ImGui::CalcTextSize("F").x + 1; // monospace 
    sizes.glyphHeight      = ImGui::GetTextLineHeight();
    sizes.addressTextWidth = (sizes.glyphWidth * sizes.addressPadding) + sizes.glyphWidth;
    sizes.byteTextWidth    = sizes.glyphWidth * 2;
    sizes.dataColWidth     = (((sizes.byteTextWidth) + sizes.glyphWidth) * sizes.rowWidth) + (sizes.glyphWidth * (std::ceil(sizes.rowWidth / 8.f) - 1));
    sizes.asciiColWidth    = (sizes.glyphWidth * sizes.rowWidth);
    sizes.minWindowWidth   =  sizes.windowSize = sizes.addressTextWidth +  sizes.dataColWidth  + sizes.asciiColWidth + (sizes.scrollBarWidth*3);
}

void UI::Memory_Window::draw_column_labels ()
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

UI::Hex_Editor::Hex_Editor (const char * windowName, void * const buffer, const std::size_t totalMemSize, const std::size_t begin, const std::size_t end, const std::size_t typeSize)
: Memory_Window {buffer, totalMemSize, begin, end, typeSize}
, name {windowName}
{

    lookupBuffer = std::vector<char> (sizes.addressPadding);
    
    this->sizes.rowWidth = 16;
    this->sizes.scrollBarWidth = 20.f;
    selectedValue = 0;
}

void UI::Hex_Editor::draw ()
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
    if (ImGui::InputText("##address lookup", lookupBuffer.data(), this->sizes.addressPadding + 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
    {
        lookup = true;
    }
    ImGui::NewLine();
    ImGui::TextUnformatted(std::format ("Hex     {:{}}{:02X}", " ", 5, selectedValue).c_str());
    ImGui::TextUnformatted(std::format ("Binary  {:{}}{:08b}", " ", 5, selectedValue).c_str());
    ImGui::TextUnformatted(std::format ("Octal   {:{}}{:o}", " ", 5, selectedValue).c_str());
    ImGui::TextUnformatted(std::format ("uint8   {:{}}{:}", " ", 5, selectedValue).c_str());
    ImGui::TextUnformatted(std::format ("int8    {:{}}{:}", " ", 5, (std::int8_t)selectedValue).c_str());
    if (selectedIndex + 1 >= view.size())
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

int UI::Hex_Editor::input_callback(ImGuiInputTextCallbackData* data)
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


 template <std::size_t ContainerSize, class ContainerType, class AddressType>
    requires is_container <ContainerType> && std::is_integral_v<AddressType>
    class Memory_Window
    {
        protected:
        using span = std::span <typename nested_type<ContainerType>::type, ContainerSize>;
        span view;

        static constexpr int addressPadding {sizeof(AddressType) * 8 / 4};

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

        Memory_Window () = delete;
        inline constexpr Memory_Window (const ContainerType::iterator& begin) 
        :  view {begin, begin + ContainerSize}
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

    template <std::size_t ContainerSize, class ContainerType, class AddressType>
    requires is_container <ContainerType> && std::is_integral_v<AddressType>
    class Hex_Editor : protected Memory_Window <ContainerSize, ContainerType, AddressType>
    {
        const char* name;
        static const ImGuiInputTextFlags inpuTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways;
        static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
        char lookupBuffer [calc_address_size <ContainerSize> ()];
        bool lookup;
        bool isShowing;
        nested_type <ContainerType>::type selectedValue;
        AddressType selectedIndex;

        struct Colors
        {
            static constexpr ImVec4 scrollbar_backGroundColor {0.2f, 0.2f, 0.2f, 1.0f};
            static constexpr ImVec4 scrollbar_grabber         {0.4f, 0.4f, 0.4f, 1.0f};
            static constexpr ImVec4 scrollbar_grabberHover    {0.6f, 0.6f, 0.6f, 1.0f};
            static constexpr ImVec4 scrollbar_grabberActive   {0.8f, 0.0f, 0.0f, 1.0f};
            static constexpr ImVec4 white                     {1,1,1,1};
        } colors;

        struct User_Data
        {
            bool set = false;
            bool selected;
            char buffer[3];
        };


        public:
        Hex_Editor () = delete;
        Hex_Editor (const char* name, const ContainerType::iterator& begin)
        : Memory_Window <ContainerSize, ContainerType, AddressType> {begin}
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

            clipper.Begin(ContainerSize / this->sizes.rowWidth, ImGui::GetTextLineHeightWithSpacing());
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
            if (ImGui::InputText("##address lookup", lookupBuffer, this->addressPadding + 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                lookup = true;
            }
            ImGui::NewLine();
            ImGui::TextUnformatted(std::format ("Hex     {:{}}{:02X}", " ", 5, selectedValue).c_str());
            ImGui::TextUnformatted(std::format ("Binary  {:{}}{:08b}", " ", 5, selectedValue).c_str());
            ImGui::TextUnformatted(std::format ("Octal   {:{}}{:o}", " ", 5, selectedValue).c_str());
            ImGui::TextUnformatted(std::format ("uint8   {:{}}{:}", " ", 5, selectedValue).c_str());
            ImGui::TextUnformatted(std::format ("int8    {:{}}{:}", " ", 5, (std::int8_t)selectedValue).c_str());
            if (selectedIndex + 1 >= ContainerSize)
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


UI::Debugger::Debugger (Window_Interface& win)
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

    textSize = 15.0f;
    font = io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/ProggyClean.ttf", textSize, nullptr, io.Fonts->GetGlyphRangesDefault());
    float iconFontSize = textSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
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