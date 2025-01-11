////////////////////////////////////////////////////////////
//  memory source file
//  implements the memory management functions
//
// general discloser: copy or share the file is forbidden
// Written : 11/01/2025
////////////////////////////////////////////////////////////

#include "xMemory.h"
#include "xAssert.h"
#include "hash/xHash.h"
#include <stdlib.h>
#include <string.h>

// Global memory manager
static xMemoryManager_t g_tMemoryManager = { 0 };

int xMemInit(void)
{
    X_ASSERT_RETURN(g_tMemoryManager.t_ptBlocks == NULL, XOS_MEM_ALREADY_INIT);

    memset(&g_tMemoryManager, 0, sizeof(xMemoryManager_t));
    return XOS_MEM_OK;
}

void* xMemAlloc(size_t p_ulSize, const char* p_ptkcFile, int p_iLine)
{
    X_ASSERT(p_ulSize > 0);
    X_ASSERT(p_ptkcFile != NULL);

    // Allouer le bloc mémoire
    void* l_ptPtr = malloc(p_ulSize);
    if (l_ptPtr == NULL) return NULL;

    // Créer le bloc de suivi
    xMemoryBlock_t* l_ptBlock = malloc(sizeof(xMemoryBlock_t));
    if (l_ptBlock == NULL)
    {
        free(l_ptPtr);
        return NULL;
    }

    // Initialiser le bloc
    l_ptBlock->t_ptAddress = l_ptPtr;
    l_ptBlock->t_ulSize = p_ulSize;
    l_ptBlock->t_ptkcFile = p_ptkcFile;
    l_ptBlock->t_iLine = p_iLine;

    // Calculer le hash SHA256
    size_t l_ulHashSize;
    int l_iRet = xHashCalculate(XOS_HASH_TYPE_SHA256, l_ptPtr, p_ulSize,
        l_ptBlock->t_ucHash, &l_ulHashSize);

    if (l_iRet != XOS_HASH_OK)
    {
        free(l_ptPtr);
        free(l_ptBlock);
        return NULL;
    }

    // Ajouter à la liste
    l_ptBlock->t_ptNext = g_tMemoryManager.t_ptBlocks;
    g_tMemoryManager.t_ptBlocks = l_ptBlock;

    // Mettre à jour les statistiques
    g_tMemoryManager.t_ulTotalAllocated += p_ulSize;
    g_tMemoryManager.t_ulAllocCount++;
    if (g_tMemoryManager.t_ulTotalAllocated > g_tMemoryManager.t_ulPeakUsage)
    {
        g_tMemoryManager.t_ulPeakUsage = g_tMemoryManager.t_ulTotalAllocated;
    }

    return l_ptPtr;
}

int xMemFree(void* p_ptPtr)
{
    X_ASSERT(p_ptPtr != NULL);

    xMemoryBlock_t* l_ptCurrent = g_tMemoryManager.t_ptBlocks;
    xMemoryBlock_t* l_ptPrev = NULL;

    while (l_ptCurrent != NULL)
    {
        if (l_ptCurrent->t_ptAddress == p_ptPtr)
        {
            // Vérifier l'intégrité avec le hash SHA256
            uint8_t l_ucCurrentHash[XOS_HASH_SHA256_SIZE];
            size_t l_ulHashSize;

            int l_iRet = xHashCalculate(XOS_HASH_TYPE_SHA256, p_ptPtr,
                l_ptCurrent->t_ulSize,
                l_ucCurrentHash, &l_ulHashSize);

            if (l_iRet != XOS_HASH_OK ||
                memcmp(l_ucCurrentHash, l_ptCurrent->t_ucHash, XOS_HASH_SHA256_SIZE) != 0)
            {
                return XOS_MEM_CORRUPTION;
            }

            // Mettre à jour la liste chaînée
            if (l_ptPrev == NULL)
            {
                g_tMemoryManager.t_ptBlocks = l_ptCurrent->t_ptNext;
            }
            else
            {
                l_ptPrev->t_ptNext = l_ptCurrent->t_ptNext;
            }

            // Mettre à jour les statistiques
            g_tMemoryManager.t_ulTotalAllocated -= l_ptCurrent->t_ulSize;
            g_tMemoryManager.t_ulFreeCount++;

            // Libérer la mémoire
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

    *p_pulTotal = g_tMemoryManager.t_ulTotalAllocated;
    *p_pulPeak = g_tMemoryManager.t_ulPeakUsage;
    *p_pulCount = g_tMemoryManager.t_ulAllocCount;

    return XOS_MEM_OK;
}

int xMemCheck(void)
{
    xMemoryBlock_t* l_ptCurrent = g_tMemoryManager.t_ptBlocks;

    while (l_ptCurrent != NULL)
    {
        uint8_t l_ucCurrentHash[XOS_HASH_SHA256_SIZE];
        size_t l_ulHashSize;

        int l_iRet = xHashCalculate(XOS_HASH_TYPE_SHA256,
            l_ptCurrent->t_ptAddress,
            l_ptCurrent->t_ulSize,
            l_ucCurrentHash,
            &l_ulHashSize);

        if (l_iRet != XOS_HASH_OK ||
            memcmp(l_ucCurrentHash, l_ptCurrent->t_ucHash, XOS_HASH_SHA256_SIZE) != 0)
        {
            return XOS_MEM_CORRUPTION;
        }

        l_ptCurrent = l_ptCurrent->t_ptNext;
    }

    return XOS_MEM_OK;
}

int xMemCleanup(void)
{
    xMemoryBlock_t* l_ptCurrent = g_tMemoryManager.t_ptBlocks;

    while (l_ptCurrent != NULL)
    {
        xMemoryBlock_t* l_ptNext = l_ptCurrent->t_ptNext;
        free(l_ptCurrent->t_ptAddress);
        free(l_ptCurrent);
        l_ptCurrent = l_ptNext;
    }

    memset(&g_tMemoryManager, 0, sizeof(xMemoryManager_t));
    return XOS_MEM_OK;
}