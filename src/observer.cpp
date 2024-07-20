#include "observer.hpp"
#include <cassert>

UI::MSG::Observer::~Observer () {}

UI::MSG::Observer::Observer () {}

UI::MSG::Subject::Subject ()
: _observers {}
{}

UI::MSG::Subject::~Subject () {}

void UI::MSG::Subject::Attach (UI::MSG::KeyType const id, Observer* observer)
{
    assert (observer != nullptr);
    _observers[id] = observer;
}

void UI::MSG::Subject::Detach (UI::MSG::KeyType const id)
{
    _observers.erase(id);
}

void UI::MSG::Subject::Notify ()
{
    for (auto& i : _observers)
    {
        i.second->Update(this);
    }
}
