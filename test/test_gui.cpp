#include "../gui.h"

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

    gui::post(gui::events::CONNECT, 1);
    gui::post(gui::events::BUTTON, 1);

    gui::post(gui::events::ERROR);
    gui::post(gui::events::SAFE_EXIT);

    return 0;
}