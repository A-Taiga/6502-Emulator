#ifndef HEX_EDITOR_H
#define HEX_EDITOR_H



#include <span>
#include <cstdint>
#include <string>
#include <vector>


struct ImGuiInputTextCallbackData;

class Hex_Editor
{
public:

    Hex_Editor (const char * window_name,
                const std::size_t total_mem_size,
                const std::size_t begin,
                const std::size_t end,
                const std::size_t type_size,
                void * const buffer);

    void present (void);
    static int input_callback (ImGuiInputTextCallbackData* data);

private:

    struct Sizes
    {
        float glyph_width;
        float glyph_height;
        float address_text_width;
        float byte_text_width;
        float data_col_width;
        float ascii_col_width;
        float min_window_width;
        float min_window_height;
        float scroll_bar_width;
        float window_size;
        float col_spacing;
        int row_width;
        int address_padding;
    };

    struct User_Data
    {
        bool set;
        bool selected;
        char buffer[3];
    };


    Sizes       sizes;
    std::string name;
    std::size_t offset;
    std::span <std::uint8_t> view;

    bool lookup;
    bool is_showing;

    std::uint8_t selected_value;
    std::uint16_t selected_index;

    std::vector <char> lookup_buffer;


    void calc (void);
    void draw_column_labels (void);

};

#endif