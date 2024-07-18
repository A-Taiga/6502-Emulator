#include "window.hpp"
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


// #include <SDL.h>
// #include <SDL_opengl.h>
// #include <stdexcept>
// #include "window.hpp"
// #include "SDL_pixels.h"
// #include "imgui.h"

// #define WINDOW_FLAGS SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI

// OS_Window::OS_Window (const char* title, int w, int h)
// : width (w), height (h)
// {

//     if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) throw std::runtime_error (SDL_GetError());

//     #if defined (__APPLE__)
//         // GL 3.2 Core + GLSL 150
//         glslVersion = "#version 150";
//         SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
//         SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
//         SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//         SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 2);
//     #else
//         glslVersion = "#version 130";
//         SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
//         SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
//         SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//         SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
//     #endif

//     #ifdef SDL_HINT_IME_SHOW_UI
//         SDL_SetHint (SDL_HINT_IME_SHOW_UI, "1");
//     #endif

//     SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
//     SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
//     SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

//     window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, WINDOW_FLAGS);
//     if (window == nullptr) throw std::runtime_error (SDL_GetError());

//     glContext = SDL_GL_CreateContext (window);
//     SDL_GL_MakeCurrent (window, glContext);
//     SDL_GL_SetSwapInterval (1); // enables vsync
//     mWindowID = SDL_GetWindowID( window );
//     printf ("%d\n",mWindowID);
// }

// OS_Window::~OS_Window ()
// {
//     puts ("~OS_Window");
//     SDL_GL_DeleteContext(glContext);
//     SDL_DestroyWindow(window);
//     SDL_Quit();
// }

// void OS_Window::render (int a, int b, int x, int y, const SDL_Color& color)
// {
//     SDL_GL_MakeCurrent (window, glContext);
//     glViewport(a, b, x, y);
//     glClearColor(color.r, color.g, color.b, color.a);
//     glClear(GL_COLOR_BUFFER_BIT);
// }

// void OS_Window::render (int a, int b, int x, int y, const ImVec4& color)
// {
//     SDL_GL_MakeCurrent (window, glContext);
//     glViewport(a, b, x, y);
//     glClearColor(color.x * color.w, color.y * color.w, color.z * color.w, color.w);
//     glClear(GL_COLOR_BUFFER_BIT);
// }

// void OS_Window::swap_window () { SDL_GL_SwapWindow(window);}
// SDL_Window* OS_Window::get_window () { return window;}
// SDL_GLContext OS_Window::get_glContext () { return glContext;}
// const char* OS_Window::get_glslVersion () { return glslVersion;}
// std::uint32_t OS_Window::get_windowID () { return SDL_GetWindowID (window);}
// int OS_Window::get_width() {return width;}
// int OS_Window::get_height() {return height;}