#include "clock.hpp"
#include <chrono>

Clock::Clock(bool& _running)
: tick(LOW)
, tick_thread(&Clock::handler, this)
, running(_running)
{

}

void Clock::handler()
{
    using namespace std::chrono_literals;
    while (running)
    {
        std::this_thread::sleep_for(500ms);
        tick = HIGH;
        std::this_thread::sleep_for(500ms);
        tick = LOW;
    }
}

const bool& Clock::get_tick()
{
    return tick;
}

void Clock::join()
{
    tick_thread.join();
}

