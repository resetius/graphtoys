#pragma once

struct InputEvent {
    int key;
    int scancode;
    int action;
    int mods;

    double x;
    double y;
    double dx;
    double dy;

    unsigned char* mask;
};

struct EventConsumer {
    void (*key_event)(struct EventConsumer*, struct InputEvent* ev);
    void (*mouse_move_event)(struct EventConsumer*, struct InputEvent* ev);
};

struct EventProducer {
    void (*subscribe)(struct EventProducer*, struct EventConsumer*);
};
