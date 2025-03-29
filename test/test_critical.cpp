#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>
#ifdef __cplusplus
#include <atomic>
#else
#include <stdatomic.h>
#endif

extern "C" {
#include "xOsCritical.h"
}

class CriticalTest : public ::testing::Test {
protected:
    t_osCriticalCtx critical;

    void SetUp() override {
        // Initialiser les membres individuellement au lieu d'utiliser memset
        // pour éviter le warning sur les types atomiques
        pthread_mutex_init(&critical.critical, nullptr);
#ifdef __cplusplus
        critical.a_usLockCounter.store(0);
        critical.a_bLock.store(false);
#else
        atomic_store(&critical.a_usLockCounter, 0);
        atomic_store(&critical.a_bLock, false);
#endif
    }

    void TearDown() override {
        // Si la section critique est toujours verrouillée, la débloquer avant destruction.
        if (atomic_load(&critical.a_bLock))
            osCriticalUnlock(&critical);
        osCriticalDestroy(&critical);
    }
};

// Test de création basique : vérifie que la section critique est initialisée avec des valeurs correctes.
TEST_F(CriticalTest, BasicCreation) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(atomic_load(&critical.a_usLockCounter), 0);
    EXPECT_FALSE(atomic_load(&critical.a_bLock));
}

// Test de verrouillage/déverrouillage simple.
TEST_F(CriticalTest, BasicLockUnlock) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);

    EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
    // Après un verrouillage, le compteur doit être incrémenté et le flag à true.
    EXPECT_EQ(atomic_load(&critical.a_usLockCounter), 1);
    EXPECT_TRUE(atomic_load(&critical.a_bLock));

    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(atomic_load(&critical.a_usLockCounter), 0);
    EXPECT_FALSE(atomic_load(&critical.a_bLock));
}

// Test de verrouillage récursif (le mutex est récursif, ce qui permet plusieurs verrouillages du même thread).
TEST_F(CriticalTest, RecursiveLocking) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);

    EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(atomic_load(&critical.a_usLockCounter), 2);
    EXPECT_TRUE(atomic_load(&critical.a_bLock));

    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(atomic_load(&critical.a_usLockCounter), 1);

    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(atomic_load(&critical.a_usLockCounter), 0);
    EXPECT_FALSE(atomic_load(&critical.a_bLock));
}

// Test d'accès concurrent sur une variable partagée (synchronisation par la section critique).
TEST_F(CriticalTest, ConcurrentAccess) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    int sharedCounter = 0;
    const int NUM_ITERATIONS = 1000;

    auto incrementCounter = [this, &sharedCounter]() {
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
            sharedCounter++;
            EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
        }
        };

    std::thread t1(incrementCounter);
    std::thread t2(incrementCounter);
    t1.join(); t2.join();

    EXPECT_EQ(sharedCounter, NUM_ITERATIONS * 2);
}

// Test des paramètres invalides grâce aux assertions qui appellent exit(1).
TEST_F(CriticalTest, InvalidParameters) {
    EXPECT_DEATH(osCriticalCreate(nullptr), ".*");
    EXPECT_DEATH(osCriticalLock(nullptr), ".*");
    EXPECT_DEATH(osCriticalUnlock(nullptr), ".*");
    EXPECT_DEATH(osCriticalDestroy(nullptr), ".*");
}

// Test du déverrouillage sans verrouillage préalable.
TEST_F(CriticalTest, UnlockWithoutLock) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_ERROR);
}

// Test de destruction lorsque la section critique est verrouillée.
TEST_F(CriticalTest, DestroyWhileLocked) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
    // La destruction doit échouer tant que le mutex est verrouillé.
    EXPECT_EQ(osCriticalDestroy(&critical), OS_CRITICAL_ERROR);
    // Après déverrouillage, la destruction doit réussir.
    EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    EXPECT_EQ(osCriticalDestroy(&critical), OS_CRITICAL_SUCCESS);
}

// Test de performance sur un grand nombre d'opérations lock/unlock.
TEST_F(CriticalTest, PerformanceTest) {
    EXPECT_EQ(osCriticalCreate(&critical), OS_CRITICAL_SUCCESS);
    const int NUM_OPERATIONS = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_OPERATIONS; i++) {
        EXPECT_EQ(osCriticalLock(&critical), OS_CRITICAL_SUCCESS);
        EXPECT_EQ(osCriticalUnlock(&critical), OS_CRITICAL_SUCCESS);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto avgTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / NUM_OPERATIONS;
    EXPECT_LT(avgTime, 10); // en moyenne, moins de 10µs par opération
}
