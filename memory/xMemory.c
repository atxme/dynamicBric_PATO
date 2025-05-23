////////////////////////////////////////////////////////////
//  xMemory.c
//  Implements memory management functions
//
// Notes: 
// - Added mutex protection (pthread) for multithreaded mode
// - Explicit hash calculation to avoid padding issues
// - xMemCorrupt() function in DEBUG mode to simulate block corruption
//
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include "xMemory.h"
#include "xAssert.h"
#include "hash/xHash.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>  // Added for synchronization

// Global memory manager
static xMemoryManager_t s_tMemoryManager = { 0 };

// Mutex to protect s_tMemoryManager
static pthread_mutex_t s_memory_mutex = PTHREAD_MUTEX_INITIALIZER;

//
// Calculates SHA256 hash from explicit structure fields
//
static void calculateMetaHash(xMemoryBlock_t* p_ptBlock)
{
    X_ASSERT(p_ptBlock != NULL);
    uint8_t l_ucBuffer[sizeof(p_ptBlock->t_ulCanaryPrefix)
        + sizeof(p_ptBlock->t_ptAddress)
        + sizeof(p_ptBlock->t_ulSize)
        + sizeof(p_ptBlock->t_ptkcFile)
        + sizeof(p_ptBlock->t_iLine)];
    size_t offset = 0;

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_ulCanaryPrefix, sizeof(p_ptBlock->t_ulCanaryPrefix));
    offset += sizeof(p_ptBlock->t_ulCanaryPrefix);

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_ptAddress, sizeof(p_ptBlock->t_ptAddress));
    offset += sizeof(p_ptBlock->t_ptAddress);

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_ulSize, sizeof(p_ptBlock->t_ulSize));
    offset += sizeof(p_ptBlock->t_ulSize);

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_ptkcFile, sizeof(p_ptBlock->t_ptkcFile));
    offset += sizeof(p_ptBlock->t_ptkcFile);

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_iLine, sizeof(p_ptBlock->t_iLine));
    offset += sizeof(p_ptBlock->t_iLine);

    size_t l_ulHashSize;
    xHashCalculate(XOS_HASH_TYPE_SHA256, l_ucBuffer, offset,
        p_ptBlock->t_ucMetaHash, &l_ulHashSize);
}

//
// Checks memory block integrity using canary and hash
//
static int checkBlockIntegrity(xMemoryBlock_t* p_ptBlock)
{
    X_ASSERT(p_ptBlock != NULL);
    X_ASSERT_RETURN(p_ptBlock->t_ulCanaryPrefix == XOS_MEM_CANARY_PREFIX &&
        p_ptBlock->t_ulCanarySuffix == XOS_MEM_CANARY_SUFFIX,
        XOS_MEM_CORRUPTION);

    uint8_t l_ucCurrentHash[32];
    size_t l_ulHashSize;
    uint8_t l_ucBuffer[sizeof(p_ptBlock->t_ulCanaryPrefix)
        + sizeof(p_ptBlock->t_ptAddress)
        + sizeof(p_ptBlock->t_ulSize)
        + sizeof(p_ptBlock->t_ptkcFile)
        + sizeof(p_ptBlock->t_iLine)];
    size_t offset = 0;

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_ulCanaryPrefix, sizeof(p_ptBlock->t_ulCanaryPrefix));
    offset += sizeof(p_ptBlock->t_ulCanaryPrefix);

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_ptAddress, sizeof(p_ptBlock->t_ptAddress));
    offset += sizeof(p_ptBlock->t_ptAddress);

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_ulSize, sizeof(p_ptBlock->t_ulSize));
    offset += sizeof(p_ptBlock->t_ulSize);

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_ptkcFile, sizeof(p_ptBlock->t_ptkcFile));
    offset += sizeof(p_ptBlock->t_ptkcFile);

    memcpy(l_ucBuffer + offset, &p_ptBlock->t_iLine, sizeof(p_ptBlock->t_iLine));
    offset += sizeof(p_ptBlock->t_iLine);

    xHashCalculate(XOS_HASH_TYPE_SHA256, l_ucBuffer, offset,
        l_ucCurrentHash, &l_ulHashSize);

    if (memcmp(l_ucCurrentHash, p_ptBlock->t_ucMetaHash, sizeof(l_ucCurrentHash)) != 0)
    {
        return XOS_MEM_CORRUPTION;
    }

    return XOS_MEM_OK;
}

//
// Initializes the global memory manager
//
int xMemInit(void)
{
    pthread_mutex_lock(&s_memory_mutex);
    X_ASSERT(s_tMemoryManager.t_ptBlocks == NULL);
    memset(&s_tMemoryManager, 0, sizeof(xMemoryManager_t));
    pthread_mutex_unlock(&s_memory_mutex);
    return XOS_MEM_OK;
}

