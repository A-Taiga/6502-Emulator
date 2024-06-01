#include <ncurses.h>
#include <thread>
#include <signal.h>
#include "emulator.hpp"

#define BRIGHT_WHITE 15

void signal_handler(int signum);
void debug_memory([[maybe_unused]] Emulator& emu);
WINDOW* make_window(int w, int h, int x, int y, const char* label = nullptr);
static bool running = true;

int main()
{
    signal(SIGINT, signal_handler);
    initscr();
    cbreak();
    curs_set(0);
    refresh();

    Emulator emu("loop.bin", running);
    std::thread db(debug_memory, std::ref(emu));
    // cpu.decompiler();
    db.join();
    endwin();
    return 0;
}

void signal_handler(int signum)
{
    if (signum == 2)
    {
        running = false;
    }
}

WINDOW* make_window(int w, int h, int x, int y, const char* label)
{
    WINDOW* local = newwin(h, w, y, x);
    wborder(local, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
    if (label)
        mvwprintw(local, y, 1, "%s", label);
    return local;
}

void debug_memory([[maybe_unused]] Emulator& emu)
{
    WINDOW* zero_window = make_window(56, 21, 0, 0, "Zero Page");
    WINDOW* stack_win = make_window(56, 21, 57, 0, "Stack");
    start_color();
    init_pair(1, COLOR_BLACK, BRIGHT_WHITE);
    int row;
    int col;
    while (running)
    {
        row = 2;
        col = 8;
        wmove(zero_window, row, col);
        for (std::size_t i = 0; i < 16; i++)
        {
            wprintw(zero_window, "%02zX", i);
            col+=3;
            wmove(zero_window, row, col);
        }
        row = 3;
        col = 1;
        wmove(zero_window, row, col);
        for (std::size_t i = 0; i <= ZRO_END; i++)
        {
            if (i % 16 == 0)
            {
                row++;
                col = 1;
                wmove(zero_window, row, col);
                if(row % 2 == 0)
                    wattron(zero_window,COLOR_PAIR(1));
                wprintw(zero_window, "%04zX", i);
                wattroff(zero_window,COLOR_PAIR(1));
                col+=7;

            }
            wmove(zero_window, row, col);
            if(row % 2 == 0)
                wattron(zero_window,COLOR_PAIR(1));
            wprintw(zero_window, "%02X%c", emu.read_memory()[i], (i+1) % 16 == 0 ? 0 : ' ');
            wattroff(zero_window,COLOR_PAIR(1));
            col+=3;
        }
        wrefresh(zero_window);

        row = 2;
        col = 8;
        wmove(stack_win, row, col);
        for (std::size_t i = 0; i < 16; i++)
        {
            wprintw(stack_win, "%02zX", i);
            col+=3;
            wmove(stack_win, row, col);
        }
        row = 3;
        col = 1;
        wmove(stack_win, row, col);
        for (std::size_t i = STK_BEGIN; i <= STK_END; i++)
        {
            if ((i - STK_BEGIN) % 16 == 0)
            {
                row++;
                col = 1;
                wmove(stack_win, row, col);
                if(row % 2 == 0)
                    wattron(stack_win,COLOR_PAIR(1));
                wprintw(stack_win, "%04zX", i);
                wattroff(stack_win,COLOR_PAIR(1));
                col+=7;
            }
            wmove(stack_win, row, col);
            if(row % 2 == 0)
                wattron(stack_win,COLOR_PAIR(1));
            wprintw(stack_win, "%02X%c", emu.read_memory()[i], (i+1) % 16 == 0 ? 0 : ' ');
            wattroff(stack_win,COLOR_PAIR(1));
            col+=3;
        }
        wrefresh(stack_win);
    }
}