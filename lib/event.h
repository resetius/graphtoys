#pragma once

struct EventConsumer {
    void (*key_event)(struct EventConsumer*, int key, int scancode, int action, int mods, unsigned char* mask);
};

struct EventProducer {
    void (*subscribe)(struct EventProducer*, struct EventConsumer*);
};
