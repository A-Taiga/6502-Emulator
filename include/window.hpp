#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <functional>
#include <cstdint>

struct ImVec4;
struct SDL_Window;
typedef void *SDL_GLContext;
typedef union SDL_Event SDL_Event;

class OS_Window 
{
    public:
    SDL_Window*     window;
    SDL_GLContext   glContext;
    int             width;
    int             height;
    const char*     glslVersion;
    public:
        OS_Window       (const char* title, int w = 0, int h = 0);
        ~OS_Window      ();
        void            render (int, int, int, int, const ImVec4& color);
        void            poll (std::function<void(SDL_Event&)>);
        SDL_Window*     get_window ();
        void            swap_window ();
        SDL_GLContext   get_glContext ();
        const char*     get_glslVersion ();
        std::uint32_t   get_windowID ();
};


#endif 