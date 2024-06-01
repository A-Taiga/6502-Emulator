#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <thread>

#define LOW  0
#define HIGH 1

class Clock
{
    private:
        bool tick;
        std::thread tick_thread;
        bool& running;
        void handler();
    public:
        Clock (bool& _running);
        const bool& get_tick();
        void join();
};

#endif
