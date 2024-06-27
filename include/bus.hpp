#ifndef BUS_HPP
#define BUS_HPP

#include "cpu.hpp"
#include "memory.hpp"
#include "common.hpp"

namespace _6502 
{
    class Bus
    {
        public:
            mutable RAM ram;
            CPU cpu;
            Bus ();
            ~Bus ();
            void cpu_write (const word& address, const byte data) const;
            byte cpu_read (const word& address) const;
    };
}

#endif