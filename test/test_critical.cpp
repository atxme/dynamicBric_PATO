////////////////////////////////////////////////////////////
//  critical section test file
//  implements test cases for critical section functions
//
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>

extern "C" {
    #include "xOs/xOsCritical.h"
}

class CriticalTest : public ::testing::Test {
protected:
    os_critical_t critical;

    void SetUp() override {
        memset(&critical, 0, sizeof(os_critical_t));
    }

    void TearDown() override {
        if (critical.iLock == OS_CRITICAL_LOCKED) {
            osCriticalUnlock(&critical);
        }
        osCriticalDestroy(&critical);
    }
};

// Test création basique
TEST_F(CriticalTest, BasicCreation) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(critical.iLock, OS_CRITICAL_UNLOCKED);
    EXPECT_EQ(critical.ulTimeout, OS_CRITICAL_DEFAULT_TIMEOUT);
}

// Test verrouillage/déverrouillage basique
TEST_F(CriticalTest, BasicLockUnlock) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    
    EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(critical.iLock, OS_CRITICAL_LOCKED);
    
    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(critical.iLock, OS_CRITICAL_UNLOCKED);
}

// Test verrouillage récursif
TEST_F(CriticalTest, RecursiveLocking) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    
    EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
    
    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
}

// Test timeout
TEST_F(CriticalTest, Timeout) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    critical.ulTimeout = 100; // 100ms timeout
    
    EXPECT_EQ(osCriticalLockWithTimeout(&critical), OS_CRITICAL_SUCCESS);
    
    std::thread t([this]() {
        EXPECT_EQ(osCriticalLockWithTimeout(&critical), OS_CRITICAL_TIMEOUT);
    });
    
    t.join();
    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
}

// Test accès concurrent
TEST_F(CriticalTest, ConcurrentAccess) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    int sharedCounter = 0;
    const int NUM_ITERATIONS = 1000;
    
    auto incrementCounter = [this, &sharedCounter]() {
        for(int i = 0; i < NUM_ITERATIONS; i++) {
            EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
            sharedCounter++;
            EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
        }
    };
    
    std::thread t1(incrementCounter);
    std::thread t2(incrementCounter);
    
    t1.join();
    t2.join();
    
    EXPECT_EQ(sharedCounter, NUM_ITERATIONS * 2);
}

// Test paramètres invalides
TEST_F(CriticalTest, InvalidParameters) {
    EXPECT_DEATH(osCriticalCreate(nullptr), ".*");
    EXPECT_DEATH(osCriticalLock(nullptr), ".*");
    EXPECT_DEATH(osCriticalUnlock(nullptr), ".*");
    EXPECT_DEATH(osCriticalDestroy(nullptr), ".*");
}

// Test déverrouillage sans verrouillage
TEST_F(CriticalTest, UnlockWithoutLock) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_DEATH(osCriticalUnlock(&critical), ".*");
}

// Test stress avec plusieurs threads
TEST_F(CriticalTest, StressTest) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    const int NUM_THREADS = 10;
    const int ITERATIONS_PER_THREAD = 100;
    int sharedResource = 0;
    
    std::vector<std::thread> threads;
    for(int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([this, &sharedResource, ITERATIONS_PER_THREAD]() {
            for(int j = 0; j < ITERATIONS_PER_THREAD; j++) {
                EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
                sharedResource++;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
            }
        });
    }
    
    for(auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(sharedResource, NUM_THREADS * ITERATIONS_PER_THREAD);
}

// Test de destruction pendant le verrouillage
TEST_F(CriticalTest, DestroyWhileLocked) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalDestroy(&critical), OS_CRITICAL_SUCCESS);
}

// Test de performance
TEST_F(CriticalTest, PerformanceTest) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    const int NUM_OPERATIONS = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for(int i = 0; i < NUM_OPERATIONS; i++) {
        EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
        EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Vérifier que chaque opération prend moins de 10 microsecondes en moyenne
    EXPECT_LT(duration.count() / NUM_OPERATIONS, 10);
}
