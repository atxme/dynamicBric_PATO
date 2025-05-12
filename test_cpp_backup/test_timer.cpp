////////////////////////////////////////////////////////////
//  timer test file
//  implements test cases for timer functions
//
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

extern "C"
{
#include "timer/xTimer.h"
}

class TimerTest : public ::testing::Test
{
protected:
    xOsTimerCtx timer;

    void SetUp() override
    {
        memset(&timer, 0, sizeof(xOsTimerCtx));
    }

    void TearDown() override
    {
        if (timer.t_ucActive)
        {
            xTimerStop(&timer);
        }
    }
};

// Test création basique
TEST_F(TimerTest, BasicCreation)
{
    EXPECT_EQ(xTimerCreate(&timer, 1000 * 1000, XOS_TIMER_MODE_ONESHOT), XOS_TIMER_OK);
    EXPECT_EQ(timer.t_ulPeriod, 1000 * 1000); // 1000ms = 1000000µs
    EXPECT_EQ(timer.t_ucMode, XOS_TIMER_MODE_ONESHOT);
    EXPECT_EQ(timer.t_ucActive, 0);
}

// Test paramètres invalides
TEST_F(TimerTest, InvalidParameters)
{
    EXPECT_DEATH(xTimerCreate(nullptr, 1000 * 1000, XOS_TIMER_MODE_ONESHOT), ".*");
    EXPECT_DEATH(xTimerCreate(&timer, 0, XOS_TIMER_MODE_ONESHOT), ".*");
    EXPECT_DEATH(xTimerCreate(&timer, 1000 * 1000, 2), ".*");
}

// Test timer one-shot
TEST_F(TimerTest, OneShot)
{
    EXPECT_EQ(xTimerCreate(&timer, 100 * 1000, XOS_TIMER_MODE_ONESHOT), XOS_TIMER_OK); // 100ms = 100000µs
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
TEST_F(TimerTest, Periodic)
{
    EXPECT_EQ(xTimerCreate(&timer, 100 * 1000, XOS_TIMER_MODE_PERIODIC), XOS_TIMER_OK); // 100ms = 100000µs
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);

    // Vérifier plusieurs périodes
    for (int i = 0; i < 3; i++)
    {
        EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_TIMEOUT);
        xTimerDelay(100);
        EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_OK);
        EXPECT_EQ(timer.t_ucActive, 1); // Timer périodique reste actif
    }
}

// Test précision du timer
TEST_F(TimerTest, TimerPrecision)
{
    const uint32_t PERIOD_MS = 100;              // 100ms
    const uint32_t PERIOD_US = PERIOD_MS * 1000; // Conversion en microsecondes

    EXPECT_EQ(xTimerCreate(&timer, PERIOD_US, XOS_TIMER_MODE_ONESHOT), XOS_TIMER_OK);

    uint32_t start = xTimerGetCurrentMs();
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);

    while (xTimerExpired(&timer) == XOS_TIMER_TIMEOUT)
    {
        xTimerDelay(1);
    }

    uint32_t elapsed = xTimerGetCurrentMs() - start;
    EXPECT_GE(elapsed, PERIOD_MS);
    EXPECT_LE(elapsed, PERIOD_MS + 50); // Permettre une marge d'erreur de 50ms
}

// Test arrêt du timer
TEST_F(TimerTest, TimerStop)
{
    EXPECT_EQ(xTimerCreate(&timer, 1000 * 1000, XOS_TIMER_MODE_PERIODIC), XOS_TIMER_OK); // 1000ms = 1000000µs
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);
    EXPECT_EQ(timer.t_ucActive, 1);

    EXPECT_EQ(xTimerStop(&timer), XOS_TIMER_OK);
    EXPECT_EQ(timer.t_ucActive, 0);
    EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_NOT_INIT);
}

// Test redémarrage du timer
TEST_F(TimerTest, TimerRestart)
{
    EXPECT_EQ(xTimerCreate(&timer, 100 * 1000, XOS_TIMER_MODE_PERIODIC), XOS_TIMER_OK); // 100ms = 100000µs
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);

    xTimerDelay(50);
    EXPECT_EQ(xTimerStop(&timer), XOS_TIMER_OK);
    EXPECT_EQ(xTimerStart(&timer), XOS_TIMER_OK);

    // Le timer devrait repartir de zéro
    EXPECT_EQ(xTimerExpired(&timer), XOS_TIMER_TIMEOUT);
}

// Test GetCurrentMs
TEST_F(TimerTest, GetCurrentMs)
{
    uint32_t start = xTimerGetCurrentMs();
    xTimerDelay(100);
    uint32_t elapsed = xTimerGetCurrentMs() - start;

    EXPECT_GE(elapsed, 100);
    EXPECT_LE(elapsed, 150); // Permettre une marge d'erreur
}

// Test de charge
TEST_F(TimerTest, StressTest)
{
    const int NUM_TIMERS = 10;
    xOsTimerCtx timers[NUM_TIMERS];

    // Déterminer la période maximale
    uint32_t max_period_ms = 0;

    for (int i = 0; i < NUM_TIMERS; i++)
    {
        uint32_t period_ms = 100 + i * 10;
        if (period_ms > max_period_ms)
        {
            max_period_ms = period_ms;
        }
        // (100 + i*10)ms = (100 + i*10)*1000µs
        EXPECT_EQ(xTimerCreate(&timers[i], period_ms * 1000, XOS_TIMER_MODE_PERIODIC), XOS_TIMER_OK);
        EXPECT_EQ(xTimerStart(&timers[i]), XOS_TIMER_OK);
    }

    // Ajouter une marge de sécurité de 20ms
    uint32_t wait_time = max_period_ms + 20;

    // Vérifier tous les timers pendant quelques cycles
    for (int cycle = 0; cycle < 3; cycle++)
    {
        xTimerDelay(wait_time);
        for (int i = 0; i < NUM_TIMERS; i++)
        {
            EXPECT_EQ(xTimerExpired(&timers[i]), XOS_TIMER_OK);
        }
    }

    // Nettoyer
    for (int i = 0; i < NUM_TIMERS; i++)
    {
        EXPECT_EQ(xTimerStop(&timers[i]), XOS_TIMER_OK);
    }
}
