////////////////////////////////////////////////////////////
//  event queue test file
//  implements test cases for event queue
//
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <vector>

extern "C" {
    #include "events/xEventQueue.h"
}

class EventQueueTest : public ::testing::Test {
protected:
    xos_event_queue_t queue;
    
    void SetUp() override {
        EXPECT_EQ(xEventQueueInit(&queue), XOS_EVENT_QUEUE_OK);
    }
    
    // Helper pour créer un événement de test
    xos_event_t createTestEvent(uint32_t id) {
        xos_event_t event = {0};
        event.t_ulEventId = id;
        event.t_eType = XOS_EVENT_TYPE_USER;
        event.t_tPrio = XOS_EVENT_PRIORITY_MEDIUM;
        event.t_ucFlags = XOS_EVENT_FLAG_NONE;
        return event;
    }
};

// Test initialisation
TEST_F(EventQueueTest, Initialization) {
    xos_event_queue_t testQueue;
    EXPECT_EQ(xEventQueueInit(&testQueue), XOS_EVENT_QUEUE_OK);
    EXPECT_TRUE(testQueue.t_bInitialized);
    EXPECT_EQ(testQueue.t_ulCount, 0);
    EXPECT_EQ(testQueue.t_ulDropped, 0);
}

// Test paramètres invalides
TEST_F(EventQueueTest, InvalidParameters) {
    xos_event_t event;
    EXPECT_DEATH(xEventQueueInit(nullptr), ".*");
    EXPECT_DEATH(xEventQueuePush(nullptr, &event), ".*");
    EXPECT_DEATH(xEventQueuePush(&queue, nullptr), ".*");
    EXPECT_DEATH(xEventQueuePop(nullptr, &event), ".*");
    EXPECT_DEATH(xEventQueuePop(&queue, nullptr), ".*");
}

// Test opérations basiques
TEST_F(EventQueueTest, BasicOperations) {
    xos_event_t event = createTestEvent(1);
    xos_event_t receivedEvent;
    
    EXPECT_EQ(xEventQueuePush(&queue, &event), XOS_EVENT_QUEUE_OK);
    EXPECT_EQ(queue.t_ulCount, 1);
    
    EXPECT_EQ(xEventQueuePop(&queue, &receivedEvent), XOS_EVENT_QUEUE_OK);
    EXPECT_EQ(queue.t_ulCount, 0);
    EXPECT_EQ(receivedEvent.t_ulEventId, event.t_ulEventId);
}

// Test file pleine
TEST_F(EventQueueTest, QueueFull) {
    xos_event_t event = createTestEvent(1);
    
    // Remplir la file
    for(uint32_t i = 0; i < XOS_EVENT_QUEUE_MAX_SIZE; i++) {
        EXPECT_EQ(xEventQueuePush(&queue, &event), XOS_EVENT_QUEUE_OK);
    }
    
    // Vérifier que la file est pleine
    EXPECT_TRUE(xEventQueueIsFull(&queue));
    EXPECT_EQ(xEventQueuePush(&queue, &event), XOS_EVENT_QUEUE_FULL);
    EXPECT_EQ(queue.t_ulDropped, 1);
}

// Test file vide
TEST_F(EventQueueTest, QueueEmpty) {
    xos_event_t event;
    EXPECT_TRUE(xEventQueueIsEmpty(&queue));
    EXPECT_EQ(xEventQueuePop(&queue, &event), XOS_EVENT_QUEUE_EMPTY);
}

// Test cycle complet
TEST_F(EventQueueTest, FullCycle) {
    std::vector<xos_event_t> events;
    const int NUM_EVENTS = XOS_EVENT_QUEUE_MAX_SIZE;
    
    // Pousser des événements
    for(int i = 0; i < NUM_EVENTS; i++) {
        xos_event_t event = createTestEvent(i);
        events.push_back(event);
        EXPECT_EQ(xEventQueuePush(&queue, &event), XOS_EVENT_QUEUE_OK);
    }
    
    // Récupérer tous les événements
    for(int i = 0; i < NUM_EVENTS; i++) {
        xos_event_t receivedEvent;
        EXPECT_EQ(xEventQueuePop(&queue, &receivedEvent), XOS_EVENT_QUEUE_OK);
        EXPECT_EQ(receivedEvent.t_ulEventId, events[i].t_ulEventId);
    }
}

