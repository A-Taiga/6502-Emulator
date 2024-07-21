#include "window.hpp"
#include "SDL2/SDL_events.h"
#include <cassert>
#include <chrono>
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
    keys.fill({std::chrono::high_resolution_clock::now(),false});

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

// const std::bitset <SDL_NUM_SCANCODES>& UI::Window_Interface::get_keys() const
// {
//     return keys;
// }

const std::array <UI::Key_State, SDL_NUM_SCANCODES>& UI::Window_Interface::get_keys() const
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

        keys[index].time = std::chrono::high_resolution_clock::now();
        keys[index].state = state;
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

char UI::to_ASCII (const SDL_Scancode code)
{
    switch (code)
    {
        case SDL_SCANCODE_A:     return 'a';
        case SDL_SCANCODE_B:     return 'b';
        case SDL_SCANCODE_C:     return 'c';
        case SDL_SCANCODE_D:     return 'd';
        case SDL_SCANCODE_E:     return 'e';
        case SDL_SCANCODE_F:     return 'f';
        case SDL_SCANCODE_G:     return 'g';
        case SDL_SCANCODE_H:     return 'h';
        case SDL_SCANCODE_I:     return 'i';
        case SDL_SCANCODE_J:     return 'j';
        case SDL_SCANCODE_K:     return 'k';
        case SDL_SCANCODE_L:     return 'l';
        case SDL_SCANCODE_M:     return 'm';
        case SDL_SCANCODE_N:     return 'n';
        case SDL_SCANCODE_O:     return 'o';
        case SDL_SCANCODE_P:     return 'p';
        case SDL_SCANCODE_Q:     return 'q';
        case SDL_SCANCODE_R:     return 'r';
        case SDL_SCANCODE_S:     return 's';
        case SDL_SCANCODE_T:     return 't';
        case SDL_SCANCODE_U:     return 'u';
        case SDL_SCANCODE_V:     return 'v';
        case SDL_SCANCODE_W:     return 'w';
        case SDL_SCANCODE_X:     return 'x';
        case SDL_SCANCODE_Y:     return 'y';
        case SDL_SCANCODE_Z:     return 'z';
        case SDL_SCANCODE_1:     return '1';
        case SDL_SCANCODE_2:     return '2';
        case SDL_SCANCODE_3:     return '3';
        case SDL_SCANCODE_4:     return '4';
        case SDL_SCANCODE_5:     return '5';
        case SDL_SCANCODE_6:     return '6';
        case SDL_SCANCODE_7:     return '7';
        case SDL_SCANCODE_8:     return '8';
        case SDL_SCANCODE_9:     return '9';
        case SDL_SCANCODE_0:     return '0';
        case SDL_SCANCODE_SPACE: return ' ';
        default: return '0';
    }
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
