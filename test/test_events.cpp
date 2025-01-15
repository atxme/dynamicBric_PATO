////////////////////////////////////////////////////////////
//  event test file
//  implements test cases for event system
//
// Written : 15/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

extern "C" {
    #include "events/xEvent.h"
    #include "xOs/xOsHorodateur.h"
}

class EventTest : public ::testing::Test {
protected:
    void SetUp() override {
        EXPECT_EQ(xEventInit(), XOS_EVENT_OK);
    }
};

// Test initialisation basique
TEST_F(EventTest, BasicInitialization) {
    EXPECT_DEATH(xEventInit(), ".*");
}

// Test publication d'événement basique
TEST_F(EventTest, BasicEventPublish) {
    int data = 42;
    EXPECT_EQ(xEventPublish(1, XOS_EVENT_TYPE_USER, XOS_EVENT_PRIORITY_MEDIUM, 
              XOS_EVENT_FLAG_NONE, &data, sizeof(data)), XOS_EVENT_OK);
}

// Test validation des paramètres
TEST_F(EventTest, InvalidParameters) {
    EXPECT_DEATH(xEventPublish(1, XOS_EVENT_TYPE_USER, XOS_EVENT_PRIORITY_MEDIUM,
                 XOS_EVENT_FLAG_NONE, nullptr, 4), ".*");
}

// Test limite d'événements
TEST_F(EventTest, EventLimit) {
    int data = 42;
    
    for(uint32_t i = 0; i < XOS_EVENT_MAX_EVENTS; i++) {
        EXPECT_EQ(xEventPublish(i + 1, XOS_EVENT_TYPE_USER, XOS_EVENT_PRIORITY_MEDIUM,
                  XOS_EVENT_FLAG_NONE, &data, sizeof(data)), XOS_EVENT_OK);
    }
    
    EXPECT_EQ(xEventPublish(XOS_EVENT_MAX_EVENTS + 1, XOS_EVENT_TYPE_USER,
              XOS_EVENT_PRIORITY_MEDIUM, XOS_EVENT_FLAG_NONE, 
              &data, sizeof(data)), XOS_EVENT_FULL);
}

// Test priorités d'événements
TEST_F(EventTest, EventPriorities) {
    int data = 42;
    
    EXPECT_EQ(xEventPublish(1, XOS_EVENT_TYPE_USER, XOS_EVENT_PRIORITY_LOW,
              XOS_EVENT_FLAG_NONE, &data, sizeof(data)), XOS_EVENT_OK);
    EXPECT_EQ(xEventPublish(2, XOS_EVENT_TYPE_USER, XOS_EVENT_PRIORITY_HIGH,
              XOS_EVENT_FLAG_NONE, &data, sizeof(data)), XOS_EVENT_OK);
    
    EXPECT_EQ(xEventProcess(), XOS_EVENT_OK);
}

// Test événements persistants
TEST_F(EventTest, PersistentEvents) {
    int data = 42;
    
    EXPECT_EQ(xEventPublish(1, XOS_EVENT_TYPE_USER, XOS_EVENT_PRIORITY_MEDIUM,
              XOS_EVENT_FLAG_PERSISTENT, &data, sizeof(data)), XOS_EVENT_OK);
    
    EXPECT_EQ(xEventProcess(), XOS_EVENT_OK);
    EXPECT_EQ(xEventProcess(), XOS_EVENT_OK);
}

// Test séquence d'événements
TEST_F(EventTest, EventSequence) {
    int data = 42;
    
    for(uint32_t i = 0; i < 3; i++) {
        EXPECT_EQ(xEventPublish(i + 1, XOS_EVENT_TYPE_USER, XOS_EVENT_PRIORITY_MEDIUM,
                  XOS_EVENT_FLAG_NONE, &data, sizeof(data)), XOS_EVENT_OK);
    }
    
    EXPECT_EQ(xEventProcess(), XOS_EVENT_OK);
}

// Test horodatage
TEST_F(EventTest, EventTimestamp) {
    int data = 42;
    uint32_t startTime = xHorodateurGet();
    
    EXPECT_EQ(xEventPublish(1, XOS_EVENT_TYPE_USER, XOS_EVENT_PRIORITY_MEDIUM,
              XOS_EVENT_FLAG_NONE, &data, sizeof(data)), XOS_EVENT_OK);
    
    EXPECT_GE(xHorodateurGet(), startTime);
}

// Test traitement file vide
TEST_F(EventTest, EmptyQueueProcess) {
    EXPECT_EQ(xEventProcess(), XOS_EVENT_OK);
}
