////////////////////////////////////////////////////////////
//  memory test file
//  implements test cases for memory management
//
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <vector>

extern "C" {
    #include "memory/xMemory.h"
}

class MemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        EXPECT_EQ(xMemInit(), XOS_MEM_OK);
    }

    void TearDown() override {
        EXPECT_EQ(xMemCleanup(), XOS_MEM_OK);
    }
};

// Test initialisation basique
TEST_F(MemoryTest, BasicInitialization) {
    size_t total, peak, count;
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, 0);
    EXPECT_EQ(peak, 0);
    EXPECT_EQ(count, 0);
}

// Test allocation simple
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

// Test intégrité mémoire
TEST_F(MemoryTest, MemoryIntegrity) {
    void* ptr = X_MALLOC(100);
    ASSERT_NE(ptr, nullptr);
    
    EXPECT_EQ(xMemCheck(), XOS_MEM_OK);
    
    // Simuler une corruption de mémoire
    uint32_t* canary = (uint32_t*)ptr - 1;
    *canary = 0;  // Corrompre le canary
    
    EXPECT_EQ(X_FREE(ptr), XOS_MEM_CORRUPTION);
}

// Test limites et erreurs
TEST_F(MemoryTest, BoundaryConditions) {
    // Test allocation zéro
    EXPECT_DEATH(X_MALLOC(0), ".*");
    
    // Test allocation excessive
    void* ptr = X_MALLOC(XOS_MEM_MAX_ALLOCATION + 1);
    EXPECT_EQ(ptr, nullptr);
    
    // Test double free
    ptr = X_MALLOC(100);
    EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
    EXPECT_EQ(X_FREE(ptr), XOS_MEM_INVALID);
}

// Test allocations multiples
TEST_F(MemoryTest, MultipleAllocations) {
    std::vector<void*> ptrs;
    const int NUM_ALLOCS = 1000;
    const size_t ALLOC_SIZE = 100;
    
    for(int i = 0; i < NUM_ALLOCS; i++) {
        void* ptr = X_MALLOC(ALLOC_SIZE);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    size_t total, peak, count;
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, NUM_ALLOCS * ALLOC_SIZE);
    EXPECT_EQ(count, NUM_ALLOCS);
    
    for(void* ptr : ptrs) {
        EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
    }
}

// Test accès concurrents
TEST_F(MemoryTest, ConcurrentAccess) {
    const int NUM_THREADS = 10;
    const int ALLOCS_PER_THREAD = 100;
    
    auto threadFunc = []() {
        std::vector<void*> ptrs;
        for(int i = 0; i < ALLOCS_PER_THREAD; i++) {
            void* ptr = X_MALLOC(100);
            EXPECT_NE(ptr, nullptr);
            ptrs.push_back(ptr);
        }
        
        for(void* ptr : ptrs) {
            EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
        }
    };
    
    std::vector<std::thread> threads;
    for(int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(threadFunc);
    }
    
    for(auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(xMemCheck(), XOS_MEM_OK);
}

// Test pic d'utilisation
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
    EXPECT_EQ(peak, 3000);  // Le pic doit rester à 3000
    
    EXPECT_EQ(X_FREE(ptr2), XOS_MEM_OK);
}

// Test nettoyage
TEST_F(MemoryTest, Cleanup) {
    void* ptr1 = X_MALLOC(100);
    void* ptr2 = X_MALLOC(200);
    
    EXPECT_EQ(xMemCleanup(), XOS_MEM_OK);
    
    // Vérifier que tout est réinitialisé
    size_t total, peak, count;
    EXPECT_EQ(xMemGetStats(&total, &peak, &count), XOS_MEM_OK);
    EXPECT_EQ(total, 0);
    EXPECT_EQ(peak, 0);
    EXPECT_EQ(count, 0);
}

// Test source tracking
TEST_F(MemoryTest, SourceTracking) {
    void* ptr = X_MALLOC(100);
    ASSERT_NE(ptr, nullptr);
    
    // Vérifier que les informations de source sont correctement stockées
    // Note: Cette vérification nécessiterait l'accès aux structures internes
    
    EXPECT_EQ(X_FREE(ptr), XOS_MEM_OK);
}
