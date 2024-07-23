#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "common.hpp"
#include <array>

namespace MOS_6502
{
    class RAM
    {
        public:
            using type = typename std::array<byte, RAM_SIZE>;
            std::size_t program_size; // remove this and place it in the emulator
            RAM ();
            void reset();
            type& get_ram();

            byte& operator[](const word index);
            const byte&  operator[](const word index) const;

        private:
            type ram;
    };
}
#endif