#ifndef WINDOW_HPP
#define WINDOW_HPP


#include "SDL2/SDL_events.h"
#include "SDL2/SDL_scancode.h"
#include <cstdint>
#include <bitset>

struct SDL_Renderer;
struct SDL_Window;

namespace UI
{

    class Window_Interface
    {
        protected:
        int             xPos;
        int             yPos;
        int             width;
        int             height;
        SDL_Window*     window;
        SDL_Renderer*   renderer;
        std::uint32_t   windowID;

        std::bitset <SDL_NUM_SCANCODES> keys;
        

        public:
        Window_Interface (const char* title, const int x, const int y, const int w, const int h, const std::uint32_t flags);
        ~Window_Interface ();
        SDL_Window*   get_window    () const;
        SDL_Renderer* get_renderer  () const;
        int           get_xPos      () const;
        int           get_yPos      () const;
        int           get_width     () const;
        int           get_height    () const;

        const std::bitset <SDL_NUM_SCANCODES>& get_keys() const;

        std::uint32_t get_windowID  () const;
        void          set_xPos      (const int x);
        void          set_yPos      (const int y);
        void          set_width     (const int w);
        void          set_height    (const int h);

        void          set_keys      (const std::size_t index, const bool state);
        virtual void update () const = 0;

    };

    class OS_Window : public Window_Interface
    {
        public:
        OS_Window  (const char* title
                    , const int w
                    , const int h
                    , const int x
                    , const int y
                    , const std::uint32_t flag);
        ~OS_Window ();
        void update () const;

    };

    bool poll (Window_Interface& window, void (*)(SDL_Event*,void*), void* uData);
}

#endif