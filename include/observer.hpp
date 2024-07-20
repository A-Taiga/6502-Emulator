#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include <unordered_map>

namespace UI
{
    namespace MSG 
    {
        using KeyType = const char*;
        class Subject;
        class Observer
        {
            public:
                virtual ~Observer   ();
                virtual void Update (Subject*) = 0;
            protected:
                KeyType _id;
                Observer ();
        };

        class Subject
        {
            public:
                virtual ~Subject    ();
                virtual void Attach (KeyType const id, Observer* observer);
                virtual void Detach (KeyType const id);
                virtual void Notify ();
            protected:
                Subject ();
            private:
                std::unordered_map<KeyType, Observer*> _observers;
        };
    }
}




#endif