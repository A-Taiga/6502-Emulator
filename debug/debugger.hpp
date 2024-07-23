#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "imgui.h"
#include <span>
#include <vector>


namespace _6502 {class Bus;}
namespace UI
{
    class Memory_window
    {
        public:
            Memory_window ( void * const buffer
                            , const std::size_t totalMemSize
                            , const std::size_t begin
                            , const std::size_t end
                            , const std::size_t typeSize);
            void draw_column_labels ();
        protected:
            struct Sizes
            {
                float glyph_width;
                float glyph_height;
                float address_text_width;
                float byte_text_width;
                float data_col_width;
                float ascii_col_width;
                float address_end;
                float min_window_width;
                float min_window_height;
                float scroll_bar_width;
                float window_size;
                float col_spacing;
                int   row_width;
                int   address_padding;
                Sizes ();
            } sizes;
            void calc ();
            std::span <std::uint8_t> view;
            std::size_t offset;
    };

    class Hex_editor : public Memory_window
    {
        public:
            Hex_editor ( const char* window_name
                        , void * const buffer
                        , const std::size_t total_mem_size
                        , const std::size_t begin
                        , const std::size_t end
                        , const std::size_t type_size);
            void draw ();
            static int input_callback(ImGuiInputTextCallbackData* data);
        private:
            const char* name;
            bool lookup;
            bool is_showing;
            std::uint8_t selected_value;
            std::size_t selected_index;
            static const ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways;
            static const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
            std::vector <char> lookup_buffer;

            struct Colors
            {
                static constexpr ImVec4 scrollbar_backGroundColor {0.2f, 0.2f, 0.2f, 1.0f};
                static constexpr ImVec4 scrollbar_grabber         {0.4f, 0.4f, 0.4f, 1.0f};
                static constexpr ImVec4 scrollbar_grabberHover    {0.6f, 0.6f, 0.6f, 1.0f};
                static constexpr ImVec4 scrollbar_grabberActive   {0.8f, 0.0f, 0.0f, 1.0f};
                static constexpr ImVec4 white                     {1,1,1,1};

            } colors;

            struct User_data
            {
                bool set = false;
                bool selected;
                char buffer[3];
            };
    };
    class Window_interface;
    class Debugger
    {
        UI::Window_interface& window;
        ImFont* font;
        float text_size;
        
        public:
        Debugger (Window_interface& win);
        ~Debugger ();
        void run (void (*callback)(void*), void* uData);
    };
}
#endif
