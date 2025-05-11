#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>
#include <functional>

extern "C" {
#include "xOsSemaphore.h"
}

TEST(SemaphoreTest, Initialization) {
    t_OSSemCtx sem;
    EXPECT_EQ(osSemInit(&sem, 1, nullptr), OS_SEM_SUCCESS);
    EXPECT_EQ(sem.initialized, 1);
    EXPECT_EQ(sem.value, 1);
    EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
}

TEST(SemaphoreTest, InitializationWithName) {
    t_OSSemCtx sem;
    EXPECT_EQ(osSemInit(&sem, 1, "/test_sem"), OS_SEM_SUCCESS);
    EXPECT_EQ(sem.initialized, 1);
    EXPECT_EQ(sem.value, 1);
    EXPECT_STREQ(sem.name, "/test_sem");
    EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
}

TEST(SemaphoreTest, InvalidInitialization) {
    t_OSSemCtx sem;
    // Test avec une valeur initiale négative
    EXPECT_EQ(osSemInit(&sem, -1, nullptr), OS_SEM_ERROR);
    // Test avec un pointeur NULL
    EXPECT_EQ(osSemInit(nullptr, 1, nullptr), OS_SEM_ERROR);
}

TEST(SemaphoreTest, WaitAndPost) {
    t_OSSemCtx sem;
    EXPECT_EQ(osSemInit(&sem, 1, nullptr), OS_SEM_SUCCESS);
    
    // Attend une fois, devrait réussir immédiatement
    EXPECT_EQ(osSemWait(&sem), OS_SEM_SUCCESS);
    
    // La valeur devrait être 0 maintenant
    int value;
    EXPECT_EQ(osSemGetValue(&sem, &value), OS_SEM_SUCCESS);
    EXPECT_EQ(value, 0);
    
    // Incrémente le sémaphore
    EXPECT_EQ(osSemPost(&sem), OS_SEM_SUCCESS);
    
    // La valeur devrait être 1 maintenant
    EXPECT_EQ(osSemGetValue(&sem, &value), OS_SEM_SUCCESS);
    EXPECT_EQ(value, 1);
    
    EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
}

TEST(SemaphoreTest, TryWait) {
    t_OSSemCtx sem;
    EXPECT_EQ(osSemInit(&sem, 1, nullptr), OS_SEM_SUCCESS);
    
    // Premier try_wait devrait réussir
    EXPECT_EQ(osSemTryWait(&sem), OS_SEM_SUCCESS);
    
    // Second try_wait devrait échouer car le sémaphore est à 0
    EXPECT_EQ(osSemTryWait(&sem), OS_SEM_NOT_AVAILABLE);
    
    // Libérer le sémaphore
    EXPECT_EQ(osSemPost(&sem), OS_SEM_SUCCESS);
    
    EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
}

TEST(SemaphoreTest, WaitTimeout) {
    t_OSSemCtx sem;
    EXPECT_EQ(osSemInit(&sem, 1, nullptr), OS_SEM_SUCCESS);
    
    // Premier wait_timeout devrait réussir immédiatement
    EXPECT_EQ(osSemWaitTimeout(&sem, 100), OS_SEM_SUCCESS);
    
    // Second wait_timeout devrait expirer après 100ms
    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(osSemWaitTimeout(&sem, 100), OS_SEM_TIMEOUT);
    auto end = std::chrono::steady_clock::now();
    
    // Vérifier que le délai d'attente était d'environ 100ms (avec une marge)
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(duration, 90);  // Un peu moins que 100ms pour tenir compte des imprécisions
    EXPECT_LE(duration, 200); // Un peu plus que 100ms pour tenir compte des surcharges du système
    
    EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
}

// Test de concurrence avec des threads
TEST(SemaphoreTest, MultithreadAccess) {
    t_OSSemCtx sem;
    EXPECT_EQ(osSemInit(&sem, 0, nullptr), OS_SEM_SUCCESS);
    
    const int NUM_ITERATIONS = 5;
    int counter = 0;
    
    // Thread producteur qui incrémente le compteur et poste sur le sémaphore
    auto producer = [&sem, &counter]() {
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            osSemPost(&sem);
        }
    };
    
    // Thread consommateur qui attend sur le sémaphore et vérifie que le compteur a été incrémenté
    auto consumer = [&sem, &counter]() {
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            EXPECT_EQ(osSemWait(&sem), OS_SEM_SUCCESS);
            EXPECT_GE(counter, i + 1); // Le compteur devrait être au moins à i+1
        }
    };
    
    // Démarrer le thread consommateur d'abord
    std::thread consumerThread(consumer);
    
    // Attendre un peu pour s'assurer que le consommateur est bien en attente
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Démarrer le thread producteur
    std::thread producerThread(producer);
    
    // Attendre la fin des threads
    producerThread.join();
    consumerThread.join();
    
    // Vérifier que le compteur a la bonne valeur
    EXPECT_EQ(counter, NUM_ITERATIONS);
    
    EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
}

TEST(SemaphoreTest, GetValue) {
    t_OSSemCtx sem;
    EXPECT_EQ(osSemInit(&sem, 5, nullptr), OS_SEM_SUCCESS);
    
    int value;
    EXPECT_EQ(osSemGetValue(&sem, &value), OS_SEM_SUCCESS);
    EXPECT_EQ(value, 5);
    
    // Décrémenter trois fois
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(osSemWait(&sem), OS_SEM_SUCCESS);
    }
    
    EXPECT_EQ(osSemGetValue(&sem, &value), OS_SEM_SUCCESS);
    EXPECT_EQ(value, 2);
    
    EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
}

TEST(SemaphoreTest, InvalidParameters) {
    t_OSSemCtx sem;
    EXPECT_EQ(osSemInit(&sem, 1, nullptr), OS_SEM_SUCCESS);
    
    // Test des fonctions avec un pointeur NULL
    EXPECT_EQ(osSemDestroy(nullptr), OS_SEM_ERROR);
    EXPECT_EQ(osSemWait(nullptr), OS_SEM_ERROR);
    EXPECT_EQ(osSemPost(nullptr), OS_SEM_ERROR);
    EXPECT_EQ(osSemTryWait(nullptr), OS_SEM_ERROR);
    EXPECT_EQ(osSemWaitTimeout(nullptr, 100), OS_SEM_ERROR);
    
    int value;
    EXPECT_EQ(osSemGetValue(nullptr, &value), OS_SEM_ERROR);
    EXPECT_EQ(osSemGetValue(&sem, nullptr), OS_SEM_ERROR);
    
    EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
}

// Test de création et destruction multiple
TEST(SemaphoreTest, MultipleCreateDestroy) {
    t_OSSemCtx sem;
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(osSemInit(&sem, i, nullptr), OS_SEM_SUCCESS);
        
        int value;
        EXPECT_EQ(osSemGetValue(&sem, &value), OS_SEM_SUCCESS);
        EXPECT_EQ(value, i);
        
        EXPECT_EQ(osSemDestroy(&sem), OS_SEM_SUCCESS);
    }
} 