#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "common.hpp"
#include <array>
#include <thread>


class RAM
{
    public:
        std::size_t programSize;
        RAM (const char* fileName, Link& l);
        void reset();
        std::array<byte, RAM_SIZE>& data();
        byte& operator[](word index);
        const byte& operator[](word index) const;
    private:
        Link& link;
        std::array<byte, RAM_SIZE> mem;

};


#endif