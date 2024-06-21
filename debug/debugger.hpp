#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <chrono>
#include <cstdint>

struct ImVec4;
struct SDL_Window;
typedef void *SDL_GLContext;
typedef union SDL_Event SDL_Event;

struct OS_Window 
{
    SDL_Window*     window;
    SDL_GLContext   glContext;
    int             width;
    int             height;
    const char*     glslVersion;
    public:
        OS_Window       (const char* title, int w = 0, int h = 0);
        ~OS_Window      ();
        void            render (int, int, int, int, const ImVec4& color);
        void            poll (bool& running);
        SDL_Window*     get_window ();
        void            swap_window ();
        SDL_GLContext   get_glContext ();
        const char*     get_glslVersion ();
        std::uint32_t   get_windowID ();
};

namespace _6502 {class Bus;}
namespace UI
{
    struct debug_v
    {
        _6502::Bus& bus;
        std::chrono::milliseconds delay;
        bool running;
        bool pause;
        bool step;
    };
    void init ();
    void debug (debug_v& values);
    void end ();
}





#endif 