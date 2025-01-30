#ifndef MEM_H
#define MEM_H

#include <cstdint>
#include <array>
#include <string>




namespace Memory
{

    static constexpr std::uint16_t ram_size = 0x7FFF;
    static constexpr std::uint16_t rom_size = 0x7FFF - 0xFFFF;

    class RAM
    {
    public:
        using ram_type = std::array <std::uint8_t, ram_size>;

        RAM();
        ~RAM();
        std::uint8_t read (const std::uint16_t address) const;
        void write (std::uint16_t address, const std::uint8_t data);
        void reset ();
        ram_type& get_ram ();
        
    private:
       ram_type ram;  
    };
    
    struct Rom_file
    {
        std::string file_path;
        std::string file_name;
        std::size_t file_size;
    };

    class ROM
    {
    public:
        using rom_type = std::array <std::uint8_t, rom_size>;

        ROM ();
        ~ROM();
        bool load (const std::string& path, const std::size_t size);
        std::uint8_t read (const std::uint16_t address) const;
        void reset ();
        bool is_loaded () const;
        rom_type& get_rom ();
       
    private:
        std::array <std::uint8_t, rom_size> rom;
        bool loaded;
    };
};



#endif