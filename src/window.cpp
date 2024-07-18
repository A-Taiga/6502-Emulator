#include "window.hpp"
#include "SDL2/SDL_events.h"
#include <cassert>
#include <stdexcept>
#include <SDL2/SDL.h>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

UI::Window_Interface::Window_Interface (const char* title, const int x, const int y, const int w, const int h, const std::uint32_t flags)
: xPos   (x)
, yPos   (y)
, width  (w)
, height (h)
{
    if (SDL_Init(flags) < 0)
        throw std::runtime_error (SDL_GetError());

    window = SDL_CreateWindow (title, x, y, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
        throw std::runtime_error (SDL_GetError());
    windowID = SDL_GetWindowID (window);
}

UI::Window_Interface::~Window_Interface ()
{
    puts ("UI::Window_Interface::~Window_Interface ()");
    SDL_DestroyWindow(window);
}

SDL_Window* UI::Window_Interface::get_window () const
{
    return window;
}

SDL_Renderer* UI::Window_Interface::get_renderer () const
{
    assert (renderer != nullptr);
    return renderer;
}

int UI::Window_Interface::get_xPos () const
{
    return xPos;
}

int UI::Window_Interface::get_yPos () const
{
    return yPos;
}

int UI::Window_Interface::get_width () const
{
    return width;
}

int UI::Window_Interface::get_height () const
{
    return height;
}

std::uint32_t UI::Window_Interface::get_windowID  () const
{
    return windowID;
}

const std::bitset <SDL_NUM_SCANCODES>& UI::Window_Interface::get_keys() const
{
    return keys;
}

void UI::Window_Interface::set_xPos (const int x)
{
    xPos = x;
}

void UI::Window_Interface::set_yPos (const int y)
{
    yPos = y;
}

void UI::Window_Interface::set_width (const int w)
{
    width = w;
}

void UI::Window_Interface::set_height (const int h)
{
    height = h;
}

void UI::Window_Interface::set_keys (const std::size_t index, const bool state)
{
    keys[index] = state;
}

UI::OS_Window::OS_Window  (const char* title, const int w, const int h, const int x, const int y, std::uint32_t flags)
: UI::Window_Interface {title, x, y, w, h, flags}
{
    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (this->renderer == nullptr)
        std::runtime_error (SDL_GetError());
}

UI::OS_Window::~OS_Window ()
{
    puts("UI::OS_Window::~OS_Window ()");
    SDL_DestroyRenderer(this->renderer);
}

void UI::OS_Window::update () const
{
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 0);
	SDL_RenderPresent(this->renderer);
	SDL_RenderClear(this->renderer);
}

bool UI::poll (Window_Interface& window, void (*callback)(SDL_Event*, void*), void* uData)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0)
    {
        (*callback)(&event, uData);

        if (event.type == SDL_QUIT)
            return false;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == window.get_windowID())
            return false;


    }
    return true;
}
