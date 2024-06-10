#include "memory.hpp"
#include "common.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <format>

template<std::size_t N>
std::size_t read_file (const char* path, std::array<byte, N>& buffer, [[maybe_unused]] word begin = 0, word end = 0)
{
    FILE* file = fopen(path, "rb");
    std::size_t size;

    if (file == nullptr)                                      throw std::runtime_error(std::strerror(errno));
    if (std::fseek (file, 0L, SEEK_END) == -1)                throw std::runtime_error(std::strerror(errno));
    if ((size = std::ftell(file)) == -1UL)                    throw std::runtime_error(std::strerror(errno));
    if (size > end)                                           throw std::runtime_error(std::format("file > {:d}", N));
    if ((std::fseek(file, 0L, SEEK_SET)) == -1L)              throw std::runtime_error(std::strerror(errno));
    if ((std::fread(buffer.data() + ROM_BEGIN, size, 1, file)) == -1UL)  throw std::runtime_error(std::strerror(errno));
    fclose (file);
    return size;
}

_6502::RAM::RAM()
: mem{0}
{
    // programSize = read_file(fileName, mem, ROM_BEGIN, ROM_END);
}

byte& _6502::RAM::operator[](word index)
{
    return mem[index];
}

const byte& _6502::RAM::operator[](word index) const
{
    return mem[index];
}

void _6502::RAM::reset()
{
    mem = {0};
}

std::array<byte, RAM_SIZE>& _6502::RAM::data()
{
    return mem;
}

