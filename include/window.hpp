#ifndef WINDOW_HPP
#define WINDOW_HPP
#include <cstdint>
#include <SDL2/SDL_events.h>
#include <functional>
#include <type_traits>
struct SDL_Window;
struct SDL_Color;
struct ImVec4;
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
        void            render (int, int, int, int, const SDL_Color& color);
        void            render (int, int, int, int, const ImVec4& color);

        template <class Callable>
        requires std::is_invocable_v<Callable, const SDL_Event&>
        void            poll (bool& running, Callable callback);
        SDL_Window*     get_window ();
        void            swap_window ();
        SDL_GLContext   get_glContext ();
        const char*     get_glslVersion ();
        std::uint32_t   get_windowID ();
};

template <class Callable>
requires std::is_invocable_v<Callable, const SDL_Event&>
inline void OS_Window::poll (bool& running, Callable callback)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        std::invoke(callback, event);

        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            SDL_GetWindowSize(window, &width, &height);

        if (event.type == SDL_QUIT)
        {
            running = false;
            return;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID (window))
        {
            running = false;
            return;
        }
    }
}
#endif