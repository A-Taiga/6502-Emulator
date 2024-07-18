#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "common.hpp"
#include <array>

namespace _6502
{
    class RAM
    {
        public:
            using type = typename std::array<byte, RAM_SIZE>;
            std::size_t programSize;
            RAM ();
            void reset();
            type& data();
            byte& operator[](const word index);
            const byte& operator[](const word index) const;
        private:
            type mem;
    };
}
#endif