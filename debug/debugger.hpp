#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "imgui.h"
#include "window.hpp"
#include <span>
#include <type_traits>
#include <format>

namespace _6502 {class Bus;}

template <class T>
concept is_container = requires {{typename std::decay_t<decltype(*std::declval <T>().begin())>()} -> std::integral;};

template <class T>
requires is_container <T>
struct nested_type {using type = typename std::decay_t <decltype (*std::declval<T>().begin())>;};


namespace UI
{
    // class Memory_Window
    // {
    //     protected:

    //     struct Sizes
    //     {
    //         float glyphWidth;
    //         float glyphHeight;
    //         float addressTextWidth;
    //         float byteTextWidth;
    //         float dataColWidth;
    //         float asciiColWidth;
    //         float addressEnd;
    //         float minWindowWidth;
    //         float minWindowHeight;
    //         float scrollBarWidth;
    //         float windowSize;
    //         float colSpacing;
    //         int   rowWidth;
    //         int   addressPadding;
    //         Sizes ();
    //     } sizes;

    //     void calc_sizes();
    //     std::span <std::uint8_t> view;
    //     std::size_t offset;
        
    //     public:
    //     Memory_Window ( void * const buffer
    //                     , const std::size_t totalMemSize
    //                     , const std::size_t begin
    //                     , const std::size_t end
    //                     , const std::size_t typeSize);
    //     void calc ();
    //     void draw_column_labels ();
    // };

    // class Hex_Editor : public Memory_Window
    // {
    //     private:
    //     const char* name;
    //     bool lookup;
    //     bool isShowing;
    //     std::uint8_t selectedValue;
    //     std::size_t selectedIndex;
    //     static const ImGuiInputTextFlags inpuTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways;
    //     static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
    //     std::vector <char> lookupBuffer;

    //     struct Colors
    //     {
    //         static constexpr ImVec4 scrollbar_backGroundColor {0.2f, 0.2f, 0.2f, 1.0f};
    //         static constexpr ImVec4 scrollbar_grabber         {0.4f, 0.4f, 0.4f, 1.0f};
    //         static constexpr ImVec4 scrollbar_grabberHover    {0.6f, 0.6f, 0.6f, 1.0f};
    //         static constexpr ImVec4 scrollbar_grabberActive   {0.8f, 0.0f, 0.0f, 1.0f};
    //         static constexpr ImVec4 white                     {1,1,1,1};

    //     } colors;

    //     struct User_Data
    //     {
    //         bool set = false;
    //         bool selected;
    //         char buffer[3];
    //     };


    //     public:

    //     Hex_Editor ( const char* windowName
    //                 , void * const buffer
    //                 , const std::size_t totalMemSize
    //                 , const std::size_t begin
    //                 , const std::size_t end
    //                 , const std::size_t typeSize);

    //     void draw ();
    //     static int input_callback(ImGuiInputTextCallbackData* data);
    // };

    class Window_Interface;
    class Debugger
    {
        Window_Interface& window;
        ImFont* font;
        float textSize;
        
        public:
        Debugger (Window_Interface& win);
        ~Debugger ();
        void run (void (*callback)(void*), void* uData);
    };

    template <std::size_t TotalMemorySize>
    std::size_t consteval calc_address_size ()
    {
        std::size_t result = 0;
        for (std::size_t i = TotalMemorySize - 1; i > 0; i >>= 4)
            result++;
        return result;
    };

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

}
#endif
