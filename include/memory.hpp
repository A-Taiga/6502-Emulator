#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "common.hpp"
#include <array>
#include <mutex>

namespace _6502
{
    class RAM
    {
        public:
            using type = typename std::array<byte, RAM_SIZE>;
            std::size_t programSize; // remove this and place it in the emulator
            RAM ();
            void reset();
            // type& data();
            type& get_ram();
            byte& operator[](const word index);
            byte operator[](const word index) const;
        private:
            type ram;
            std::mutex m;
    };
}
#endif