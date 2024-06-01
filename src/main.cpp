#include "cpu.hpp"
#include <ncurses.h>
#include <thread>


WINDOW* make_window(int w, int h, int x, int y, const char* label = nullptr)
{
    WINDOW* local = newwin(h, w, y, x);
    box (local, y, x);
    if (label)
        mvwprintw(local, y, 1, label);
    return local;

}


void debug_memory([[maybe_unused]] _6502& emu)
{
    WINDOW* win = make_window(58, 21, 0, 0, "Zero Page");
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(1, COLOR_CYAN, COLOR_BLACK);

    while (true)
    {
        int row = 2;
        int col = 8;
        wmove(win, row, col);
        for (std::size_t i = 0; i < 16; i++)
        {
	        wattroff(win, COLOR_PAIR(1));
            wprintw(win, "%02X", i);
            col+=3;
            wmove(win, row, col);
        }
        row = 3;
        col = 1;
        wmove(win, row, col);
        for (std::size_t i = 0; i <= ZRO_END; i++)
        {
            if (i % 16 == 0)
            {
                row++;
                col = 1;
                wmove(win, row, col);
                wprintw(win, "%04X", i);
                col+=7;

            }
            wmove(win, row, col);
            wprintw(win, "%02X", emu.memory[i]);
            col+=3;
        }
        wrefresh(win); 
        refresh();
    }
}

int main()
{
    initscr();
    cbreak();
    curs_set(0);
    refresh();
    _6502 cpu("loop.bin");
    std::thread db(debug_memory, std::ref(cpu));
    // cpu.decompiler();
    cpu.run();
    db.join();
    

    endwin();
    return 0;
}