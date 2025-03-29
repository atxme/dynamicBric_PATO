////////////////////////////////////////////////////////////
// os Task header file
// defines the os function for task manipulation
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#pragma once
#ifndef OS_TASK_H_
#define OS_TASK_H_

#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "xAssert.h"  // Macro d'assertion personnalisée (si utilisée)

// Codes de retour
#define OS_TASK_SUCCESS         0
#define OS_TASK_ERROR          -1

// États de la tâche
#define OS_TASK_STATUS_READY       0UL
#define OS_TASK_STATUS_RUNNING     1UL
#define OS_TASK_STATUS_BLOCKED     2UL
#define OS_TASK_STATUS_SUSPENDED   3UL
#define OS_TASK_STATUS_TERMINATED  4UL

// Codes de sortie de tâche
#define OS_TASK_EXIT_SUCCESS 0
#define OS_TASK_EXIT_FAILURE -1

typedef enum {
    OS_SCHED_NORMAL = 0,  // SCHED_OTHER - Ordonnancement standard
    OS_SCHED_FIFO,        // SCHED_FIFO - Temps réel FIFO
    OS_SCHED_RR,          // SCHED_RR - Temps réel Round Robin
    OS_SCHED_BATCH,       // SCHED_BATCH - Traitement par lots
    OS_SCHED_IDLE         // SCHED_IDLE - Priorité très basse
} t_SchedPolicy;

#ifdef OS_USE_RT_SCHEDULING
#define OS_DEFAULT_SCHED_POLICY OS_SCHED_FIFO
// Pour Linux avec la politique temps réel
#define OS_TASK_LOWEST_PRIORITY  1  // Minimum pour SCHED_FIFO/RR
#define OS_TASK_HIGHEST_PRIORITY 99 // Maximum pour SCHED_FIFO/RR
#define OS_TASK_DEFAULT_PRIORITY 50 // Valeur médiane
#else
#define OS_DEFAULT_SCHED_POLICY OS_SCHED_NORMAL
// Pour l'ordonnancement normal (nice values)
#define OS_TASK_LOWEST_PRIORITY  19  // Priorité la plus basse (nice +19)
#define OS_TASK_HIGHEST_PRIORITY -20 // Priorité la plus haute (nice -20)
#define OS_TASK_DEFAULT_PRIORITY 0   // Priorité normale (nice 0)
#endif

// Taille par défaut de la pile : ici 8 MB
#define OS_TASK_DEFAULT_STACK_SIZE (8 * 1024 * 1024)

//////////////////////////////////
// Structure de gestion de la tâche
//////////////////////////////////
typedef struct {
    void* (*task)(void*); // Pointeur vers la fonction de la tâche
    void* arg;            // Arguments de la tâche
    int priority;         // Priorité de la tâche (pour RT, définie via SCHED_RR; pour normal, cette valeur est fixe)
    size_t stack_size;    // Taille de la pile
    int id;               // ID de la tâche (attribut interne à ne pas modifier)
    int status;           // Statut de la tâche
    int exit_code;        // Code de sortie de la tâche
    pthread_t handle;     // Handle du thread pthread
#ifdef OS_USE_RT_SCHEDULING
	struct sched_param sched_param; // Paramètres d'ordonnancement
    t_SchedPolicy policy; // Politique d'ordonnancement
#endif
} t_TaskCtx;

//////////////////////////////////
/// @brief Initialize a task context with default values
/// @param p_pttOSTask : pointer to the task structure context to initialize
/// @return OS_TASK_SUCCESS if success, OS_TASK_ERROR otherwise
///
/// @note This function sets default values for the task context:
///       - Default stack size
///       - Default priority
///       - Default scheduling policy
//////////////////////////////////
int osTaskInit(t_TaskCtx* p_pttOSTask);

//////////////////////////////////
/// @brief Create a task
/// @param p_pttOSTask : pointer to the task structure context
/// @return OS_TASK_SUCCESS if success, OS_TASK_ERROR otherwise
/// 
/// @note create a task with the given parameters
/// @note the task is created with the default stack size
/// @pre configuration of the context structure
//////////////////////////////////
int osTaskCreate(t_TaskCtx* p_pttOSTask);

//////////////////////////////////
/// @brief End a task
/// @param p_pttOSTask : pointer to the task structure context
/// @return OS_TASK_SUCCESS if success, OS_TASK_ERROR otherwise
//////////////////////////////////
int osTaskEnd(t_TaskCtx* p_pttOSTask);

//////////////////////////////////
/// @brief Get the status of a task
/// @param p_pttOSTask : pointer to the task structure context
/// @return Current task status or OS_TASK_ERROR on error
//////////////////////////////////
int osTaskGetStatus(t_TaskCtx* p_pttOSTask);

//////////////////////////////////
/// @brief Wait for a task to complete
/// @param p_pttOSTask : pointer to the task structure context
/// @param p_pvExitValue : pointer to store the exit value (can be NULL)
/// @return OS_TASK_SUCCESS if success, OS_TASK_ERROR otherwise
//////////////////////////////////
int osTaskWait(t_TaskCtx* p_pttOSTask, void** p_pvExitValue);

#endif // OS_TASK_H_
