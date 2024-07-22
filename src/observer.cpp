#include "observer.hpp"
#include <cassert>
#include <cstddef>
#include <iostream>

UI::MSG::Observer::~Observer () {}

UI::MSG::Observer::Observer () {}

UI::MSG::Subject::Subject ()
: observers {}
{}

UI::MSG::Subject::~Subject () {}

void UI::MSG::Subject::attach (UI::MSG::Key_type const id, Observer* observer)
{
    assert (observer != nullptr);
    observers[id] = observer;
}

void UI::MSG::Subject::detach (UI::MSG::Key_type const id)
{
    observers.erase(id);
}

void UI::MSG::Subject::notify ()
{
    for (auto& i : observers)
    {
        if (i.second != nullptr)
            i.second->Update(this);
        else
            std::cerr << "observer with ID : " << i.first << " was nullptr" << std::endl;
    }
}
