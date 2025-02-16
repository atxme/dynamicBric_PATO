////////////////////////////////////////////////////////////
//  xMemory_test.cpp
//  Implémente les cas de test pour la gestion de mémoire avec corrections
//
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <vector>

extern "C" {
#include "memory/xMemory.h"
}

// Assurez-vous que les macros X_MALLOC et X_FREE soient définies dans xMemory.h
// par exemple :
// #define X_MALLOC(sz)    xMemAlloc(sz, __FILE__, __LINE__)
// #define X_FREE(ptr)     xMemFree(ptr)

class MemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        EXPECT_EQ(xMemInit(), XOS_MEM_OK);
    }
    void TearDown() override {
        EXPECT_EQ(xMemCleanup(), XOS_MEM_OK);
    }
};

// Test d'initialisation basique
TEST_F(MemoryTest, BasicInitialization) {
    size_t total, peak, count;
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, 0);
    EXPECT_EQ(peak, 0);
    EXPECT_EQ(count, 0);
}

// Test d'allocation élémentaire
TEST_F(MemoryTest, BasicAllocation) {
    void* ptr = X_MALLOC(100);
    ASSERT_NE(ptr, nullptr);

    size_t total, peak, count;
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, 100);
    EXPECT_EQ(peak, 100);
    EXPECT_EQ(count, 1);

    EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
}

// Test d'intégrité mémoire via la fonction dédiée xMemCorrupt (mode DEBUG)
TEST_F(MemoryTest, MemoryIntegrity) {
    void* ptr = X_MALLOC(100);
    ASSERT_NE(ptr, nullptr);

    EXPECT_EQ(xMemCheck(), XOS_MEM_OK);

#ifdef DEBUG
    // Simuler la corruption via la fonction xMemCorrupt()
    xMemCorrupt();
#else
    GTEST_SKIP() << "Test contourné car le mode DEBUG n'est pas activé.";
#endif

    EXPECT_EQ(X_FREE(ptr), XOS_MEM_CORRUPTION);
}

// Test des conditions limites et gestion d’erreurs
TEST_F(MemoryTest, BoundaryConditions) {
    // Allocation de taille zéro doit échouer
    EXPECT_DEATH(X_MALLOC(0), ".*");

    // Allocation excessive
    void* ptr = X_MALLOC(XOS_MEM_MAX_ALLOCATION + 1);
    EXPECT_EQ(ptr, nullptr);

    // Test du double free
    ptr = X_MALLOC(100);
    EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
    EXPECT_EQ(X_FREE(ptr), XOS_MEM_INVALID);
}

// Test d'allocations multiples
TEST_F(MemoryTest, MultipleAllocations) {
    std::vector<void*> ptrs;
    const int NUM_ALLOCS = 1000;
    const size_t ALLOC_SIZE = 100;

    for (int i = 0; i < NUM_ALLOCS; i++) {
        void* ptr = X_MALLOC(ALLOC_SIZE);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }

    size_t total, peak, count;
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, NUM_ALLOCS * ALLOC_SIZE);
    EXPECT_EQ(count, NUM_ALLOCS);

    for (void* ptr : ptrs) {
        EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
    }
}

// Test d'accès concurrent
TEST_F(MemoryTest, ConcurrentAccess) {
    const int NUM_THREADS = 10;
    const int ALLOCS_PER_THREAD = 100;

    auto threadFunc = []() {
        std::vector<void*> ptrs;
        for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
            void* ptr = X_MALLOC(100);
            EXPECT_NE(ptr, nullptr);
            ptrs.push_back(ptr);
        }
        for (void* ptr : ptrs) {
            EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
        }
        };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(threadFunc);
    }

    for (auto& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(xMemCheck(), XOS_MEM_OK);
}

// Test du pic d'utilisation (peak usage)
TEST_F(MemoryTest, PeakUsage) {
    void* ptr1 = X_MALLOC(1000);
    void* ptr2 = X_MALLOC(2000);

    size_t total, peak, count;
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, 3000);
    EXPECT_EQ(peak, 3000);

    EXPECT_EQ(X_FREE(ptr1), XOS_MEM_OK);
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, 2000);
    EXPECT_EQ(peak, 3000);  // Le pic reste à 3000

    EXPECT_EQ(X_FREE(ptr2), XOS_MEM_OK);
}

// Test de nettoyage complet
TEST_F(MemoryTest, Cleanup) {
    void* ptr1 = X_MALLOC(100);
    void* ptr2 = X_MALLOC(200);

    EXPECT_EQ(xMemCleanup(), XOS_MEM_OK);

    // Vérifier que les statistiques sont réinitialisées
    size_t total, peak, count;
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, 0);
    EXPECT_EQ(peak, 0);
    EXPECT_EQ(count, 0);
}

// Test du suivi de la source de l'allocation
TEST_F(MemoryTest, SourceTracking) {
    void* ptr = X_MALLOC(100);
    ASSERT_NE(ptr, nullptr);

    // On ne fait ici qu'une allocation de contrôle, l'accès
    // direct aux informations source se ferait via des fonctions d'inspection spécifiques.
    EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
}
