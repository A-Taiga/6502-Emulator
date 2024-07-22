#ifndef WINDOW_HPP
#define WINDOW_HPP


#include "SDL2/SDL_events.h"
#include "SDL2/SDL_scancode.h"
#include <chrono>
#include <cstdint>

struct SDL_Renderer;
struct SDL_Window;

namespace UI
{
    struct Key_state
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> time;
        bool state;
    };

    class Window_interface
    {
        protected:
        int             x_pos;
        int             y_pos;
        int             width;
        int             height;
        SDL_Window*     window;
        SDL_Renderer*   renderer;
        std::uint32_t   window_ID;

        std::array <Key_state, SDL_NUM_SCANCODES> keys;
        

        public:
        Window_interface (const char* title, const int x, const int y, const int w, const int h, const std::uint32_t flags);
        ~Window_interface ();
        SDL_Window*   get_window    () const;
        SDL_Renderer* get_renderer  () const;
        int           get_xPos      () const;
        int           get_yPos      () const;
        int           get_width     () const;
        int           get_height    () const;

        const std::array <Key_state, SDL_NUM_SCANCODES>& get_keys() const;

        std::uint32_t get_window_ID  () const;
        void          set_x_pos      (const int x);
        void          set_y_pos      (const int y);
        void          set_width     (const int w);
        void          set_height    (const int h);
        void          set_keys      (const std::size_t index, const bool state);
        virtual void update () const = 0;

    };

    class OS_window : public Window_interface
    {
        public:
        OS_window  (const char* title
                    , const int w
                    , const int h
                    , const int x
                    , const int y
                    , const std::uint32_t flag);
        ~OS_window ();
        void update () const;

    };
    char to_ASCII (const SDL_Scancode);
    bool poll (Window_interface& window, void (*)(SDL_Event*,void*), void* uData);
}

#endif