////////////////////////////////////////////////////////////
//  os Task test file
//  implements test cases for task functions
//
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

extern "C" {
    #include "xOs/xTask.h"
}

class TaskTest : public ::testing::Test {
protected:
    os_task_t task;


void SetUp() override {
    memset(&task, 0, sizeof(os_task_t));
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;
    task.task = nullptr;
    task.arg = nullptr;
    task.handle = 0;
}


    void TearDown() override {
        if (task.status != OS_TASK_STATUS_TERMINATED) {
            osTaskEnd(&task);
        }
    }

    // Fonction helper pour une tâche simple
    static void* TestTaskFunction(void* arg) {
        int* value = static_cast<int*>(arg);
        *value = 42;
        return nullptr;
    }
};

// Test de création de tâche basique
TEST_F(TaskTest, BasicTaskCreation) {
    task.task = TestTaskFunction;  // Définir une fonction de test valide
    task.priority = OS_TASK_DEFAULT_PRIORITY;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;
    int testValue = 0;
    task.arg = &testValue;
    
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    EXPECT_NE(task.id, 0);
    
    // Attendre que la tâche se termine
    if (task.status != OS_TASK_STATUS_TERMINATED) {
        osTaskEnd(&task);
    }

    // destruction du thread
    pthread_join(task.handle, NULL);
}

/*

// Test avec priorités invalides
TEST_F(TaskTest, InvalidPriority) {
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
*/

TEST_F(TaskTest, InvalidPriority) {
    memset(&task, 0, sizeof(os_task_t));
    task.task = TestTaskFunction;
    task.stack_size = OS_TASK_DEFAULT_STACK_SIZE;
    
    // Test priorité trop haute
    task.priority = OS_TASK_HIGHEST_PRIORITY + 1;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_ERROR);
    
    // Test priorité négative
    task.priority = -1;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_ERROR);
}


// Test avec taille de pile invalide
TEST_F(TaskTest, InvalidStackSize) {
    task.task = TestTaskFunction;
    task.stack_size = 0;
    
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_ERROR);
}

// Test de mise à jour de priorité
TEST_F(TaskTest, UpdatePriority) {
    task.task = TestTaskFunction;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    
    int newPriority = OS_TASK_HIGHEST_PRIORITY;
    EXPECT_EQ(osTaskUpdatePriority(&task, &newPriority), OS_TASK_SUCCESS);
    EXPECT_EQ(task.priority, newPriority);
}

// Test de récupération du code de sortie
TEST_F(TaskTest, GetExitCode) {
    task.task = TestTaskFunction;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    
    // Attendre que la tâche se termine
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(osTaskGetExitCode(&task), OS_TASK_EXIT_SUCCESS);
}

#ifdef _WIN32
// Tests spécifiques à Windows
TEST_F(TaskTest, WindowsSpecificOperations) {
    task.task = TestTaskFunction;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    
    EXPECT_EQ(osTaskStart(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskSuspend(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(osTaskResume(&task), OS_TASK_SUCCESS);
}
#endif

// Test de terminaison de tâche
TEST_F(TaskTest, TaskTermination) {
    task.task = TestTaskFunction;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    
    EXPECT_EQ(osTaskEnd(&task), OS_TASK_SUCCESS);
    EXPECT_EQ(task.status, OS_TASK_STATUS_TERMINATED);
}

// Test avec paramètres NULL
TEST_F(TaskTest, NullParameters) {
    EXPECT_EQ(osTaskCreate(nullptr), OS_TASK_ERROR);
    EXPECT_EQ(osTaskEnd(nullptr), OS_TASK_ERROR);
    EXPECT_EQ(osTaskUpdatePriority(nullptr, nullptr), OS_TASK_ERROR);
}

// Test de création multiple de tâches
TEST_F(TaskTest, MultipleTaskCreation) {
    const int NUM_TASKS = 5;
    os_task_t tasks[NUM_TASKS];
    int values[NUM_TASKS] = {0};
    
    for (int i = 0; i < NUM_TASKS; i++) {
        tasks[i].task = TestTaskFunction;
        tasks[i].arg = &values[i];
        tasks[i].priority = OS_TASK_DEFAULT_PRIORITY;
        tasks[i].stack_size = OS_TASK_DEFAULT_STACK_SIZE;
        
        EXPECT_EQ(osTaskCreate(&tasks[i]), OS_TASK_SUCCESS);
    }
    
    // Nettoyage
    for (int i = 0; i < NUM_TASKS; i++) {
        EXPECT_EQ(osTaskEnd(&tasks[i]), OS_TASK_SUCCESS);
    }
}

// Test de longue durée
TEST_F(TaskTest, LongRunningTask) {
    static bool taskCompleted = false;
    
    auto longTask = [](void* arg) -> void* {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        taskCompleted = true;
        return nullptr;
    };
    
    task.task = longTask;
    EXPECT_EQ(osTaskCreate(&task), OS_TASK_SUCCESS);
    
    // Attendre la fin de la tâche
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    EXPECT_TRUE(taskCompleted);
}
