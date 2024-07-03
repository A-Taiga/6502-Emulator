#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <chrono>
#include <functional>


class OS_Window;


namespace _6502 {class Bus;}
namespace UI
{
    struct debug_v
    {
        _6502::Bus& bus;
        std::chrono::nanoseconds delay;
        bool running;
        bool pause;
        bool step;
        std::function<void()> resetCallback;
        OS_Window& window;
    };
    void init (OS_Window& window);
    void debug (debug_v& values);
    void end ();
}







#endif 