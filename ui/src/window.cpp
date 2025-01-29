#include "window.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>
#include <iostream>
#include <source_location>

namespace
{
    void print_error (const std::source_location& source = std::source_location::current())
    {
        std::clog << "file: "
                << source.file_name() << '('
                << source.line() << ':'
                << source.column() << ") `"
                << source.function_name() << "`: "
                << SDL_GetError() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

Window::Window(const char* _title, int _width, int _height)
: title {_title}
, width {_width}
, height {_height}
{

   // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif



    const auto result = SDL_Init (SDL_INIT_EVERYTHING);
    if (result < 0) print_error ();

    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);

    const SDL_WindowFlags window_flags = static_cast <SDL_WindowFlags> (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    
    window = SDL_CreateWindow (_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, window_flags);
    if (!window) print_error ();

    gl_context = SDL_GL_CreateContext (window);
    if (!gl_context) print_error ();
    
    SDL_GL_MakeCurrent (window, gl_context);
    SDL_GL_SetSwapInterval (1); // Enable vsync

    running = true;

}

Window::~Window() 
{
    SDL_GL_DeleteContext (gl_context);
    SDL_DestroyWindow (window);
    SDL_Quit ();
}

void Window::poll(std::function<void(SDL_Event&)> callback) 
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) 
    {
        callback(event);

        if (event.type == SDL_QUIT)
            running = false;

        else if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                width = event.window.data1;
                height = event.window.data2;
            }
        }
    }
}

bool Window::is_running() const
{
    return running;
}

void Window::update() const
{

}

SDL_Window* Window::get_window () const
{
    return window;
}

void* Window::get_gl_context () const
{
    return gl_context;
}

std::string Window::get_glsl_version () const
{
    return glsl_version;
}