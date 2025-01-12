////////////////////////////////////////////////////////////
//  memory header file
//  defines the memory management types and functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_MEMORY_H_
#define XOS_MEMORY_H_

#include <stddef.h>
#include <stdint.h>

// Memory error codes
#define XOS_MEM_OK            0
#define XOS_MEM_ERROR        -1
#define XOS_MEM_INVALID      -2
#define XOS_MEM_OVERFLOW     -3
#define XOS_MEM_UNDERFLOW    -4
#define XOS_MEM_CORRUPTION   -5
#define XOS_MEM_ALREADY_INIT -6

// Memory canary values
#define XOS_MEM_CANARY_PREFIX 0xDEADBEEFUL
#define XOS_MEM_CANARY_SUFFIX 0xBEEFDEADUL

// Memory block structure
typedef struct xMemoryBlock
{
    unsigned long t_ulCanaryPrefix;  // Canary prefix for underflow detection
    void* t_ptAddress;              // Allocated address
    size_t t_ulSize;                // Block size
    const char* t_ptkcFile;         // Source file
    int t_iLine;                    // Line number
    unsigned char t_ucMetaHash[32]; // SHA256 hash of metadata for integrity
    unsigned long t_ulCanarySuffix; // Canary suffix for overflow detection
    struct xMemoryBlock* t_ptNext;  // Next block in list
} xMemoryBlock_t;

// Memory manager structure
typedef struct
{
    xMemoryBlock_t* t_ptBlocks;     // List of allocated blocks
    size_t t_ulTotalAllocated;      // Total allocated memory
    size_t t_ulPeakUsage;           // Peak memory usage
    size_t t_ulAllocCount;          // Number of allocations
    size_t t_ulFreeCount;           // Number of frees
} xMemoryManager_t;

//////////////////////////////////
/// @brief Initialize memory manager
/// @return success or error code
//////////////////////////////////
int xMemInit(void);

//////////////////////////////////
/// @brief Allocate memory block
/// @param p_ulSize : block size
/// @param p_ptkcFile : source file
/// @param p_iLine : line number
/// @return allocated memory pointer or NULL
//////////////////////////////////
void* xMemAlloc(size_t p_ulSize, const char* p_ptkcFile, int p_iLine);

//////////////////////////////////
/// @brief Allocate and zero memory block
/// @param p_ulCount : number of elements
/// @param p_ulSize : element size
/// @param p_ptkcFile : source file
/// @param p_iLine : line number
/// @return allocated memory pointer or NULL
//////////////////////////////////
void* xMemCalloc(size_t p_ulCount, size_t p_ulSize, const char* p_ptkcFile, int p_iLine);

//////////////////////////////////
/// @brief Reallocate memory block
/// @param p_ptPtr : current memory pointer
/// @param p_ulSize : new size
/// @param p_ptkcFile : source file
/// @param p_iLine : line number
/// @return reallocated memory pointer or NULL
//////////////////////////////////
void* xMemRealloc(void* p_ptPtr, size_t p_ulSize, const char* p_ptkcFile, int p_iLine);

//////////////////////////////////
/// @brief Free memory block
/// @param p_ptPtr : memory pointer
/// @return success or error code
//////////////////////////////////
int xMemFree(void* p_ptPtr);

//////////////////////////////////
/// @brief Get memory statistics
/// @param p_pulTotal : total allocated memory
/// @param p_pulPeak : peak memory usage
/// @param p_pulCount : number of allocations
/// @return success or error code
//////////////////////////////////
int xMemGetStats(size_t* p_pulTotal, size_t* p_pulPeak, size_t* p_pulCount);

//////////////////////////////////
/// @brief Check memory integrity
/// @return success or error code
//////////////////////////////////
int xMemCheck(void);

//////////////////////////////////
/// @brief Clean all allocated memory
/// @return success or error code
//////////////////////////////////
int xMemCleanup(void);

// Macro helpers for source tracking
#define X_MALLOC(size)           xMemAlloc(size, __FILE__, __LINE__)
#define X_CALLOC(count, size)    xMemCalloc(count, size, __FILE__, __LINE__)
#define X_REALLOC(ptr, size)     xMemRealloc(ptr, size, __FILE__, __LINE__)
#define X_FREE(ptr)              xMemFree(ptr)

#endif // XOS_MEMORY_H_
