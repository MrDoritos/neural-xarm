#include "../gui.h"

enum ElementEvents : gui::events::event_t {
    ELEMENT_INIT,
    ELEMENT_REGISTER,
    ELEMENT_DESTROY,
    ELEMENT_UNREGISTER
};

struct EventElement;

template<>
struct gui::events::FunctionType<ElementEvents> { using type = std::function<void()>; };

struct EventElement : public gui::Element {
    EventElement() {
        gui::subscribe(ELEMENT_INIT, std::bind(&EventElement::init, this));
        gui::subscribe(ELEMENT_DESTROY, std::bind(&EventElement::destroy, this));
    }

    void init() override {
        gui::log("Init Event\n");
    }

    void destroy() override {
        gui::log("Destroy Event\n");
    }
};

struct OtherElement : public gui::Element {
    OtherElement() {
        gui::subscribe(ELEMENT_INIT, std::bind(&OtherElement::init, this));
        gui::subscribe(ELEMENT_DESTROY, std::bind(&OtherElement::destroy, this));
    }

    void init() override {
        gui::log("Init Other\n");
    }

    void destroy() override {
        gui::log("Destroy Other\n");
    }
};

void eventslol() {
    using namespace gui::events;
    using namespace gui;

    subscribe(PRESS, [](KeyboardEvent i){
        log("Press %i\n", i.keys);
    });

    subscribe(SAFE_EXIT, []() {
        log("Safely exiting\n");
    });

    post(PRESS, KeyboardEvent {1});
}

int main() {
    EventElement e1;
    OtherElement e2;

    gui::subscribe(gui::events::CONNECT, [](int i){
        gui::log("Connect %i\n", i);
    });

    gui::subscribe(gui::events::INIT, [](){
        gui::log("Program starting\n");

        eventslol();
    });

    gui::subscribe(gui::events::BUTTON, [](int i){
        gui::log("Button %i\n", i);
    });

    gui::subscribe(gui::events::CONNECT, [](int i) {
        gui::log("Connect %i\n", i + 1);
    });

    gui::post(gui::events::INIT);
    gui::post(ELEMENT_INIT);

    gui::post(gui::events::CONNECT, 1);
    gui::post(gui::events::BUTTON, 1);

    gui::post(ELEMENT_DESTROY);

    gui::post(gui::events::ERROR);
    gui::post(gui::events::SAFE_EXIT);

    return 0;
}