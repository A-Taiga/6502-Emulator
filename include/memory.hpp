#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "common.hpp"
#include <array>


namespace _6502
{
    class RAM
    {
        public:
            std::size_t programSize;
            RAM (const char* fileName);
            void reset();
            std::array<byte, RAM_SIZE>& data();
            byte& operator[](word index);
            const byte& operator[](word index) const;
        private:
            std::array<byte, RAM_SIZE> mem;
    };
}


#endif