#ifndef WINDOW_HPP
#define WINDOW_HPP
#include "SDL2/SDL_render.h"
#include <functional>
#include <SDL_events.h>

class Window
{
    private:
        SDL_Window* window;
        SDL_Renderer* renderer;
        int width, height;
        std::vector<std::function<void (void *userdata, SDL_Event* event)>> callbacks;
    public:
        Window(int w, int h, const char* title);
        void push_event_cb (std::function<void (void *userdata, SDL_Event* event)>&& callback);
        void set_window_size ();
        SDL_Window* get_window() const;
        SDL_Renderer* get_renderer() const;
        const int& get_w() const;
        const int& get_h() const;
        void poll();
        
};




#endif