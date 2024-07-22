#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include <unordered_map>

namespace UI
{
    namespace MSG 
    {
        using Key_type = const char*;
        class Subject;
        class Observer
        {
            public:
                virtual ~Observer   ();
                virtual void Update (Subject*) = 0;
            protected:
                Key_type id;
                Observer ();
        };

        class Subject
        {
            public:
                virtual ~Subject    ();
                virtual void Attach (Key_type const id, Observer* observer);
                virtual void Detach (Key_type const id);
                virtual void Notify ();
            protected:
                Subject ();
            private:
                std::unordered_map<Key_type, Observer*> observers;
        };
    }
}




#endif