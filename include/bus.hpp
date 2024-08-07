#ifndef BUS_HPP
#define BUS_HPP

#include "cpu.hpp"
#include "memory.hpp"
#include "common.hpp"

namespace MOS_6502 
{
    class Bus
    {
        public:
            RAM ram;
            CPU cpu;
            Bus ();
            ~Bus ();
            void cpu_write (const word& address, const byte data);
            byte cpu_read (const word& address) const;
    };
}

#endif