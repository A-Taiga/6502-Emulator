#include "memory.hpp"
#include "macros.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <format>

template<std::size_t N>
std::size_t read_file (const char* path, std::array<byte, N>& buffer, word offeset = 0)
{
    FILE* file = fopen(path, "rb");
    std::size_t size;

    if (file == nullptr)                                        throw std::runtime_error(std::strerror(errno));
    if (std::fseek (file, 0L, SEEK_END) == -1)                  throw std::runtime_error(std::strerror(errno));
    if ((size = std::ftell(file)) == -1UL)                      throw std::runtime_error(std::strerror(errno));
    if (size > N)                                               throw std::runtime_error(std::format("file > {:d}", N));
    if ((std::fseek(file, 0L, SEEK_SET)) == -1L)                throw std::runtime_error(std::strerror(errno));
    if ((std::fread(&buffer[offeset], size, 1, file)) == -1UL)  throw std::runtime_error(std::strerror(errno));
    fclose (file);
    return size;
}

RAM::RAM(const char* fileName, word& aBus, byte& dbus, ACCESS_MODE& accessMode)
: addressBus(aBus)
, dataBus(dbus)
, rw(accessMode)
, mem{0}
{
    read_file(fileName, mem, 0xF000);
}

byte& RAM::operator[](word index)
{
    return mem[index];
}

const byte& RAM::operator[](word index) const
{
    return mem[index];
}

void RAM::reset()
{
    mem = {0};
}

std::array<byte, 65536>& RAM::data()
{
    return mem;
}