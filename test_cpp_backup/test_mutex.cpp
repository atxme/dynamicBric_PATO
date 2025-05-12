////////////////////////////////////////////////////////////
//  mutex test file
//  implements test cases for mutex functions
//
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

extern "C"
{
#include "xOs/xOsMutex.h"
}

class MutexTest : public ::testing::Test
{
protected:
    xOsMutexCtx mutex;

    void SetUp() override
    {
        memset(&mutex, 0, sizeof(xOsMutexCtx));
    }

    void TearDown() override
    {
        if (mutex.t_iState == MUTEX_LOCKED)
        {
            mutexUnlock(&mutex);
        }
        mutexDestroy(&mutex);
    }
};

// Test création basique
TEST_F(MutexTest, BasicCreation)
{
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);
    EXPECT_EQ(mutex.t_iState, MUTEX_UNLOCKED);
    EXPECT_EQ(mutex.t_ulTimeout, MUTEX_DEFAULT_TIMEOUT);
}

// Test verrouillage récursif (supporté par l'implémentation)
TEST_F(MutexTest, RecursiveLocking)
{
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);

    // Premier verrouillage
    EXPECT_EQ(mutexLock(&mutex), MUTEX_OK);
    EXPECT_EQ(mutex.t_iState, MUTEX_LOCKED);

    // Second verrouillage (doit réussir car mutex récursif)
    EXPECT_EQ(mutexLock(&mutex), MUTEX_OK);

    // Double déverrouillage
    EXPECT_EQ(mutexUnlock(&mutex), MUTEX_OK);
    EXPECT_EQ(mutexUnlock(&mutex), MUTEX_OK);
    EXPECT_EQ(mutex.t_iState, MUTEX_UNLOCKED);
}

// Test tryLock avec état
TEST_F(MutexTest, TryLockState)
{
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);

    EXPECT_EQ(mutexTryLock(&mutex), MUTEX_OK);
    EXPECT_EQ(mutex.t_iState, MUTEX_LOCKED);

    std::thread t([this]()
                  { EXPECT_EQ(mutexTryLock(&mutex), MUTEX_TIMEOUT); });

    t.join();
    EXPECT_EQ(mutexUnlock(&mutex), MUTEX_OK);
}

// Test timeout précis
TEST_F(MutexTest, PreciseTimeout)
{
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);
    EXPECT_EQ(mutexLock(&mutex), MUTEX_OK);

    auto start = std::chrono::steady_clock::now();

    std::thread t([this]()
                  { EXPECT_EQ(mutexLockTimeout(&mutex, 100), MUTEX_TIMEOUT); });

    t.join();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Vérifier que le timeout est proche de 100ms
    EXPECT_GE(duration.count(), 95);
    EXPECT_LE(duration.count(), 150);
}

// Test modification timeout
TEST_F(MutexTest, TimeoutModification)
{
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);

    EXPECT_EQ(mutexSetTimeout(&mutex, 2000), MUTEX_OK);
    EXPECT_EQ(mutex.t_ulTimeout, 2000);

    EXPECT_EQ(mutexSetTimeout(&mutex, MUTEX_DEFAULT_TIMEOUT), MUTEX_OK);
    EXPECT_EQ(mutex.t_ulTimeout, MUTEX_DEFAULT_TIMEOUT);
}

// Test accès concurrent avec vérification d'état
TEST_F(MutexTest, ConcurrentAccessWithState)
{
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);
    int sharedCounter = 0;

    auto incrementCounter = [this, &sharedCounter]()
    {
        for (int i = 0; i < 1000; i++)
        {
            EXPECT_EQ(mutexLock(&mutex), MUTEX_OK);
            EXPECT_EQ(mutex.t_iState, MUTEX_LOCKED);
            sharedCounter++;
            EXPECT_EQ(mutexUnlock(&mutex), MUTEX_OK);
            EXPECT_EQ(mutex.t_iState, MUTEX_UNLOCKED);
        }
    };

    std::thread t1(incrementCounter);
    std::thread t2(incrementCounter);

    t1.join();
    t2.join();

    EXPECT_EQ(sharedCounter, 2000);
    EXPECT_EQ(mutex.t_iState, MUTEX_UNLOCKED);
}

// Test déverrouillage sans verrouillage (doit déclencher une assertion)
TEST_F(MutexTest, UnlockWithoutLock)
{
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);
    EXPECT_EQ(mutexUnlock(&mutex), MUTEX_ERROR);
}

// Test avec pointeur null (doit déclencher une assertion)
TEST_F(MutexTest, NullPointerHandling)
{
    EXPECT_DEATH(mutexCreate(nullptr), ".*");
    EXPECT_DEATH(mutexLock(nullptr), ".*");
    EXPECT_DEATH(mutexUnlock(nullptr), ".*");
    EXPECT_DEATH(mutexDestroy(nullptr), ".*");
}

// Test de destruction et réutilisation
TEST_F(MutexTest, DestroyAndReuse)
{
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);
    EXPECT_EQ(mutexDestroy(&mutex), MUTEX_OK);
    EXPECT_EQ(mutex.t_iState, MUTEX_UNLOCKED);

    // Recréation après destruction
    EXPECT_EQ(mutexCreate(&mutex), MUTEX_OK);
    EXPECT_EQ(mutexLock(&mutex), MUTEX_OK);
    EXPECT_EQ(mutexUnlock(&mutex), MUTEX_OK);
}
