#ifndef WINDOW_H
#define WINDOW_H

#include <functional>
#include <string>

struct SDL_Window;
union SDL_Event;

class Window
{
public:

    Window(const char* _title, int _width, int _height);
    ~Window();

    void poll(std::function<void(SDL_Event&)> callback);
    bool is_running() const;
    void update() const;
    
    SDL_Window* get_window () const;
    void* get_gl_context () const;
    std::string get_glsl_version () const;

private:

    SDL_Window* window;

    void* gl_context;

    std::string title;
    std::string glsl_version;

    int width;
    int height;

    bool running;
};


#endif