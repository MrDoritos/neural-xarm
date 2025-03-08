#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <errno.h>
#include <string.h>
#include <functional>
#include <algorithm>
#include <map>
#include <vector>
#include <signal.h>

namespace gui {
  
template<typename Iter, typename Func>
inline Func for_each(Iter &iterable, Func func) {
    return std::for_each(iterable.begin(), iterable.end(), func);
}

template<typename Func, typename Iter, typename ...Args>
inline void for_each(Iter &iterable, Args&&...args) {
    gui::for_each<Iter>(iterable, [args...](Func &func) { func(args...); });
}

namespace events {
    using event_t = uint32_t;

    struct KeyboardEvent {
        int key;
    };

    enum ProgramEvents : event_t {
        INIT,
        LOAD,
        RESET,
        DESTROY,
        HINT_EXIT,
        SAFE_EXIT,
        ERROR,
        SIGNAL,
        UPDATE
    };

    enum KeyboardEvents : event_t {
        PRESS,
        HOLD,
        RELEASE
    };
    
    enum JoystickEvents : event_t {
        CONNECT,
        DISCONNECT,
        BUTTON
    };

    template<template<typename> typename P, typename T> 
    using Invoke = typename P<T>::type;

    template<typename T>
    struct FunctionType { using type = std::function<void()>; };

    template<typename T> 
    struct EnumType { using type = T; };

    template<>
    struct FunctionType<JoystickEvents> { using type = std::function<void(int)>; };
    
    template<>
    struct FunctionType<KeyboardEvents> { using type = std::function<void(KeyboardEvent)>; };

    template<typename T>
    using GetFunction = Invoke<FunctionType, T>;

    template<typename T>
    using GetEnum = Invoke<EnumType, T>;

    template<typename Event, typename Function = GetFunction<Event>>
    using GetMap = std::map<GetEnum<Event>, std::vector<Function>>;

    template<typename Event, typename Function = GetFunction<Event>>
    struct Dispatcher {
        using function = Function;
        using event = Event;

        static GetMap<Event, Function> subscribers;

        template<typename ...Args>
        static void post(const Event &e, Args &&... args) {
            gui::for_each<Function>(subscribers[e], args...);
        }

        static void subscribe(const Event &e, Function f) {
            subscribers[e].push_back(f);
        }
    };

    template<typename Event, typename Function = GetFunction<Event>>
    using GetDispatcher = gui::events::Dispatcher<GetEnum<Event>, Function>;

    template<typename Event, typename Function>
    GetMap<Event, Function> Dispatcher<Event, Function>::subscribers = GetMap<Event, Function>();
}

template<typename Event, typename Function>
void subscribe(Event e, Function func) {
    gui::events::GetDispatcher<Event>
        ::subscribe(e, func);
}

template<typename Event, typename ...Args>
void post(Event e, Args&&...args) {
    gui::events::GetDispatcher<Event>
        ::post(e, args...);
}

}

enum NewEvents : gui::events::event_t {
    NEW_EVENT
};

struct NewEvent {
    std::string message;
};

template<>
struct gui::events::FunctionType<NewEvents> { using type = std::function<void(const NewEvent&)>; };

void error();

int main() {
  using namespace gui;
  using namespace gui::events;
  
  subscribe(ERROR, error);
  
  subscribe(INIT, [](){ 
    puts("Program starting");
  });
  
  subscribe(CONNECT, [](int device) { 
    printf("Device %i connected\n", device); 
  });
  
  subscribe(PRESS, [](KeyboardEvent e) { 
    printf("Key press %i\n", e.key);
  });

  subscribe(NEW_EVENT, [](const NewEvent &e) {
    printf("Message received %s\n", e.message.c_str());
  });
  
  post(INIT);
  post(CONNECT, 9);
  post(PRESS, KeyboardEvent {14});
  post(NEW_EVENT, NewEvent { "Hello World!" });
  post(ERROR);
  
  return 0;
}

void error() {
    kill(getpid(), SIGABRT);
}
  