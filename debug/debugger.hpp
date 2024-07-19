#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "imgui.h"
#include "window.hpp"
#include <span>

namespace _6502 {class Bus;}

namespace UI
{

    class Memory_Window
    {
        protected:

        struct Sizes
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
            int   addressPadding;
            Sizes ();
        } sizes;

        void calc_sizes();
        std::span <std::uint8_t> view;
        std::size_t offset;
        
        public:
        Memory_Window ( void * const buffer
                        , const std::size_t totalMemSize
                        , const std::size_t begin
                        , const std::size_t end
                        , const std::size_t typeSize);
        void calc ();
        void draw_column_labels ();
    };

    class Hex_Editor : public Memory_Window
    {
        private:
        const char* name;
        bool lookup;
        bool isShowing;
        std::uint8_t selectedValue;
        std::size_t selectedIndex;
        static const ImGuiInputTextFlags inpuTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways;
        static const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;
        std::vector <char> lookupBuffer;

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
        Hex_Editor ( const char* windowName
                    , void * const buffer
                    , const std::size_t totalMemSize
                    , const std::size_t begin
                    , const std::size_t end
                    , const std::size_t typeSize);
        void draw ();
        static int input_callback(ImGuiInputTextCallbackData* data);
    };

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
}
#endif
