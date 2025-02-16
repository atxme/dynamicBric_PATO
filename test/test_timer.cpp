////////////////////////////////////////////////////////////
//  timer test file
//  implements test cases for timer functions
//
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

extern "C" {
    #include "timer/xTimer.h"
}

class TimerTest : public ::testing::Test {
protected:
    xos_timer_t timer;

    void SetUp() override {
        memset(&timer, 0, sizeof(xos_timer_t));
    }

    void TearDown() override {
        if (timer.t_ucActive) {
            xTimerStop(&timer);
        }
    }
};

// Test création basique
TEST_F(TimerTest, BasicCreation) {
    EXPECT_EQ(xTimerCreate(&timer, 1000, XOS_TIMER_MODE_ONESHOT), XOS_TIMER_OK);
    EXPECT_EQ(timer.t_ulPeriod, 1000);
    EXPECT_EQ(timer.t_ucMode, XOS_TIMER_MODE_ONESHOT);
    EXPECT_EQ(timer.t_ucActive, 0);
}

// Test paramètres invalides
TEST_F(TimerTest, InvalidParameters) {
    EXPECT_DEATH(xTimerCreate(nullptr, 1000, XOS_TIMER_MODE_ONESHOT), ".*");
    EXPECT_DEATH(xTimerCreate(&timer, 0, XOS_TIMER_MODE_ONESHOT), ".*");
    EXPECT_DEATH(xTimerCreate(&timer, 1000, 2), ".*");
}

// Test timer one-shot
TEST_F(TimerTest, OneShot) {
    EXPECT_EQ(xTimerCreate(&timer, 100, XOS_TIMER_MODE_ONESHOT), XOS_TIMER_OK);
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);
    
    // Timer ne devrait pas avoir expiré immédiatement
    EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_TIMEOUT);
    
    // Attendre que le timer expire
    xTimerDelay(150);
    EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_OK);
    
    // Le timer one-shot devrait être inactif après expiration
    EXPECT_EQ(timer.t_ucActive, 0);
}

// Test timer périodique
TEST_F(TimerTest, Periodic) {
    EXPECT_EQ(xTimerCreate(&timer, 100, XOS_TIMER_MODE_PERIODIC), XOS_TIMER_OK);
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);
    
    // Vérifier plusieurs périodes
    for(int i = 0; i < 3; i++) {
        EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_TIMEOUT);
        xTimerDelay(100);
        EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_OK);
        EXPECT_EQ(timer.t_ucActive, 1);  // Timer périodique reste actif
    }
}

// Test précision du timer
TEST_F(TimerTest, TimerPrecision) {
    const uint32_t PERIOD = 100;  // 100ms
    EXPECT_EQ(xTimerCreate(&timer, PERIOD, XOS_TIMER_MODE_ONESHOT), XOS_TIMER_OK);
    
    uint32_t start = xTimerGetCurrentMs();
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);
    
    while(xTimerExpired(&timer) == XOS_TIMER_TIMEOUT) {
        xTimerDelay(1);
    }
    
    uint32_t elapsed = xTimerGetCurrentMs() - start;
    EXPECT_GE(elapsed, PERIOD);
    EXPECT_LE(elapsed, PERIOD + 50);  // Permettre une marge d'erreur de 50ms
}

// Test arrêt du timer
TEST_F(TimerTest, TimerStop) {
    EXPECT_EQ(xTimerCreate(&timer, 1000, XOS_TIMER_MODE_PERIODIC), XOS_TIMER_OK);
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);
    EXPECT_EQ(timer.t_ucActive, 1);
    
    EXPECT_EQ(xTimerStop(&timer), XOS_TIMER_OK);
    EXPECT_EQ(timer.t_ucActive, 0);
    EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_NOT_INIT);
}

// Test redémarrage du timer
TEST_F(TimerTest, TimerRestart) {
    EXPECT_EQ(xTimerCreate(&timer, 100, XOS_TIMER_MODE_PERIODIC), XOS_TIMER_OK);
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);
    
    xTimerDelay(50);
    EXPECT_EQ(xTimerStop(&timer), XOS_TIMER_OK);
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);
    
    // Le timer devrait repartir de zéro
    EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_TIMEOUT);
}

// Test GetCurrentMs
TEST_F(TimerTest, GetCurrentMs) {
    uint32_t start = xTimerGetCurrentMs();
    xTimerDelay(100);
    uint32_t elapsed = xTimerGetCurrentMs() - start;
    
    EXPECT_GE(elapsed, 100);
    EXPECT_LE(elapsed, 150);  // Permettre une marge d'erreur
}

// Test de charge
TEST_F(TimerTest, StressTest) {
    const int NUM_TIMERS = 10;
    xos_timer_t timers[NUM_TIMERS];
    
    for(int i = 0; i < NUM_TIMERS; i++) {
        EXPECT_EQ(xTimerCreate(&timers[i], 100 + i*10, XOS_TIMER_MODE_PERIODIC), XOS_TIMER_OK);
        EXPECT_EQ(xTimerStart(&timers[i]), XOS_TIMER_OK);
    }
    
    // Vérifier tous les timers pendant quelques cycles
    for(int cycle = 0; cycle < 3; cycle++) {
        xTimerDelay(150);
        for(int i = 0; i < NUM_TIMERS; i++) {
            EXPECT_EQ(xTimerExpired(&timers[i]), XOS_TIMER_OK);
        }
    }
    
    // Nettoyer
    for(int i = 0; i < NUM_TIMERS; i++) {
        EXPECT_EQ(xTimerStop(&timers[i]), XOS_TIMER_OK);
    }
}
