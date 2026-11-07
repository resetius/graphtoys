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
    const struct EventConsumerVtbl* vtbl;
};

struct EventConsumerVtbl {
    void (*key_event)(struct EventConsumer*, struct InputEvent* ev);
    void (*mouse_move_event)(struct EventConsumer*, struct InputEvent* ev);
};

struct EventProducer {
    const struct EventProducerVtbl* vtbl;
};

struct EventProducerVtbl {
    void (*subscribe)(struct EventProducer*, struct EventConsumer*);
};

#define ev_key_event(c, ev) ((c)->vtbl->key_event((c), (ev)))
#define ev_mouse_move_event(c, ev) ((c)->vtbl->mouse_move_event((c), (ev)))
#define ev_subscribe(p, c) ((p)->vtbl->subscribe((p), (c)))
