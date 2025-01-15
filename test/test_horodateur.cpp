////////////////////////////////////////////////////////////
//  horodateur test file
//  implements test cases for timestamp functions
//
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <chrono>
#include <regex>
#include <thread>

extern "C" {
    #include "xOs/xOsHorodateur.h"
}

class HorodateurTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialisation si nécessaire
    }

    void TearDown() override {
        // Nettoyage si nécessaire
    }

    // Helper pour valider le format de timestamp
    bool isValidTimestampFormat(const char* timestamp) {
        std::regex pattern("\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}");
        return std::regex_match(timestamp, pattern);
    }
};

// Test de base pour xHorodateurGet
TEST_F(HorodateurTest, BasicTimestampGet) {
    uint32_t timestamp1 = xHorodateurGet();
    EXPECT_GT(timestamp1, 0);
    
    // Vérifier que le timestamp avance
    std::this_thread::sleep_for(std::chrono::seconds(1));
    uint32_t timestamp2 = xHorodateurGet();
    EXPECT_GT(timestamp2, timestamp1);
}

// Test de la fonction de formatage string
TEST_F(HorodateurTest, TimestampString) {
    const char* timestamp = xHorodateurGetString();
    ASSERT_NE(timestamp, nullptr);
    EXPECT_TRUE(isValidTimestampFormat(timestamp));
}

// Test de cohérence entre les deux fonctions
TEST_F(HorodateurTest, ConsistencyBetweenFunctions) {
    uint32_t timestamp = xHorodateurGet();
    const char* timeStr = xHorodateurGetString();
    
    // Convertir le string en time_t pour comparaison
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(timeStr, "%Y-%m-%d %H:%M:%S", &tm);
    time_t strTime = mktime(&tm);
    
    // Les timestamps doivent être proches (±1 seconde de différence max)
    EXPECT_NEAR(timestamp, static_cast<uint32_t>(strTime), 1);
}

// Test de multiples appels rapides
TEST_F(HorodateurTest, MultipleRapidCalls) {
    const int NUM_CALLS = 1000;
    for(int i = 0; i < NUM_CALLS; i++) {
        const char* timeStr = xHorodateurGetString();
        ASSERT_NE(timeStr, nullptr);
        EXPECT_TRUE(isValidTimestampFormat(timeStr));
    }
}

// Test de la progression du temps
TEST_F(HorodateurTest, TimeProgression) {
    uint32_t start = xHorodateurGet();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    uint32_t end = xHorodateurGet();
    
    EXPECT_GE(end - start, 2);
    EXPECT_LE(end - start, 3);  // Permettre une petite marge d'erreur
}

// Test du buffer statique
TEST_F(HorodateurTest, StaticBufferConsistency) {
    const char* str1 = xHorodateurGetString();
    const char* str2 = xHorodateurGetString();
    
    // Les pointeurs doivent être identiques (même buffer statique)
    EXPECT_EQ(str1, str2);
}

// Test des appels concurrents
TEST_F(HorodateurTest, ConcurrentAccess) {
    std::vector<std::thread> threads;
    const int NUM_THREADS = 10;
    
    for(int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([]{
            for(int j = 0; j < 100; j++) {
                EXPECT_NE(xHorodateurGetString(), nullptr);
                EXPECT_GT(xHorodateurGet(), 0);
            }
        });
    }
    
    for(auto& thread : threads) {
        thread.join();
    }
}
