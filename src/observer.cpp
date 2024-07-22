#include "observer.hpp"
#include <cassert>

UI::MSG::Observer::~Observer () {}

UI::MSG::Observer::Observer () {}

UI::MSG::Subject::Subject ()
: observers {}
{}

UI::MSG::Subject::~Subject () {}

void UI::MSG::Subject::Attach (UI::MSG::Key_type const id, Observer* observer)
{
    assert (observer != nullptr);
    observers[id] = observer;
}

void UI::MSG::Subject::Detach (UI::MSG::Key_type const id)
{
    observers.erase(id);
}

void UI::MSG::Subject::Notify ()
{
    for (auto& i : observers)
    {
        i.second->Update(this);
    }
}
