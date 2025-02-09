#ifndef MEM_H
#define MEM_H

#include <cstdint>
#include <string>
#include <vector>


class Memory
{
public:
    using mem_type = std::vector <std::uint8_t>;

    Memory (const std::uint16_t size);
    ~Memory();

    bool load (const std::string& path, const std::size_t size);

    std::uint8_t read (const std::uint16_t address) const;
    void write (const std::uint16_t address, const std::uint8_t data);
    void reset ();

    std::uint8_t* data ();

    mem_type::iterator begin();
    mem_type::iterator end  ();

    std::size_t size ();

    bool is_loaded () const;

private:
    mem_type mem;
    bool loaded;
};



#endif