// Test statistiques
TEST_F(EventQueueTest, Statistics) {
    uint32_t count, dropped;
    xos_event_t event = createTestEvent(1);
    
    EXPECT_EQ(xEventQueueGetStats(&queue, &count, &dropped), XOS_EVENT_QUEUE_OK);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(dropped, 0);
    
    // Ajouter quelques événements
    EXPECT_EQ(xEventQueuePush(&queue, &event), XOS_EVENT_QUEUE_OK);
    EXPECT_EQ(xEventQueuePush(&queue, &event), XOS_EVENT_QUEUE_OK);
    
    EXPECT_EQ(xEventQueueGetStats(&queue, &count, &dropped), XOS_EVENT_QUEUE_OK);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(dropped, 0);
}

// Test comportement circulaire
TEST_F(EventQueueTest, CircularBehavior) {
    xos_event_t event;
    const int HALF_SIZE = XOS_EVENT_QUEUE_MAX_SIZE / 2;
    
    // Remplir la moitié de la file
    for(int i = 0; i < HALF_SIZE; i++) {
        event = createTestEvent(i);
        EXPECT_EQ(xEventQueuePush(&queue, &event), XOS_EVENT_QUEUE_OK);
    }
    
    // Retirer la moitié
    for(int i = 0; i < HALF_SIZE; i++) {
        EXPECT_EQ(xEventQueuePop(&queue, &event), XOS_EVENT_QUEUE_OK);
        EXPECT_EQ(event.t_ulEventId, (uint32_t)i);
    }
    
    // Ajouter de nouveaux événements
    for(int i = 0; i < HALF_SIZE; i++) {
        event = createTestEvent(i + HALF_SIZE);
        EXPECT_EQ(xEventQueuePush(&queue, &event), XOS_EVENT_QUEUE_OK);
    }
    
    // Vérifier que tout est correct
    for(int i = 0; i < HALF_SIZE; i++) {
        EXPECT_EQ(xEventQueuePop(&queue, &event), XOS_EVENT_QUEUE_OK);
        EXPECT_EQ(event.t_ulEventId, (uint32_t)(i + HALF_SIZE));
    }
}

// Test accès concurrents
TEST_F(EventQueueTest, ConcurrentAccess) {
    const int NUM_PRODUCERS = 5;
    const int NUM_EVENTS_PER_PRODUCER = 100;
    std::atomic<int> totalPushed(0);
    std::atomic<int> totalPopped(0);
    
    // Producteurs
    auto producer = [&](int id) {
        for(int i = 0; i < NUM_EVENTS_PER_PRODUCER; i++) {
            xos_event_t event = createTestEvent(id * 1000 + i);
            if(xEventQueuePush(&queue, &event) == XOS_EVENT_QUEUE_OK) {
                totalPushed++;
            }
        }
    };
    
    // Consommateur
    auto consumer = [&]() {
        xos_event_t event;
        while(totalPopped < totalPushed || totalPushed < NUM_PRODUCERS * NUM_EVENTS_PER_PRODUCER) {
            if(xEventQueuePop(&queue, &event) == XOS_EVENT_QUEUE_OK) {
                totalPopped++;
            }
            std::this_thread::yield();
        }
    };
    
    // Démarrer les threads
    std::vector<std::thread> producers;
    for(int i = 0; i < NUM_PRODUCERS; i++) {
        producers.emplace_back(producer, i);
    }
    std::thread consumerThread(consumer);
    
    // Attendre la fin
    for(auto& t : producers) {
        t.join();
    }
    consumerThread.join();
    
    // Vérifier les résultats
    EXPECT_EQ(totalPushed, totalPopped);
}
