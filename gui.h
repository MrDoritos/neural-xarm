#pragma once
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

extern int errno;

namespace gui {

using clk = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<clk>;
template <typename T = double>
using dur = std::chrono::duration<T>;

template<typename Iter, typename Func>
inline Func for_each(Iter &iterable, Func func) {
    return std::for_each(iterable.begin(), iterable.end(), func);
}

template<typename Func, typename Iter, typename ...Args>
inline void for_each(Iter &iterable, Args&&...args) {
    gui::for_each<Iter>(iterable, [args...](Func &func) { func(args...); });
}


enum LogLevel {
    QUIET = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    VERBOSE = 4,
    PEDANTIC = 5
};

namespace events {
    using event_t = uint32_t;

    struct KeyboardEvent {
        int keys;
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

struct Element {
    virtual void init() = 0;
    virtual void destroy() = 0;
};

LogLevel logLevel = LogLevel::INFO;
FILE *logFile = stderr;

template<typename ...Args>
int log(const char *format, Args &&... args) {
    return log(gui::INFO, format, std::forward<Args>(args)...);
}

template<typename ...Args>
int log(LogLevel level, const char *format, Args &&... args) {
    if (level > logLevel)
        return 0;

    return fprintf(logFile, format, std::forward<Args>(args)...);
}

int log(const char *format) { 
    return log("%s", format); 
}

int log(const std::string &str) {
    return log("%s", str.c_str());
}

void safe_exit(int errcode = 0) {
    exit(errcode);
}

void fatal(const std::string &str, int errcode = -1) {
    log("Error %s", str.c_str());
    safe_exit(errcode);
}

void fatal_errno(const std::string &str) {
    int _errno = errno;
    log("Error %s, %s %i", str.c_str(), strerror(_errno), _errno);
    safe_exit(_errno);
}

}