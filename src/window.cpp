#include "window.hpp"
#include "SDL2/SDL_events.h"
#include "SDL2/SDL_scancode.h"
#include <cassert>
#include <stdexcept>
#include <SDL2/SDL.h>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

UI::Window_interface::Window_interface (const char* title, const int x, const int y, const int w, const int h, const std::uint32_t flags)
: x_pos  (x)
, y_pos  (y)
, width  (w)
, height (h)
, keys {false}
, left_shift (false)
, right_shift (false)
{

    if (SDL_Init(flags) < 0)
        throw std::runtime_error (SDL_GetError());

    window = SDL_CreateWindow (title, x, y, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
        throw std::runtime_error (SDL_GetError());
    window_ID = SDL_GetWindowID (window);
}

UI::Window_interface::~Window_interface ()
{
    puts ("UI::Window_interface::~Window_interface ()");
    SDL_DestroyWindow(window);
}

SDL_Window* UI::Window_interface::get_window () const
{
    return window;
}

SDL_Renderer* UI::Window_interface::get_renderer () const
{
    assert (renderer != nullptr);
    return renderer;
}

int UI::Window_interface::get_xPos () const
{
    return x_pos;
}

int UI::Window_interface::get_yPos () const
{
    return y_pos;
}

int UI::Window_interface::get_width () const
{
    return width;
}

int UI::Window_interface::get_height () const
{
    return height;
}

std::uint32_t UI::Window_interface::get_window_ID  () const
{
    return window_ID;
}

const std::array <bool, SDL_NUM_SCANCODES>& UI::Window_interface::get_keys() const
{
    return keys;
}

void UI::Window_interface::set_x_pos (const int x)
{
    x_pos = x;
}

void UI::Window_interface::set_y_pos (const int y)
{
    y_pos = y;
}

void UI::Window_interface::set_width (const int w)
{
    width = w;
}

void UI::Window_interface::set_height (const int h)
{
    height = h;
}

void UI::Window_interface::set_keys (const std::size_t index, const bool state)
{
    keys[index] = state;
    switch (index)
    {
        case SDL_SCANCODE_LSHIFT: left_shift = state;
        case SDL_SCANCODE_RSHIFT: right_shift = state;
    }
}

UI::OS_window::OS_window  (const char* title, const int w, const int h, const int x, const int y, std::uint32_t flags)
: UI::Window_interface {title, x, y, w, h, flags}
{
    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (this->renderer == nullptr)
        std::runtime_error (SDL_GetError());
}

UI::OS_window::~OS_window ()
{
    puts("UI::OS_window::~OS_window ()");
    SDL_DestroyRenderer(this->renderer);
}

void UI::OS_window::update () const
{
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 0);
	SDL_RenderPresent(this->renderer);
	SDL_RenderClear(this->renderer);
}


bool UI::poll (Window_interface& window, void (*callback)(SDL_Event*, void*), void* uData)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0)
    {
        (*callback)(&event, uData);

        if (event.type == SDL_QUIT)
            return false;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == window.get_window_ID())
            return false;
    }
    return true;
}

char UI::Window_interface::to_ASCII (const SDL_Scancode code)
{
    switch (code)
    {
        case SDL_SCANCODE_A:     return (left_shift || right_shift) ? 'A' : 'a';
        case SDL_SCANCODE_B:     return (left_shift || right_shift) ? 'B' : 'b';
        case SDL_SCANCODE_C:     return (left_shift || right_shift) ? 'C' : 'c';
        case SDL_SCANCODE_D:     return (left_shift || right_shift) ? 'D' : 'd';
        case SDL_SCANCODE_E:     return (left_shift || right_shift) ? 'E' : 'e';
        case SDL_SCANCODE_F:     return (left_shift || right_shift) ? 'F' : 'f';
        case SDL_SCANCODE_G:     return (left_shift || right_shift) ? 'G' : 'g';
        case SDL_SCANCODE_H:     return (left_shift || right_shift) ? 'H' : 'h';
        case SDL_SCANCODE_I:     return (left_shift || right_shift) ? 'I' : 'i';
        case SDL_SCANCODE_J:     return (left_shift || right_shift) ? 'J' : 'j';
        case SDL_SCANCODE_K:     return (left_shift || right_shift) ? 'K' : 'k';
        case SDL_SCANCODE_L:     return (left_shift || right_shift) ? 'L' : 'l';
        case SDL_SCANCODE_M:     return (left_shift || right_shift) ? 'M' : 'm';
        case SDL_SCANCODE_N:     return (left_shift || right_shift) ? 'N' : 'n';
        case SDL_SCANCODE_O:     return (left_shift || right_shift) ? 'O' : 'o';
        case SDL_SCANCODE_P:     return (left_shift || right_shift) ? 'P' : 'p';
        case SDL_SCANCODE_Q:     return (left_shift || right_shift) ? 'Q' : 'q';
        case SDL_SCANCODE_R:     return (left_shift || right_shift) ? 'R' : 'r';
        case SDL_SCANCODE_S:     return (left_shift || right_shift) ? 'S' : 's';
        case SDL_SCANCODE_T:     return (left_shift || right_shift) ? 'T' : 't';
        case SDL_SCANCODE_U:     return (left_shift || right_shift) ? 'U' : 'u';
        case SDL_SCANCODE_V:     return (left_shift || right_shift) ? 'V' : 'v';
        case SDL_SCANCODE_W:     return (left_shift || right_shift) ? 'W' : 'w';
        case SDL_SCANCODE_X:     return (left_shift || right_shift) ? 'X' : 'x';
        case SDL_SCANCODE_Y:     return (left_shift || right_shift) ? 'Y' : 'y';
        case SDL_SCANCODE_Z:     return (left_shift || right_shift) ? 'Z' : 'z';
        case SDL_SCANCODE_1:     return (left_shift || right_shift) ? '!' : '1';
        case SDL_SCANCODE_2:     return (left_shift || right_shift) ? '@' : '2';
        case SDL_SCANCODE_3:     return (left_shift || right_shift) ? '#' : '3';
        case SDL_SCANCODE_4:     return (left_shift || right_shift) ? '$' : '4';
        case SDL_SCANCODE_5:     return (left_shift || right_shift) ? '%' : '5';
        case SDL_SCANCODE_6:     return (left_shift || right_shift) ? '^' : '6';
        case SDL_SCANCODE_7:     return (left_shift || right_shift) ? '&' : '7';
        case SDL_SCANCODE_8:     return (left_shift || right_shift) ? '*' : '8';
        case SDL_SCANCODE_9:     return (left_shift || right_shift) ? '(' : '9';
        case SDL_SCANCODE_0:     return (left_shift || right_shift) ? ')' : '0';

        case SDL_SCANCODE_SPACE:  return ' ';
        case SDL_SCANCODE_RETURN: return '\r';

        
        case SDL_SCANCODE_LSHIFT:
            left_shift = true;
            return static_cast <char> (14);
        case SDL_SCANCODE_RSHIFT: 
            right_shift = true;
            return static_cast <char> (15);
            
        case SDL_SCANCODE_BACKSPACE: return static_cast <char> (8);
        
        default: return '.';
    }
}
