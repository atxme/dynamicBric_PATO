////////////////////////////////////////////////////////////
//  event handler test file
//  implements test cases for event handler
//
// Written : 15/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

extern "C" {
    #include "events/xEventHandler.h"
}

class EventHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        EXPECT_EQ(xEventHandlerInit(), XOS_EVENT_HANDLER_OK);
    }

    void TearDown() override {
        EXPECT_EQ(xEventHandlerCleanup(), XOS_EVENT_HANDLER_OK);
    }

    static void TestCallback(xos_event_t* event, void* arg) {
        if (arg) {
            (*(int*)arg)++;
        }
    }

    static void SlowCallback(xos_event_t* event, void* arg) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
};

// Test initialisation
TEST_F(EventHandlerTest, Initialization) {
    EXPECT_EQ(xEventHandlerCleanup(), XOS_EVENT_HANDLER_OK);
    EXPECT_EQ(xEventHandlerInit(), XOS_EVENT_HANDLER_OK);
}

// Test souscription basique
TEST_F(EventHandlerTest, BasicSubscription) {
    int callCount = 0;
    EXPECT_EQ(xEventHandlerSubscribe(1, TestCallback, &callCount, 0), 
              XOS_EVENT_HANDLER_OK);

    xos_event_t event;
    event.t_ulEventId = 1;
    EXPECT_EQ(xEventHandlerProcess(&event), XOS_EVENT_HANDLER_OK);
    EXPECT_EQ(callCount, 1);
}

// Test désinscription
TEST_F(EventHandlerTest, Unsubscription) {
    int callCount = 0;
    EXPECT_EQ(xEventHandlerSubscribe(1, TestCallback, &callCount, 0), 
              XOS_EVENT_HANDLER_OK);
    EXPECT_EQ(xEventHandlerUnsubscribe(1, TestCallback), XOS_EVENT_HANDLER_OK);
    
    xos_event_t event;
    event.t_ulEventId = 1;
    EXPECT_EQ(xEventHandlerProcess(&event), XOS_EVENT_HANDLER_OK);
    EXPECT_EQ(callCount, 0);
}

// Test souscription multiple
TEST_F(EventHandlerTest, MultipleSubscribers) {
    int count1 = 0, count2 = 0;
    EXPECT_EQ(xEventHandlerSubscribe(1, TestCallback, &count1, 0), 
              XOS_EVENT_HANDLER_OK);
    EXPECT_EQ(xEventHandlerSubscribe(1, TestCallback, &count2, 0), 
              XOS_EVENT_HANDLER_OK);

    xos_event_t event;
    event.t_ulEventId = 1;
    EXPECT_EQ(xEventHandlerProcess(&event), XOS_EVENT_HANDLER_OK);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
}

// Test limite de souscripteurs
TEST_F(EventHandlerTest, SubscriberLimit) {
    for(uint32_t i = 0; i < XOS_EVENT_HANDLER_MAX_SUBSCRIBERS; i++) {
        EXPECT_EQ(xEventHandlerSubscribe(i, TestCallback, nullptr, 0), 
                  XOS_EVENT_HANDLER_OK);
    }
    
    EXPECT_EQ(xEventHandlerSubscribe(100, TestCallback, nullptr, 0), 
              XOS_EVENT_HANDLER_FULL);
}

// Test timeout callback
TEST_F(EventHandlerTest, CallbackTimeout) {
    EXPECT_EQ(xEventHandlerSubscribe(1, SlowCallback, nullptr, 0), 
              XOS_EVENT_HANDLER_OK);

    xos_event_t event;
    event.t_ulEventId = 1;
    EXPECT_EQ(xEventHandlerProcess(&event), XOS_EVENT_HANDLER_TIMEOUT);
}

// Test événement invalide
TEST_F(EventHandlerTest, InvalidEvent) {
    xos_event_t event;
    event.t_eType = (xos_event_type_t)100;  // Type invalide
    EXPECT_EQ(xEventHandlerProcess(&event), XOS_EVENT_HANDLER_INVALID);
}

// Test accès concurrents
TEST_F(EventHandlerTest, ConcurrentAccess) {
    const int NUM_THREADS = 10;
    std::atomic<int> totalCallbacks(0);
    
    for(int i = 0; i < NUM_THREADS; i++) {
        EXPECT_EQ(xEventHandlerSubscribe(1, TestCallback, &totalCallbacks, 0), 
                  XOS_EVENT_HANDLER_OK);
    }

    std::vector<std::thread> threads;
    for(int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([this]() {
            xos_event_t event;
            event.t_ulEventId = 1;
            EXPECT_EQ(xEventHandlerProcess(&event), XOS_EVENT_HANDLER_OK);
        });
    }

    for(auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(totalCallbacks, NUM_THREADS * NUM_THREADS);
}

// Test nettoyage avec callbacks actifs
TEST_F(EventHandlerTest, CleanupWithActiveCallbacks) {
    const int TIMEOUT_MS = 1000;
    std::atomic<bool> callbackRunning(false);
    std::atomic<bool> shouldStop(false);

    auto longCallback = [](xos_event_t* event, void* arg) {
        auto running = (std::atomic<bool>*)arg;
        *running = true;

        // Réduire le temps de sommeil pour éviter les blocages
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        *running = false;
        };

    EXPECT_EQ(xEventHandlerSubscribe(1, longCallback, &callbackRunning, 0),
        XOS_EVENT_HANDLER_OK);

    auto start = std::chrono::steady_clock::now();

    std::thread t([&]() {
        xos_event_t event;
        event.t_ulEventId = 1;

        while (!shouldStop) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::steady_clock::now() - start).count();

            if (elapsed > TIMEOUT_MS) {
                shouldStop = true;
                break;
            }

            if (xEventHandlerProcess(&event) != XOS_EVENT_HANDLER_OK) {
                break;
            }
            std::this_thread::yield();
        }
        });

    // Attendre que le callback démarre avec timeout
    bool started = false;
    while (!started && !shouldStop) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::steady_clock::now() - start).count();

        if (elapsed > TIMEOUT_MS) {
            shouldStop = true;
            break;
        }

        if (callbackRunning) {
            started = true;
        }
        std::this_thread::yield();
    }

    EXPECT_TRUE(started) << "Le callback n'a pas démarré dans le délai imparti";
    EXPECT_EQ(xEventHandlerCleanup(), XOS_EVENT_HANDLER_OK);

    shouldStop = true;
    t.join();

    EXPECT_FALSE(callbackRunning) << "Le callback est toujours en cours d'exécution";
}

