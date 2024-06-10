#include <cstddef>
#include <ncurses.h>
#include <signal.h>
#include "emulator.hpp"

#define BRIGHT_WHITE 15

void signal_handler(int signum);
// void debug_memory([[maybe_unused]] Emulator& emu);
// WINDOW* make_window(int w, int h, int x, int y, const char* label = nullptr);
static bool running = true;

int main()
{
    signal(SIGINT, signal_handler); // using ctrl-c to quit wihtout turning my terminal into an interdimensional one
    _6502::Emulator emu("loop5.bin", running);
    emu.run();
    return 0;
}

void signal_handler(int signum)
{
    if (signum == 2)
    {
        running = false;
    }
}

// WINDOW* make_window(int w, int h, int x, int y, const char* label)
// {
//     WINDOW* local = newwin(h, w, y, x);
//     wborder(local, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
//     if (label)
//         mvwprintw(local, y, 1, "%s", label);
//     return local;
// }

// void debug_memory([[maybe_unused]] Emulator& emu)
// {
//     WINDOW* zero_window = make_window(56, 21, 0, 0, "Zero Page");
//     WINDOW* stack_win = make_window(56, 21, 57, 0, "Stack");
//     WINDOW* page_1_win = make_window(56, 21, 114, 0, "Page 1");
//     WINDOW* program = make_window(40, 21, 114+56, 0, "Program");
//     scrollok(program, true);
//     wscrl(program, -10);
//     int row;
//     int col;
   
    

//     start_color();
//     init_pair(1, COLOR_BLACK, BRIGHT_WHITE);
//     // int row;
//     while (running)
//     {
//         row = 2;
//         col = 8;
//         wmove(zero_window, row, col);
//         for (std::size_t i = 0; i < 16; i++)
//         {
//             wprintw(zero_window, "%02zX", i);
//             col+=3;
//             wmove(zero_window, row, col);
//         }
//         row = 3;
//         col = 1;
//         wmove(zero_window, row, col);
//         for (std::size_t i = 0; i <= ZRO_END; i++)
//         {
//             if (i % 16 == 0)
//             {
//                 row++;
//                 col = 1;
//                 wmove(zero_window, row, col);
//                 if(row % 2 == 0)
//                     wattron(zero_window,COLOR_PAIR(1));
//                 wprintw(zero_window, "%04zX", i);
//                 wattroff(zero_window,COLOR_PAIR(1));
//                 col+=7;

//             }
//             wmove(zero_window, row, col);
//             if(row % 2 == 0)
//                 wattron(zero_window,COLOR_PAIR(1));
//             wprintw(zero_window, "%02X%c", emu.read_memory()[i], (i+1) % 16 == 0 ? 0 : ' ');
//             wattroff(zero_window,COLOR_PAIR(1));
//             col+=3;
//         }

//         row = 2;
//         col = 8;
//         wmove(stack_win, row, col);
//         for (std::size_t i = 0; i < 16; i++)
//         {
//             wprintw(stack_win, "%02zX", i);
//             col+=3;
//             wmove(stack_win, row, col);
//         }
//         row = 3;
//         col = 1;
//         wmove(stack_win, row, col);
//         for (std::size_t i = STK_BEGIN; i <= STK_END; i++)
//         {
//             if ((i - STK_BEGIN) % 16 == 0)
//             {
//                 row++;
//                 col = 1;
//                 wmove(stack_win, row, col);
//                 if(row % 2 == 0)
//                     wattron(stack_win,COLOR_PAIR(1));
//                 wprintw(stack_win, "%04zX", i);
//                 wattroff(stack_win,COLOR_PAIR(1));
//                 col+=7;
//             }
//             wmove(stack_win, row, col);
//             if(row % 2 == 0)
//                 wattron(stack_win,COLOR_PAIR(1));
//             wprintw(stack_win, "%02X%c", emu.read_memory()[i], (i+1) % 16 == 0 ? 0 : ' ');
//             wattroff(stack_win,COLOR_PAIR(1));
//             col+=3;
//         }


//         row = 2;
//         col = 8;
//         wmove(page_1_win, row, col);
//         for (std::size_t i = 0; i < 16; i++)
//         {
//             wprintw(page_1_win, "%02zX", i);
//             col+=3;
//             wmove(page_1_win, row, col);
//         }
//         row = 3;
//         col = 1;
//         wmove(page_1_win, row, col);
//         for (std::size_t i = STK_END+1; i <= STK_END+256; i++)
//         {
//             if ((i - STK_BEGIN) % 16 == 0)
//             {
//                 row++;
//                 col = 1;
//                 wmove(page_1_win, row, col);
//                 if(row % 2 == 0)
//                     wattron(page_1_win,COLOR_PAIR(1));
//                 wprintw(page_1_win, "%04zX", i);
//                 wattroff(page_1_win, COLOR_PAIR(1));
//                 col+=7;
//             }
//             wmove(page_1_win, row, col);
//             if(row % 2 == 0)
//                 wattron(page_1_win,COLOR_PAIR(1));
//             wprintw(page_1_win, "%02X%c", emu.read_memory()[i], (i+1) % 16 == 0 ? 0 : ' ');
//             wattroff(page_1_win,COLOR_PAIR(1));
//             col+=3;
//         }



//         wmove(program, 2, 2);
//         row = 2;
//         for (word i = ROM_BEGIN; i < ROM_BEGIN+emu.mem.programSize;)
//         {
//             auto& ins = emu.cpu.opcodes[emu.mem.data()[i]];
//             switch (ins.addressMode)
//             {
//                 case MODE::IMP:
//                 wprintw(program, "%04X %02X %9s", (word)i, emu.mem.data()[i], ins.mnemonic);
//                 wmove(program, row++, 2);
//                 i+=1;
//                 break;
//                 case MODE::IMM: 
//                 case MODE::ZPG:
//                 case MODE::ZPX:
//                 case MODE::ZPY:
//                 case MODE::IZX:
//                 case MODE::IZY:
//                 case MODE::REL:
//                 wprintw(program, "%04X %02X %02X %s", (word)i, emu.mem.data()[i], emu.mem.data()[i+1], ins.mnemonic);
//                 wmove(program, row++, 2);
//                 i+=2;
//                 break;
//                 case MODE::ABS:
//                 case MODE::ABX:
//                 case MODE::ABY:
//                 case MODE::IND:
//                 wprintw(program, "%04X %02X %02X %02X %s", (word)i, emu.mem.data()[i], emu.mem.data()[i+1], emu.mem.data()[i+2], ins.mnemonic);
//                 wmove(program, row++, 2);
//                 i+=3;
//                 break;
//             }
//         }

        

//         mvprintw(21, 0, 
//         "AC = %d\nPC = %04X\nX  = %d\nop = %s\n", 
//         emu.cpu.AC,
//         emu.cpu.PC,
//         emu.cpu.X,
//         emu.cpu.ins.mnemonic);
//         refresh();
//         wrefresh(stack_win);
//         wrefresh(zero_window);
//         wrefresh(page_1_win);
//     wrefresh(program);

//     }
// }