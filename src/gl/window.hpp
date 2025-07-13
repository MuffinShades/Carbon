#pragma once
#include <iostream>
#include <vector>
#include <glad.h>
#include <glfw3.h>
#include "../types.hpp"

#define MAKE_GL_VER(major, minor) ((((major) & 0xff) << 8) | ((minor) & 0xff))

constexpr i32 OPENGL_WIN_VER = MAKE_GL_VER(3,5); //version 3.5

enum EventType {
    MouseDown,
    MouseUp,
    MouseMove,
    KeyDown,
    KeyUp,
    _eventTypeLen
};

class Event {
    
};

typedef i64 EventHandle;

typedef void (*_fnEPtr)(Event);

struct _evContainer {
    _fnEPtr fn;
    EventHandle handle;
    _evContainer(EventHandle hnd) : handle(hnd) {};
};

class Window {
private:
    std::vector<_evContainer> _listeners[_eventTypeLen] = {};
    static EventHandle _cHandle;
    GLFWwindow *wHandle;
    void  intCreate();
    bool running = false;
public:
    static bool winIni();
    i32 w, h;
    const char *title;

    Window() {}; //default constructor thingy
    Window(std::string title, i32 w, i32 h) : w(w), h(h), title(title.c_str()) {
        this->intCreate();
    }
    bool Update();
    bool isRunning();
    EventHandle AddEventListener(enum EventType ty, void (*fn)(Event));
    bool CancelEventListener(EventHandle handle);
};