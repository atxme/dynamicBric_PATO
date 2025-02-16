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
#define OS_TASK_EXIT_SUCCESS 0UL
#define OS_TASK_EXIT_FAILURE -1UL

// Définition de la politique d'ordonnancement
// Pour l'ordonnancement en temps réel (RT), les threads utilisent SCHED_RR tandis que 
// En revanche, pour l'ordonnancement normal (avec SCHED_OTHER), la priorité est fixe.
#ifdef OS_USE_RT_SCHEDULING
  // Pour Linux avec la politique temps réel SCHED_RR.
  // La plage de priorité est obtenue dynamiquement via sched_get_priority_min/max.
#define OS_TASK_LOWEST_PRIORITY sched_get_priority_min(SCHED_RR)
#define OS_TASK_HIGHEST_PRIORITY sched_get_priority_max(SCHED_RR)
#define OS_TASK_DEFAULT_PRIORITY ((OS_TASK_HIGHEST_PRIORITY + OS_TASK_LOWEST_PRIORITY) / 2)
#else
  // Pour l'ordonnancement normal (SCHED_OTHER), la priorité est généralement fixe (valeur 0).
#define OS_TASK_LOWEST_PRIORITY  20
#define OS_TASK_HIGHEST_PRIORITY 0
#define OS_TASK_DEFAULT_PRIORITY 10
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
} os_task_t;

//////////////////////////////////
// Déclarations des fonctions de gestion de tâches
//////////////////////////////////
int osTaskCreate(os_task_t* p_pttOSTask);
int osTaskEnd(os_task_t* p_pttOSTask);


int osTaskGetExitCode(os_task_t* p_pttOSTask);

#endif // OS_TASK_H_
