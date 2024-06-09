#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "window.hpp"
#include "cpu.hpp"
#include "memory.hpp"

namespace debug
{
    struct Data
    {
        _6502::CPU& cpu;
        _6502::RAM& memory;
        Data(_6502::CPU& processor, _6502::RAM& ram);
    };

    void init(Window& window);
    void test_demo(Window& window, debug::Data& data);
    void render (Window& window);
}

#endif