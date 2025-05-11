#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <cstring>

extern "C" {
#include "xTask.h"
}

// Fonction helper pour une tâche simple
static void* TestTaskFunction(void* arg) {
    if (arg == nullptr) return nullptr;
    int* value = static_cast<int*>(arg);
    *value = 42;
    return nullptr;
}

TEST(TaskTest, TaskInitialization) {
    t_TaskCtx task;
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(task.stack_size, OS_TASK_DEFAULT_STACK_SIZE);
    EXPECT_EQ(task.priority, OS_TASK_DEFAULT_PRIORITY);
    EXPECT_EQ(task.status, OS_TASK_STATUS_READY);
#ifdef OS_USE_RT_SCHEDULING
    EXPECT_EQ(task.policy, OS_DEFAULT_SCHED_POLICY);
#endif
}

TEST(TaskTest, BasicTaskCreation) {
    t_TaskCtx task;
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    
    int testValue = 0;
    task.task = TestTaskFunction;
    task.arg = &testValue;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    EXPECT_NE(task.id, 0);
    EXPECT_EQ(task.status, OS_TASK_STATUS_RUNNING);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(testValue, 42);

    EXPECT_EQ(osTaskEnd(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(task.status, OS_TASK_STATUS_TERMINATED);
}

TEST(TaskTest, InvalidPriority) {
    t_TaskCtx task;
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    task.task = TestTaskFunction;

    // Test de priorité trop haute (supérieure à OS_TASK_HIGHEST_PRIORITY) ou trop basse
#ifdef OS_USE_RT_SCHEDULING
    task.priority = OS_TASK_HIGHEST_PRIORITY + 1;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_ERROR);
    
    task.priority = OS_TASK_LOWEST_PRIORITY - 1;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_ERROR);
#else
    task.priority = OS_TASK_HIGHEST_PRIORITY - 1; 
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_ERROR);
    
    task.priority = OS_TASK_LOWEST_PRIORITY + 1;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_ERROR);
#endif
}

TEST(TaskTest, InvalidStackSize) {
    t_TaskCtx task;
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    task.task = TestTaskFunction;
    task.stack_size = 0;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_ERROR);
}

TEST(TaskTest, WaitForTaskCompletion) {
    t_TaskCtx task;
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    
    int testValue = 0;
    task.task = TestTaskFunction;
    task.arg = &testValue;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);

    // Attendre que la tâche se termine
    void* exitValue = nullptr;
    EXPECT_EQ(osTaskWait(&task, &exitValue), OS_TASK_SUCCESS);
    EXPECT_EQ(task.status, OS_TASK_STATUS_TERMINATED);
    EXPECT_EQ(testValue, 42);
}

TEST(TaskTest, GetTaskStatus) {
    t_TaskCtx task;
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    
    auto longTask = [](void* arg) -> void* {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        return nullptr;
    };
    
    task.task = longTask;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskGetStatus(&task), OS_TASK_STATUS_RUNNING);
    
    // Plutôt que d'attendre avec un délai fixe ou une attente active,
    // utilisons osTaskWait qui garantit que nous attendrons la fin de la tâche
    void* exitValue = nullptr;
    EXPECT_EQ(osTaskWait(&task, &exitValue), OS_TASK_SUCCESS);
    
    // À ce stade, la tâche doit être terminée
    EXPECT_EQ(task.status, OS_TASK_STATUS_TERMINATED);
    EXPECT_EQ(osTaskGetStatus(&task), OS_TASK_STATUS_TERMINATED);
}

TEST(TaskTest, TaskTermination) {
    t_TaskCtx task;
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    
    auto longTask = [](void* arg) -> void* {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return nullptr;
    };
    
    task.task = longTask;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    // Terminer la tâche avant qu'elle ne soit finie
    EXPECT_EQ(osTaskEnd(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(task.status, OS_TASK_STATUS_TERMINATED);
}

TEST(TaskTest, NullParameters) {
    EXPECT_EQ(osTaskInit(nullptr), OS_TASK_ERROR);
    EXPECT_EQ(osTaskCreate(nullptr), OS_TASK_ERROR);
    EXPECT_EQ(osTaskEnd(nullptr), OS_TASK_ERROR);
    EXPECT_EQ(osTaskGetStatus(nullptr), OS_TASK_ERROR);
    EXPECT_EQ(osTaskWait(nullptr, nullptr), OS_TASK_ERROR);
}

TEST(TaskTest, MultipleTaskCreation) 
{
    const int NUM_TASKS = 5;
    t_TaskCtx tasks[NUM_TASKS];
    int values[NUM_TASKS] = { 0 };

    for (int i = 0; i < NUM_TASKS; i++) {
        EXPECT_EQ(osTaskInit(&tasks[i]), OS_TASK_SUCCESS);
        tasks[i].task = TestTaskFunction;
        tasks[i].arg = &values[i];
        EXPECT_EQ(osTaskCreate(&tasks[i]), OS_TASK_SUCCESS);
    }
    
    // Attendre que toutes les tâches se terminent
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    for (int i = 0; i < NUM_TASKS; i++) {
        EXPECT_EQ(values[i], 42);
        EXPECT_EQ(osTaskEnd(&tasks[i]), OS_TASK_SUCCESS);
    }
}

#ifdef OS_USE_RT_SCHEDULING
TEST(TaskTest, SchedulingPolicyConfig) {
    t_TaskCtx task;
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    
    task.task = TestTaskFunction;
    task.policy = OS_SCHED_FIFO;
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskEnd(&task), OS_TASK_SUCCESS);
    
    // Test with OS_SCHED_RR
    EXPECT_EQ(osTaskInit(&task), OS_TASK_SUCCESS);
    task.task = TestTaskFunction;
    task.policy = OS_SCHED_RR;
    
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskEnd(&task), OS_TASK_SUCCESS);
}
#endif
