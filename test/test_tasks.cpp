////////////////////////////////////////////////////////////
//  os Task test file
//  implements test cases for task functions
//
//  Written : 14/11/2024
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

extern "C" {
    #include "xOs/xTask.h"
}

// Fonction helper pour une tâche simple
static void* TestTaskFunction(void* arg) {
    int* value = static_cast<int*>(arg);
    *value = 42;
    return nullptr;
}

TEST(TaskTest, BasicTaskCreation) {
    os_task_t task;
    memset(&task, 0, sizeof(os_task_t));
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;
    int testValue = 0;
    task.task = TestTaskFunction;
    task.arg = &testValue;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    EXPECT_NE(task.id, 0);

    if (task.status != OS_TASK_STATUS_TERMINATED) {
        osTaskEnd(&task);
    }

    pthread_join(task.handle, NULL);
}

TEST(TaskTest, InvalidPriority) {
    os_task_t task;
    memset(&task, 0, sizeof(os_task_t));
    task.task = TestTaskFunction;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;

    // Test priorité trop haute
    task.priority = OS_TASK_HIGHEST_PRIORITY + 1;
    EXPECT_DEATH(osTaskCreate(&task), ".*");

    // Test priorité négative
    task.priority = -1;
    EXPECT_DEATH(osTaskCreate(&task), ".*");
}

TEST(TaskTest, InvalidStackSize) {
    os_task_t task;
    memset(&task, 0, sizeof(os_task_t));
    task.task = TestTaskFunction;
    task.stack_size = 0;

    EXPECT_DEATH(osTaskCreate(&task), ".*");
}

TEST(TaskTest, UpdatePriority) {
    os_task_t task;
    memset(&task, 0, sizeof(os_task_t));
    task.task = TestTaskFunction;
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);

    int newPriority = OS_TASK_HIGHEST_PRIORITY;
    EXPECT_EQ(osTaskUpdatePriority(&task, &newPriority), OS_TASK_SUCCESS);
    EXPECT_EQ(task.priority, newPriority);

    osTaskEnd(&task);
}

/*
TEST(TaskTest, GetExitCode) {
    os_task_t task;
    memset(&task, 0, sizeof(os_task_t));
    task.task = TestTaskFunction;
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(osTaskGetExitCode(&task), OS_TASK_EXIT_SUCCESS);

    osTaskEnd(&task);
}
*/

#ifdef _WIN32
TEST(TaskTest, WindowsSpecificOperations) {
    os_task_t task;
    memset(&task, 0, sizeof(os_task_t));
    task.task = TestTaskFunction;
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskStart(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskSuspend(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskResume(&task), OS_TASK_SUCCESS);

    osTaskEnd(&task);
}
#endif

TEST(TaskTest, TaskTermination) {
    os_task_t task;
    memset(&task, 0, sizeof(os_task_t));
    task.task = TestTaskFunction;
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskEnd(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(task.status, OS_TASK_STATUS_TERMINATED);
}

TEST(TaskTest, NullParameters) {
    EXPECT_DEATH(osTaskCreate(nullptr), ".*");
    EXPECT_DEATH(osTaskEnd(nullptr), ".*");
    EXPECT_DEATH(osTaskUpdatePriority(nullptr, nullptr), ".*");
}

TEST(TaskTest, MultipleTaskCreation) {
    const int NUM_TASKS = 5;
    os_task_t tasks[NUM_TASKS];
    int values[NUM_TASKS] = {0};

    for (int i = 0; i < NUM_TASKS; i++) {
        memset(&tasks[i], 0, sizeof(os_task_t));
        tasks[i].task = TestTaskFunction;
        tasks[i].arg = &values[i];
        tasks[i].priority = OS_TASK_DEFAULT_PRIORITY;
        tasks[i].stack_size = OS_TASK_DEFAULT_STACK_SIZE;

        EXPECT_EQ(osTaskCreate(&tasks[i]), OS_TASK_SUCCESS);
    }

    for (int i = 0; i < NUM_TASKS; i++) {
        EXPECT_EQ(osTaskEnd(&tasks[i]), OS_TASK_SUCCESS);
    }
}

TEST(TaskTest, LongRunningTask) {
    static bool taskCompleted = false;

    auto longTask = [](void* arg) -> void* {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        taskCompleted = true;
        return nullptr;
    };

    os_task_t task;
    memset(&task, 0, sizeof(os_task_t));
    task.task = longTask;
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;

    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    EXPECT_TRUE(taskCompleted);

    osTaskEnd(&task);
}
