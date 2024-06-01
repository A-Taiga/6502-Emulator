#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "macros.hpp"
#include <array>

#define RAM_SIZE 65536 // 65kb



class RAM
{
    public:
        RAM (const char* fileName, word& aBus, byte& dbus, ACCESS_MODE& accessMode);
        void reset();
        std::array<byte, 65536>& data();
        byte& operator[](word index);
        const byte& operator[](word index) const;
    private:
        word& addressBus;
        byte& dataBus;
        ACCESS_MODE& rw;
        std::array<byte, RAM_SIZE> mem;
};


#endif