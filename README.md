# Single-threaded C++17 event bus.

This lib was inspired by https://github.com/gelldur/EventBus

I often need some kind of simple system of cross-coupling callbacks. Some kind of event bus system.
This is a cross-coupling from a call stack perspective.
And this is a decoupling system from an architectural point of view.

I had some eventbus implementations tailored to the place of use.
But after seeing Gelldur’s eventbus, I realized that it was time to highlight such needs in one small and simple header.

It seemed reasonable to me to remove everything unnecessary from the project, everything that is not related to the task of systematizing callbacks.
Thus, for example, I transferred the responsibility of working with multithreading to the use side and abandoned any inter-threaded synchronization.

So this project is a compilation of ideas from various sources and partly from Gelldur's Eventbus.

## How attach it to your project
Just copy the eventbus.h header file to where you need it.

## How to use it

TODO: add api mini-description.

TODO: describe the warning about capturing `this` in lambda callback - the captured object can not be moved because its address has been captured (obviously) and stored in long-life storage in the eventbus system.

Here are some examples of use.

```C++
#include "eventbus.h"

#include <string>
#include <cassert>

void example1()
{
    struct MyStateUpdate { int payload; };

    auto bus = std::make_shared<eventbus::EventBus>();

    {
        auto listener = eventbus::Listener(bus);

        listener.listen<MyStateUpdate>([](const MyStateUpdate& e) {
            assert(e.payload == 123);
        });

        bus->immediate(MyStateUpdate{ 123 });
    }
}

void example2()
{
    struct MessageReceived { std::string str; };

    auto bus = std::make_shared<eventbus::EventBus>();

    {
        auto listener = eventbus::Listener(bus);

        int eventsCount = 0;
        listener.listen<MessageReceived>([&eventsCount](const MessageReceived& e) {
            assert(
                (eventsCount == 0 && e.str == "hello") || 
                (eventsCount == 1 && e.str == "world")
            );
            ++eventsCount;
        });

        bus->post(MessageReceived{ "hello" });
        bus->post(MessageReceived{ "world" });
        bus->process();

        assert(eventsCount == 2);
    }
}

void example3()
{
    struct MyStateUpdate { int payload; };
    struct MessageReceived { std::string str; };

    auto bus = std::make_shared<eventbus::EventBus>();

    {
        auto listener = eventbus::Listener(bus);

        listener.listen<MyStateUpdate>([](const MyStateUpdate& e) {
            assert(e.payload == 123);
        });

        int messagesCount = 0;
        listener.listen<MessageReceived>([&messagesCount](const MessageReceived& e) {
            assert(
                (messagesCount == 0 && e.str == "hello") ||
                (messagesCount == 1 && e.str == "world")
            );
            ++messagesCount;
        });

        bus->immediate(MyStateUpdate{ 123 });

        bus->post(MessageReceived{ "hello" });
        bus->post(MessageReceived{ "world" });
        bus->process();

        assert(messagesCount == 2);
    }
}

void test1()
{
    auto bus = std::make_shared<eventbus::EventBus>();

    const int cycles_count = 100;

    struct A {};

    for (int i = 0; i < cycles_count; ++i)
    {
        int checks = 0;

        auto listener1 = eventbus::Listener(bus);
        listener1.listen<A>([&checks](const A&) {
            checks++;
        });

        int count = 100;
        for (int j = 0; j < count; ++j) bus->immediate(A());

        assert(checks == count);
    }

    return;
}

void test2()
{
    auto bus = std::make_shared<eventbus::EventBus>();

    const int cycles_count = 100;

    struct A {};

    for (int i = 0; i < cycles_count; ++i)
    {
        int checks = 0;

        std::vector<std::shared_ptr<eventbus::Listener>> ar;

        int count = 100;
        for (int j = 0; j < count; ++j)
        {
            auto p = std::make_shared<eventbus::Listener>(bus);
            ar.emplace_back(std::move(p));
            ar.rbegin()->get()->listen<A>([&checks](const A&) {
                checks++;
            });
        }

        bus->immediate(A());

        assert(checks == count);
    }

    return;
}

void test3()
{
    auto bus = std::make_shared<eventbus::EventBus>();

    const int cycles_count = 100;

    struct A {};

    for (int i = 0; i < cycles_count; ++i)
    {
        int checks = 0;

        std::vector<std::shared_ptr<eventbus::Listener>> ar;

        int count = 100;
        for (int j = 0; j < count; ++j)
        {
            auto p = std::make_shared<eventbus::Listener>(bus);
            ar.emplace_back(std::move(p));
            ar.rbegin()->get()->listen<A>([&checks](const A&) {
                checks++;
            });

            bus->post(A());
        }

        bus->process();

        assert(checks == count * count);
    }

    return;
}
```

## License
MIT