//
// Allocates a memory block and creates corresponding metadata structure
//
void* xMemAlloc(size_t p_ulSize, const char* p_ptkcFile, int p_iLine)
{
    X_ASSERT(p_ulSize > 0);
    X_ASSERT(p_ptkcFile != NULL);

    // Overflow check
    if (p_ulSize > SIZE_MAX - sizeof(xMemoryBlock_t))
    {
        return NULL;
    }

    pthread_mutex_lock(&s_memory_mutex);
    if (s_tMemoryManager.t_ulTotalAllocated + p_ulSize > XOS_MEM_MAX_ALLOCATION)
    {
        pthread_mutex_unlock(&s_memory_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&s_memory_mutex);

    void* l_ptPtr = malloc(p_ulSize);
    if (l_ptPtr == NULL) return NULL;

    xMemoryBlock_t* l_ptBlock = malloc(sizeof(xMemoryBlock_t));
    if (l_ptBlock == NULL)
    {
        free(l_ptPtr);
        return NULL;
    }

    l_ptBlock->t_ulCanaryPrefix = XOS_MEM_CANARY_PREFIX;
    l_ptBlock->t_ptAddress = l_ptPtr;
    l_ptBlock->t_ulSize = p_ulSize;
    l_ptBlock->t_ptkcFile = p_ptkcFile;
    l_ptBlock->t_iLine = p_iLine;
    l_ptBlock->t_ulCanarySuffix = XOS_MEM_CANARY_SUFFIX;

    calculateMetaHash(l_ptBlock);

    pthread_mutex_lock(&s_memory_mutex);
    l_ptBlock->t_ptNext = s_tMemoryManager.t_ptBlocks;
    s_tMemoryManager.t_ptBlocks = l_ptBlock;

    s_tMemoryManager.t_ulTotalAllocated += p_ulSize;
    s_tMemoryManager.t_ulAllocCount++;
    if (s_tMemoryManager.t_ulTotalAllocated > s_tMemoryManager.t_ulPeakUsage)
    {
        s_tMemoryManager.t_ulPeakUsage = s_tMemoryManager.t_ulTotalAllocated;
    }
    pthread_mutex_unlock(&s_memory_mutex);

    return l_ptPtr;
}

//
// Frees a memory block and removes the associated metadata structure
//
int xMemFree(void* p_ptPtr)
{
    X_ASSERT(p_ptPtr != NULL);
    pthread_mutex_lock(&s_memory_mutex);
    xMemoryBlock_t* l_ptCurrent = s_tMemoryManager.t_ptBlocks;
    xMemoryBlock_t* l_ptPrev = NULL;

    while (l_ptCurrent != NULL)
    {
        if (l_ptCurrent->t_ptAddress == p_ptPtr)
        {
            if (checkBlockIntegrity(l_ptCurrent) != (int)XOS_MEM_OK)
            {
                pthread_mutex_unlock(&s_memory_mutex);
                return XOS_MEM_CORRUPTION;
            }

            if (l_ptPrev == NULL)
            {
                s_tMemoryManager.t_ptBlocks = l_ptCurrent->t_ptNext;
            }
            else
            {
                l_ptPrev->t_ptNext = l_ptCurrent->t_ptNext;
            }

            s_tMemoryManager.t_ulTotalAllocated -= l_ptCurrent->t_ulSize;
            s_tMemoryManager.t_ulFreeCount++;

            pthread_mutex_unlock(&s_memory_mutex);
            free(p_ptPtr);
            free(l_ptCurrent);

            return XOS_MEM_OK;
        }
        l_ptPrev = l_ptCurrent;
        l_ptCurrent = l_ptCurrent->t_ptNext;
    }
    pthread_mutex_unlock(&s_memory_mutex);
    return XOS_MEM_INVALID;
}

//
// Returns current memory statistics
//
int xMemGetStats(size_t* p_pulTotal, size_t* p_pulPeak, size_t* p_pulCount)
{
    X_ASSERT(p_pulTotal != NULL);
    X_ASSERT(p_pulPeak != NULL);
    X_ASSERT(p_pulCount != NULL);

    pthread_mutex_lock(&s_memory_mutex);
    *p_pulTotal = s_tMemoryManager.t_ulTotalAllocated;
    *p_pulPeak = s_tMemoryManager.t_ulPeakUsage;
    *p_pulCount = s_tMemoryManager.t_ulAllocCount;
    pthread_mutex_unlock(&s_memory_mutex);

    return XOS_MEM_OK;
}

//
// Checks integrity of all allocated blocks
//
int xMemCheck(void)
{
    pthread_mutex_lock(&s_memory_mutex);
    xMemoryBlock_t* l_ptCurrent = s_tMemoryManager.t_ptBlocks;
    while (l_ptCurrent != NULL)
    {
        if (checkBlockIntegrity(l_ptCurrent) != (int)XOS_MEM_OK)
        {
            pthread_mutex_unlock(&s_memory_mutex);
            return XOS_MEM_CORRUPTION;
        }
        l_ptCurrent = l_ptCurrent->t_ptNext;
    }
    pthread_mutex_unlock(&s_memory_mutex);
    return XOS_MEM_OK;
}

//
// Frees all allocated blocks and resets the manager
//
int xMemCleanup(void)
{
    pthread_mutex_lock(&s_memory_mutex);
    xMemoryBlock_t* l_ptCurrent = s_tMemoryManager.t_ptBlocks;
    while (l_ptCurrent != NULL)
    {
        xMemoryBlock_t* l_ptNext = l_ptCurrent->t_ptNext;
        free(l_ptCurrent->t_ptAddress);
        free(l_ptCurrent);
        l_ptCurrent = l_ptNext;
    }
    memset(&s_tMemoryManager, 0, sizeof(xMemoryManager_t));
    pthread_mutex_unlock(&s_memory_mutex);
    return XOS_MEM_OK;
}

#ifdef DEBUG
//
// Function used to simulate memory corruption (for testing purposes)
// by altering the canary of an allocated block.
//
void xMemCorrupt(void)
{
    pthread_mutex_lock(&s_memory_mutex);
    if (s_tMemoryManager.t_ptBlocks != NULL)
    {
        s_tMemoryManager.t_ptBlocks->t_ulCanaryPrefix = 0;
    }
    pthread_mutex_unlock(&s_memory_mutex);
}
#endif
