#include "window.hpp"
#include <SDL2/SDL.h>
#include "SDL2/SDL_events.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_video.h"
#include "SDL_video.h"
#include "imgui_impl_sdl2.h"
#include <stdexcept>


Window::Window (int w, int h, const char* title)
: width (w), height(h)
{
	if (SDL_INIT_EVERYTHING < 0)
		throw std::runtime_error(SDL_GetError());

	window = SDL_CreateWindow(title
							, SDL_WINDOWPOS_CENTERED
							, SDL_WINDOWPOS_CENTERED
							, width
							, height
							, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	if (window == nullptr)
		throw std::runtime_error(SDL_GetError());

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr)
		throw std::runtime_error(SDL_GetError());
}


void Window::push_event_cb (std::function<void (void *userdata, SDL_Event* event)>&& callback)
{
    callbacks.emplace_back(std::forward<decltype(callback)>(callback));
}

void Window::set_window_size ()
{
	SDL_GetWindowSize(window, &width, &height);
}

SDL_Window* Window::get_window() const
{
    return window;
}

SDL_Renderer* Window::get_renderer() const
{
    return renderer;
}

const int& Window::get_w() const
{
    return width;
}

const int& Window::get_h() const
{
    return height;
}

void Window::poll()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0)
    {
        for (const auto& cb : callbacks)
        {
            cb((void*)this, &event);
        }
    }
}
