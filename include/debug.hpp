#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "window.hpp"
#include "cpu.hpp"

class Bus;
namespace debug
{
    void init(Window& window);
    void test_demo(Window& window, _6502::Bus data);
    void render (Window& window);
}

#endif