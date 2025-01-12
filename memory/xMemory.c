////////////////////////////////////////////////////////////
//  memory source file
//  implements the memory management functions
//
// general discloser: copy or share the file is forbidden
// Written : 12/01/2025
////////////////////////////////////////////////////////////

#include "xMemory.h"
#include "xAssert.h"
#include "hash/xHash.h"
#include <stdlib.h>
#include <string.h>

// Global memory manager
static xMemoryManager_t s_tMemoryManager = { 0 };

static void calculateMetaHash(xMemoryBlock_t* p_ptBlock)
{
	X_ASSERT(p_ptBlock != NULL);
    uint8_t l_ucBuffer[sizeof(xMemoryBlock_t)];
    size_t l_ulHashSize;

    // Copy metadata excluding hash field
    memcpy(l_ucBuffer, &p_ptBlock->t_ulCanaryPrefix, sizeof(unsigned long));
    memcpy(l_ucBuffer + sizeof(unsigned long), &p_ptBlock->t_ptAddress,
        offsetof(xMemoryBlock_t, t_ucMetaHash) - offsetof(xMemoryBlock_t, t_ptAddress));

    xHashCalculate(XOS_HASH_TYPE_SHA256, l_ucBuffer,
        offsetof(xMemoryBlock_t, t_ucMetaHash),
        p_ptBlock->t_ucMetaHash, &l_ulHashSize);
}

static int checkBlockIntegrity(xMemoryBlock_t* p_ptBlock)
{
    X_ASSERT(p_ptBlock != NULL);
	X_ASSERT_RETURN(p_ptBlock->t_ulCanaryPrefix == XOS_MEM_CANARY_PREFIX && 
                    p_ptBlock->t_ulCanarySuffix == XOS_MEM_CANARY_SUFFIX, 
                    XOS_MEM_CORRUPTION);

    uint8_t l_ucCurrentHash[32];
    size_t l_ulHashSize;
    uint8_t l_ucBuffer[sizeof(xMemoryBlock_t)];

    memcpy(l_ucBuffer, &p_ptBlock->t_ulCanaryPrefix, sizeof(unsigned long));
    memcpy(l_ucBuffer + sizeof(unsigned long), &p_ptBlock->t_ptAddress,
        offsetof(xMemoryBlock_t, t_ucMetaHash) - offsetof(xMemoryBlock_t, t_ptAddress));

    xHashCalculate(XOS_HASH_TYPE_SHA256, l_ucBuffer,
        offsetof(xMemoryBlock_t, t_ucMetaHash),
        l_ucCurrentHash, &l_ulHashSize);

    if (memcmp(l_ucCurrentHash, p_ptBlock->t_ucMetaHash, 32) != 0)
    {
        return XOS_MEM_CORRUPTION;
    }

    return XOS_MEM_OK;
}

int xMemInit(void)
{
    X_ASSERT(s_tMemoryManager.t_ptBlocks == NULL);

    memset(&s_tMemoryManager, 0, sizeof(xMemoryManager_t));
    return XOS_MEM_OK;
}

void* xMemAlloc(size_t p_ulSize, const char* p_ptkcFile, int p_iLine)
{
    X_ASSERT(p_ulSize > 0);
    X_ASSERT(p_ptkcFile != NULL);

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

    l_ptBlock->t_ptNext = s_tMemoryManager.t_ptBlocks;
    s_tMemoryManager.t_ptBlocks = l_ptBlock;

    s_tMemoryManager.t_ulTotalAllocated += p_ulSize;
    s_tMemoryManager.t_ulAllocCount++;
    if (s_tMemoryManager.t_ulTotalAllocated > s_tMemoryManager.t_ulPeakUsage)
    {
        s_tMemoryManager.t_ulPeakUsage = s_tMemoryManager.t_ulTotalAllocated;
    }

    return l_ptPtr;
}

int xMemFree(void* p_ptPtr)
{
    X_ASSERT(p_ptPtr != NULL);

    xMemoryBlock_t* l_ptCurrent = s_tMemoryManager.t_ptBlocks;
    xMemoryBlock_t* l_ptPrev = NULL;

    while (l_ptCurrent != NULL)
    {
        if (l_ptCurrent->t_ptAddress == p_ptPtr)
        {
            if (checkBlockIntegrity(l_ptCurrent) != XOS_MEM_OK)
            {
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

            free(p_ptPtr);
            free(l_ptCurrent);

            return XOS_MEM_OK;
        }
        l_ptPrev = l_ptCurrent;
        l_ptCurrent = l_ptCurrent->t_ptNext;
    }

    return XOS_MEM_INVALID;
}

int xMemGetStats(size_t* p_pulTotal, size_t* p_pulPeak, size_t* p_pulCount)
{
    X_ASSERT(p_pulTotal != NULL);
    X_ASSERT(p_pulPeak != NULL);
    X_ASSERT(p_pulCount != NULL);

    *p_pulTotal = s_tMemoryManager.t_ulTotalAllocated;
    *p_pulPeak = s_tMemoryManager.t_ulPeakUsage;
    *p_pulCount = s_tMemoryManager.t_ulAllocCount;

    return XOS_MEM_OK;
}

int xMemCheck(void)
{
    xMemoryBlock_t* l_ptCurrent = s_tMemoryManager.t_ptBlocks;

    while (l_ptCurrent != NULL)
    {
        if (checkBlockIntegrity(l_ptCurrent) != XOS_MEM_OK)
        {
            return XOS_MEM_CORRUPTION;
        }
        l_ptCurrent = l_ptCurrent->t_ptNext;
    }

    return XOS_MEM_OK;
}

int xMemCleanup(void)
{
    xMemoryBlock_t* l_ptCurrent = s_tMemoryManager.t_ptBlocks;

    while (l_ptCurrent != NULL)
    {
        xMemoryBlock_t* l_ptNext = l_ptCurrent->t_ptNext;
        free(l_ptCurrent->t_ptAddress);
        free(l_ptCurrent);
        l_ptCurrent = l_ptNext;
    }

    memset(&s_tMemoryManager, 0, sizeof(xMemoryManager_t));
    return XOS_MEM_OK;
}
