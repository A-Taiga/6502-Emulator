#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "window.hpp"


struct ImFont;
namespace _6502 {class Bus;}

namespace UI
{

    class Window_Interface;
    class Debugger
    {
        Window_Interface& window;
        ImFont* font;
        float textSize;
        
        public:
        Debugger (Window_Interface& win);
        ~Debugger ();
        void run (void (*callback)(const Window_Interface&), const Window_Interface& window);
    };
}

#endif 