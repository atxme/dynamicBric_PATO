////////////////////////////////////////////////////////////
//  hash source file
//  implements the hash functions using OpenSSL
//
// general discloser: copy or share the file is forbidden
// Written : 11/01/2025
////////////////////////////////////////////////////////////

#include "xHash.h"
#include "xAssert.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <string.h>

int xHashCalculate(xos_hash_type_t p_eType, const void* p_ptData, size_t p_ulSize,
    uint8_t* p_ptHash, size_t* p_pulHashSize)
{
    X_ASSERT_RETURN(p_ptData != NULL && p_ptHash != NULL && p_pulHashSize != NULL, XOS_HASH_INVALID);

    EVP_MD_CTX* l_ptContext = EVP_MD_CTX_new();
    X_ASSERT_RETURN(l_ptContext != NULL, XOS_HASH_ERROR);

    const EVP_MD* l_ptMd = NULL;
    unsigned int l_ulHashLen;

    // Sélectionner l'algorithme
    switch (p_eType)
    {
    case XOS_HASH_TYPE_SHA256:
        l_ptMd = EVP_sha256();
        break;
    case XOS_HASH_TYPE_SHA384:
        l_ptMd = EVP_sha384();
        break;
    case XOS_HASH_TYPE_SHA512:
        l_ptMd = EVP_sha512();
        break;
    case XOS_HASH_TYPE_SHA3_256:
        l_ptMd = EVP_sha3_256();
        break;
    case XOS_HASH_TYPE_SHA3_384:
        l_ptMd = EVP_sha3_384();
        break;
    case XOS_HASH_TYPE_SHA3_512:
        l_ptMd = EVP_sha3_512();
        break;
    default:
        EVP_MD_CTX_free(l_ptContext);
        return XOS_HASH_INVALID;
    }

    if (!EVP_DigestInit_ex(l_ptContext, l_ptMd, NULL) ||
        !EVP_DigestUpdate(l_ptContext, p_ptData, p_ulSize) ||
        !EVP_DigestFinal_ex(l_ptContext, p_ptHash, &l_ulHashLen))
    {
        EVP_MD_CTX_free(l_ptContext);
        return XOS_HASH_ERROR;
    }

    *p_pulHashSize = l_ulHashLen;
    EVP_MD_CTX_free(l_ptContext);
    return XOS_HASH_OK;
}

int xHashInit(xos_hash_type_t p_eType, void* p_ptContext)
{
    X_ASSERT_RETURN(p_ptContext != NULL, XOS_HASH_INVALID);

    EVP_MD_CTX* l_ptContext = EVP_MD_CTX_new();
    if (l_ptContext == NULL)
        return XOS_HASH_ERROR;

    const EVP_MD* l_ptMd = NULL;
    switch (p_eType)
    {
    case XOS_HASH_TYPE_SHA256:
        l_ptMd = EVP_sha256();
        break;
    case XOS_HASH_TYPE_SHA384:
        l_ptMd = EVP_sha384();
        break;
    case XOS_HASH_TYPE_SHA512:
        l_ptMd = EVP_sha512();
        break;
    case XOS_HASH_TYPE_SHA3_256:
        l_ptMd = EVP_sha3_256();
        break;
    case XOS_HASH_TYPE_SHA3_384:
        l_ptMd = EVP_sha3_384();
        break;
    case XOS_HASH_TYPE_SHA3_512:
        l_ptMd = EVP_sha3_512();
        break;
    default:
        EVP_MD_CTX_free(l_ptContext);
        return XOS_HASH_INVALID;
    }

    if (!EVP_DigestInit_ex(l_ptContext, l_ptMd, NULL))
    {
        EVP_MD_CTX_free(l_ptContext);
        return XOS_HASH_ERROR;
    }

    *(EVP_MD_CTX**)p_ptContext = l_ptContext;
    return XOS_HASH_OK;
}

int xHashUpdate(xos_hash_type_t p_eType, void* p_ptContext,
    const void* p_ptData, size_t p_ulSize)
{
    X_ASSERT_RETURN(p_ptContext != NULL && p_ptData != NULL, XOS_HASH_INVALID);

    EVP_MD_CTX* l_ptContext = *(EVP_MD_CTX**)p_ptContext;
    if (!EVP_DigestUpdate(l_ptContext, p_ptData, p_ulSize))
        return XOS_HASH_ERROR;

    return XOS_HASH_OK;
}

int xHashFinalize(xos_hash_type_t p_eType, void* p_ptContext,
    uint8_t* p_ptHash, size_t* p_pulHashSize)
{
    X_ASSERT_RETURN(p_ptContext != NULL && p_ptHash != NULL && p_pulHashSize != NULL,
        XOS_HASH_INVALID);

    EVP_MD_CTX* l_ptContext = *(EVP_MD_CTX**)p_ptContext;
    unsigned int l_ulHashLen;

    if (!EVP_DigestFinal_ex(l_ptContext, p_ptHash, &l_ulHashLen))
    {
        EVP_MD_CTX_free(l_ptContext);
        return XOS_HASH_ERROR;
    }

    *p_pulHashSize = l_ulHashLen;
    EVP_MD_CTX_free(l_ptContext);
    *(EVP_MD_CTX**)p_ptContext = NULL;

    return XOS_HASH_OK;
}
