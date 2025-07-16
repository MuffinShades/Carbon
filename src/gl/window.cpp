#include "window.hpp"

EventHandle Window::_cHandle = 0;

EventHandle Window::AddEventListener(enum EventType ty, void (*fn)(Event)) {
    if (ty >= _eventTypeLen)
        return -1;
    _evContainer container(Window::_cHandle);
    container.fn = fn;
    this->_listeners[ty].push_back(container);
    return Window::_cHandle++;
}

//do glfw or whatever stuff here
void Window::intCreate() {
    this->wHandle = glfwCreateWindow(
        this->w, 
        this->h, 
        this->title, 
        nullptr, 
        nullptr
    );

    if (!this->wHandle) {
        std::cout << "Failed to create Window!" << std::endl;
        this->wHandle = nullptr;
        return;
    }

    glfwMakeContextCurrent(this->wHandle);

    //GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "no load gl :/" << std::endl;
        this->wHandle = nullptr;
        return;
    }

    this->running = true;
}

bool Window::isRunning() {
    return this->running;
}

bool Window::Update() {
    this->running = !glfwWindowShouldClose(this->wHandle);
    glfwPollEvents();
    glfwSwapBuffers(this->wHandle);
    return true; //just do this for now idk why i made it return something
}

bool Window::winIni() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    return true;
}