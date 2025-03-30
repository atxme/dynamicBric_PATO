////////////////////////////////////////////////////////////
//  cipher engine source file
//  implements the cipher engine functions using OpenSSL
//
// general discloser: copy or share the file is forbidden
// Written : 30/03/2025
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////

#include "xCipherEngine.h"
#include "xAssert.h"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <string.h>

// Structure de contexte interne pour le chiffrement
struct xos_cipher_ctx_t {
    EVP_CIPHER_CTX* t_ptEvpCtx;            // Contexte OpenSSL
    const EVP_CIPHER* t_ptCipher;          // Algorithme de chiffrement
    t_cipherAlgorithm t_eAlgorithm;        // Type d'algorithme
    t_cipherMode t_eMode;                  // Mode de chiffrement
    t_cipherKeySize t_eKeySize;            // Taille de clé
    t_cipherPadding t_ePadding;            // Type de padding
    int t_bIsEncrypt;                      // Flag de chiffrement/déchiffrement
    int t_bInitialized;                    // Flag d'initialisation
    int t_bAadSet;                         // Flag AAD défini
    uint8_t t_aTag[16];                    // Tag d'authentification
    size_t t_ulTagLen;                     // Longueur du tag
};

// Conversion de cipher OpenSSL en fonction des paramètres
static const EVP_CIPHER* _getCipher(t_cipherAlgorithm p_eAlgorithm, t_cipherMode p_eMode, t_cipherKeySize p_eKeySize)
{
    // AES
    if (p_eAlgorithm == XOS_CIPHER_ALG_AES) {
        // AES-CBC
        if (p_eMode == XOS_CIPHER_MODE_CBC) {
            if (p_eKeySize == XOS_CIPHER_KEY_128) return EVP_aes_128_cbc();
            if (p_eKeySize == XOS_CIPHER_KEY_192) return EVP_aes_192_cbc();
            if (p_eKeySize == XOS_CIPHER_KEY_256) return EVP_aes_256_cbc();
        }
        // AES-CFB
        else if (p_eMode == XOS_CIPHER_MODE_CFB) {
            if (p_eKeySize == XOS_CIPHER_KEY_128) return EVP_aes_128_cfb128();
            if (p_eKeySize == XOS_CIPHER_KEY_192) return EVP_aes_192_cfb128();
            if (p_eKeySize == XOS_CIPHER_KEY_256) return EVP_aes_256_cfb128();
        }
        // AES-GCM
        else if (p_eMode == XOS_CIPHER_MODE_GCM) {
            if (p_eKeySize == XOS_CIPHER_KEY_128) return EVP_aes_128_gcm();
            if (p_eKeySize == XOS_CIPHER_KEY_192) return EVP_aes_192_gcm();
            if (p_eKeySize == XOS_CIPHER_KEY_256) return EVP_aes_256_gcm();
        }
        // AES-CCM
        else if (p_eMode == XOS_CIPHER_MODE_CCM) {
            if (p_eKeySize == XOS_CIPHER_KEY_128) return EVP_aes_128_ccm();
            if (p_eKeySize == XOS_CIPHER_KEY_192) return EVP_aes_192_ccm();
            if (p_eKeySize == XOS_CIPHER_KEY_256) return EVP_aes_256_ccm();
        }
        // AES-XTS
        else if (p_eMode == XOS_CIPHER_MODE_XTS) {
            if (p_eKeySize == XOS_CIPHER_KEY_128) return EVP_aes_128_xts();
            if (p_eKeySize == XOS_CIPHER_KEY_256) return EVP_aes_256_xts();
        }
    }
    // ChaCha20
    else if (p_eAlgorithm == XOS_CIPHER_ALG_CHACHA20) {
        return EVP_chacha20();
    }

    return NULL;
}

// Déterminer la taille de l'IV en fonction du chiffrement
static int _getIVLength(const EVP_CIPHER* p_ptCipher, t_cipherMode p_eMode)
{
    if (p_ptCipher == NULL) return 0;
    
    // Cas spécial pour CCM qui nécessite une taille d'IV de 7-13 octets
    if (p_eMode == XOS_CIPHER_MODE_CCM) {
        return 12; // Recommandé pour CCM
    }
    
    return EVP_CIPHER_iv_length(p_ptCipher);
}

// Déterminer la taille de bloc en fonction du chiffrement
static int _getBlockSize(const EVP_CIPHER* p_ptCipher)
{
    if (p_ptCipher == NULL) return 0;
    return EVP_CIPHER_block_size(p_ptCipher);
}

// Configurer le padding selon le mode
static int _setupPadding(EVP_CIPHER_CTX* p_ptCtx, t_cipherPadding p_ePadding, t_cipherMode p_eMode)
{
    // Pas de padding pour les modes de flux ou authentifiés
    if (p_eMode == XOS_CIPHER_MODE_CFB || 
        p_eMode == XOS_CIPHER_MODE_GCM ||
        p_eMode == XOS_CIPHER_MODE_CCM) {
        EVP_CIPHER_CTX_set_padding(p_ptCtx, 0);
        return XOS_CIPHER_OK;
    }
    
    // Configurer le padding pour les modes par bloc
    switch (p_ePadding) {
        case XOS_CIPHER_PADDING_NONE:
            EVP_CIPHER_CTX_set_padding(p_ptCtx, 0);
            break;
        case XOS_CIPHER_PADDING_PKCS7:
            EVP_CIPHER_CTX_set_padding(p_ptCtx, 1);
            break;
        case XOS_CIPHER_PADDING_ZERO:
            // OpenSSL ne supporte pas nativement le zero-padding
            // Il faut implémenter manuellement pour les finalisations
            EVP_CIPHER_CTX_set_padding(p_ptCtx, 0);
            break;
        default:
            return XOS_CIPHER_INVALID;
    }
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherCreate
////////////////////////////////////////////////////////////
xos_cipher_ctx_t* xCipherCreate(t_cipherAlgorithm p_eAlgorithm,
                              t_cipherMode p_eMode,
                              t_cipherKeySize p_eKeySize,
                              t_cipherPadding p_ePadding)
{
    // Allouer le contexte
    xos_cipher_ctx_t* l_ptCtx = (xos_cipher_ctx_t*)malloc(sizeof(xos_cipher_ctx_t));
    if (l_ptCtx == NULL) {
        return NULL;
    }
    
    // Initialiser la structure
    memset(l_ptCtx, 0, sizeof(xos_cipher_ctx_t));
    
    // Créer le contexte EVP
    l_ptCtx->t_ptEvpCtx = EVP_CIPHER_CTX_new();
    if (l_ptCtx->t_ptEvpCtx == NULL) {
        free(l_ptCtx);
        return NULL;
    }
    
    // Stocker les paramètres
    l_ptCtx->t_eAlgorithm = p_eAlgorithm;
    l_ptCtx->t_eMode = p_eMode;
    l_ptCtx->t_eKeySize = p_eKeySize;
    l_ptCtx->t_ePadding = p_ePadding;
    
    // Obtenir le cipher correspondant
    l_ptCtx->t_ptCipher = _getCipher(p_eAlgorithm, p_eMode, p_eKeySize);
    if (l_ptCtx->t_ptCipher == NULL) {
        EVP_CIPHER_CTX_free(l_ptCtx->t_ptEvpCtx);
        free(l_ptCtx);
        return NULL;
    }
    
    return l_ptCtx;
}

////////////////////////////////////////////////////////////
/// xCipherDestroy
////////////////////////////////////////////////////////////
void xCipherDestroy(xos_cipher_ctx_t* p_ptCtx)
{
    if (p_ptCtx == NULL) {
        return;
    }
    
    // Libérer le contexte OpenSSL
    if (p_ptCtx->t_ptEvpCtx != NULL) {
        EVP_CIPHER_CTX_free(p_ptCtx->t_ptEvpCtx);
    }
    
    // Libérer la structure
    memset(p_ptCtx, 0, sizeof(xos_cipher_ctx_t));
    free(p_ptCtx);
}

////////////////////////////////////////////////////////////
/// xCipherInit
////////////////////////////////////////////////////////////
int xCipherInit(xos_cipher_ctx_t* p_ptCtx,
               const uint8_t* p_ptKey, size_t p_ulKeyLen,
               const uint8_t* p_ptIv, size_t p_ulIvLen,
               int p_bEncrypt)
{
    int l_iResult;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptKey != NULL, XOS_CIPHER_INVALID_KEY);
    
    // Vérifier la taille de clé
    int l_iExpectedKeyLen = EVP_CIPHER_key_length(p_ptCtx->t_ptCipher);
    if ((int)p_ulKeyLen != l_iExpectedKeyLen) {
        return XOS_CIPHER_INVALID_KEY;
    }
    
    // Vérifier l'IV si nécessaire
    int l_iExpectedIvLen = _getIVLength(p_ptCtx->t_ptCipher, p_ptCtx->t_eMode);
    if (l_iExpectedIvLen > 0) {
        X_ASSERT_RETURN(p_ptIv != NULL, XOS_CIPHER_INVALID_IV);
        X_ASSERT_RETURN(p_ulIvLen == (size_t)l_iExpectedIvLen, XOS_CIPHER_INVALID_IV);
    }
    
    // Stockage du mode
    p_ptCtx->t_bIsEncrypt = p_bEncrypt;
    
    // Initialiser le contexte
    if (p_bEncrypt) {
        l_iResult = EVP_EncryptInit_ex(p_ptCtx->t_ptEvpCtx, p_ptCtx->t_ptCipher, NULL, p_ptKey, p_ptIv);
    } else {
        l_iResult = EVP_DecryptInit_ex(p_ptCtx->t_ptEvpCtx, p_ptCtx->t_ptCipher, NULL, p_ptKey, p_ptIv);
    }
    
    if (l_iResult != 1) {
        return XOS_CIPHER_ERROR;
    }
    
    // Configurer le padding
    l_iResult = _setupPadding(p_ptCtx->t_ptEvpCtx, p_ptCtx->t_ePadding, p_ptCtx->t_eMode);
    if (l_iResult != XOS_CIPHER_OK) {
        return l_iResult;
    }
    
    p_ptCtx->t_bInitialized = 1;
    p_ptCtx->t_bAadSet = 0;
    p_ptCtx->t_ulTagLen = 0;
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherUpdate
////////////////////////////////////////////////////////////
int xCipherUpdate(xos_cipher_ctx_t* p_ptCtx,
                 const uint8_t* p_ptIn, size_t p_ulInLen,
                 uint8_t* p_ptOut, size_t* p_pulOutLen)
{
    int l_iOutLen = 0;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL && p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptIn != NULL || p_ulInLen == 0, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptOut != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_pulOutLen != NULL, XOS_CIPHER_INVALID);
    
    // Vérifier l'espace de sortie
    if (*p_pulOutLen < p_ulInLen + _getBlockSize(p_ptCtx->t_ptCipher)) {
        return XOS_CIPHER_BUFFER_TOO_SMALL;
    }
    
    // Mise à jour du chiffrement
    if (p_ptCtx->t_bIsEncrypt) {
        if (!EVP_EncryptUpdate(p_ptCtx->t_ptEvpCtx, p_ptOut, &l_iOutLen, p_ptIn, (int)p_ulInLen)) {
            return XOS_CIPHER_ERROR;
        }
    } else {
        if (!EVP_DecryptUpdate(p_ptCtx->t_ptEvpCtx, p_ptOut, &l_iOutLen, p_ptIn, (int)p_ulInLen)) {
            return XOS_CIPHER_ERROR;
        }
    }
    
    *p_pulOutLen = (size_t)l_iOutLen;
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherFinalize
////////////////////////////////////////////////////////////
int xCipherFinalize(xos_cipher_ctx_t* p_ptCtx,
                   uint8_t* p_ptOut, size_t* p_pulOutLen)
{
    int l_iOutLen = 0;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL && p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptOut != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_pulOutLen != NULL, XOS_CIPHER_INVALID);
    
    // Vérifier l'espace de sortie
    if (*p_pulOutLen < (size_t)_getBlockSize(p_ptCtx->t_ptCipher)) {
        return XOS_CIPHER_BUFFER_TOO_SMALL;
    }
    
    // Finaliser le chiffrement/déchiffrement
    if (p_ptCtx->t_bIsEncrypt) {
        if (!EVP_EncryptFinal_ex(p_ptCtx->t_ptEvpCtx, p_ptOut, &l_iOutLen)) {
            return XOS_CIPHER_ERROR;
        }
        
        // Récupérer le tag si mode authentifié
        if (p_ptCtx->t_eMode == XOS_CIPHER_MODE_GCM || p_ptCtx->t_eMode == XOS_CIPHER_MODE_CCM) {
            if (!EVP_CIPHER_CTX_ctrl(p_ptCtx->t_ptEvpCtx, EVP_CTRL_GCM_GET_TAG, 
                                    sizeof(p_ptCtx->t_aTag), p_ptCtx->t_aTag)) {
                return XOS_CIPHER_ERROR;
            }
            p_ptCtx->t_ulTagLen = sizeof(p_ptCtx->t_aTag);
        }
    } else {
        // Vérifier le tag pour les modes authentifiés avant la finalisation
        if ((p_ptCtx->t_eMode == XOS_CIPHER_MODE_GCM || p_ptCtx->t_eMode == XOS_CIPHER_MODE_CCM) 
            && p_ptCtx->t_ulTagLen > 0) {
            if (!EVP_CIPHER_CTX_ctrl(p_ptCtx->t_ptEvpCtx, EVP_CTRL_GCM_SET_TAG, 
                                    (int)p_ptCtx->t_ulTagLen, p_ptCtx->t_aTag)) {
                return XOS_CIPHER_ERROR;
            }
        }
        
        if (!EVP_DecryptFinal_ex(p_ptCtx->t_ptEvpCtx, p_ptOut, &l_iOutLen)) {
            return XOS_CIPHER_AUTH_FAILED;
        }
    }
    
    *p_pulOutLen = (size_t)l_iOutLen;
    
    // Gestion du zero-padding si nécessaire et si en mode chiffrement
    if (p_ptCtx->t_ePadding == XOS_CIPHER_PADDING_ZERO && p_ptCtx->t_bIsEncrypt) {
        int blockSize = _getBlockSize(p_ptCtx->t_ptCipher);
        if (blockSize > 1) {  // Ne s'applique qu'aux chiffrements par bloc
            int paddingNeeded = blockSize - (*p_pulOutLen % blockSize);
            if (paddingNeeded > 0 && paddingNeeded < blockSize) {
                // Ajouter du zero-padding manuellement
                if (*p_pulOutLen + paddingNeeded <= *p_pulOutLen) {
                    memset(p_ptOut + *p_pulOutLen, 0, paddingNeeded);
                    *p_pulOutLen += paddingNeeded;
                }
            }
        }
    }
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherAddAuthData
////////////////////////////////////////////////////////////
int xCipherAddAuthData(xos_cipher_ctx_t* p_ptCtx,
                      const uint8_t* p_ptAad, size_t p_ulAadLen)
{
    int l_iOutLen = 0;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL && p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptAad != NULL, XOS_CIPHER_INVALID_AAD);
    
    // Vérifier que le mode est authentifié
    if (p_ptCtx->t_eMode != XOS_CIPHER_MODE_GCM && p_ptCtx->t_eMode != XOS_CIPHER_MODE_CCM) {
        return XOS_CIPHER_INVALID;
    }
    
    // Ajouter les données authentifiées
    if (p_ptCtx->t_bIsEncrypt) {
        if (!EVP_EncryptUpdate(p_ptCtx->t_ptEvpCtx, NULL, &l_iOutLen, p_ptAad, (int)p_ulAadLen)) {
            return XOS_CIPHER_ERROR;
        }
    } else {
        if (!EVP_DecryptUpdate(p_ptCtx->t_ptEvpCtx, NULL, &l_iOutLen, p_ptAad, (int)p_ulAadLen)) {
            return XOS_CIPHER_ERROR;
        }
    }
    
    p_ptCtx->t_bAadSet = 1;
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherSetTag
////////////////////////////////////////////////////////////
int xCipherSetTag(xos_cipher_ctx_t* p_ptCtx,
                 const uint8_t* p_ptTag, size_t p_ulTagLen)
{
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL && p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptTag != NULL, XOS_CIPHER_INVALID_TAG);
    X_ASSERT_RETURN(p_ulTagLen > 0 && p_ulTagLen <= sizeof(p_ptCtx->t_aTag), XOS_CIPHER_INVALID_TAG);
    
    // Vérifier que le mode est authentifié
    if (p_ptCtx->t_eMode != XOS_CIPHER_MODE_GCM && p_ptCtx->t_eMode != XOS_CIPHER_MODE_CCM) {
        return XOS_CIPHER_INVALID;
    }
    
    // Vérifier que nous sommes en mode déchiffrement
    if (p_ptCtx->t_bIsEncrypt) {
        return XOS_CIPHER_INVALID;
    }
    
    // Copier le tag
    memcpy(p_ptCtx->t_aTag, p_ptTag, p_ulTagLen);
    p_ptCtx->t_ulTagLen = p_ulTagLen;
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherGetTag
////////////////////////////////////////////////////////////
int xCipherGetTag(xos_cipher_ctx_t* p_ptCtx,
                 uint8_t* p_ptTag, size_t p_ulTagLen)
{
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL && p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptTag != NULL, XOS_CIPHER_INVALID_TAG);
    X_ASSERT_RETURN(p_ulTagLen > 0, XOS_CIPHER_INVALID_TAG);
    
    // Vérifier que le mode est authentifié
    if (p_ptCtx->t_eMode != XOS_CIPHER_MODE_GCM && p_ptCtx->t_eMode != XOS_CIPHER_MODE_CCM) {
        return XOS_CIPHER_INVALID;
    }
    
    // Vérifier que nous sommes en mode chiffrement
    if (!p_ptCtx->t_bIsEncrypt) {
        return XOS_CIPHER_INVALID;
    }
    
    // Vérifier que le tag a été généré
    if (p_ptCtx->t_ulTagLen == 0) {
        return XOS_CIPHER_INVALID_TAG;
    }
    
    // Copier le tag
    size_t l_ulCopyLen = (p_ulTagLen < p_ptCtx->t_ulTagLen) ? p_ulTagLen : p_ptCtx->t_ulTagLen;
    memcpy(p_ptTag, p_ptCtx->t_aTag, l_ulCopyLen);
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherInitConfig
////////////////////////////////////////////////////////////
int xCipherInitConfig(xos_cipher_config_t* p_ptConfig)
{
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptConfig != NULL, XOS_CIPHER_INVALID);
    
    // Initialiser la structure avec des valeurs par défaut
    memset(p_ptConfig, 0, sizeof(xos_cipher_config_t));
    
    // Valeurs par défaut pour TLS 1.3 (AES-GCM est le cipher recommandé)
    p_ptConfig->t_eAlgorithm = XOS_CIPHER_ALG_AES;
    p_ptConfig->t_eMode = XOS_CIPHER_MODE_GCM;  // GCM est le mode recommandé pour TLS 1.3
    p_ptConfig->t_eKeySize = XOS_CIPHER_KEY_256;
    p_ptConfig->t_ePadding = XOS_CIPHER_PADDING_NONE;
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherEncrypt
////////////////////////////////////////////////////////////
int xCipherEncrypt(const xos_cipher_config_t* p_ptConfig,
                  const uint8_t* p_ptIn, size_t p_ulInLen,
                  uint8_t* p_ptOut, size_t* p_pulOutLen)
{
    int l_iResult;
    size_t l_ulOutLen = 0;
    size_t l_ulFinalLen = 0;
    xos_cipher_ctx_t* l_ptCtx = NULL;
    
    // Vérifier les paramètres essentiels
    X_ASSERT_RETURN(p_ptConfig != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptConfig->t_ptKey != NULL && p_ptConfig->t_ulKeyLen > 0, XOS_CIPHER_INVALID_KEY);
    X_ASSERT_RETURN(p_ptIn != NULL || p_ulInLen == 0, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptOut != NULL && p_pulOutLen != NULL, XOS_CIPHER_INVALID);
    
    // Créer le contexte
    l_ptCtx = xCipherCreate(p_ptConfig->t_eAlgorithm, p_ptConfig->t_eMode, 
                           p_ptConfig->t_eKeySize, p_ptConfig->t_ePadding);
    if (l_ptCtx == NULL) {
        return XOS_CIPHER_ERROR;
    }
    
    // Initialiser le chiffrement
    l_iResult = xCipherInit(l_ptCtx, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen, 
                           p_ptConfig->t_ptIv, p_ptConfig->t_ulIvLen, 1);
    if (l_iResult != XOS_CIPHER_OK) {
        xCipherDestroy(l_ptCtx);
        return l_iResult;
    }
    
    // Ajouter les données authentifiées si nécessaire
    if ((p_ptConfig->t_eMode == XOS_CIPHER_MODE_GCM || p_ptConfig->t_eMode == XOS_CIPHER_MODE_CCM) && 
        p_ptConfig->t_ptAad != NULL && p_ptConfig->t_ulAadLen > 0) {
        l_iResult = xCipherAddAuthData(l_ptCtx, p_ptConfig->t_ptAad, p_ptConfig->t_ulAadLen);
        if (l_iResult != XOS_CIPHER_OK) {
            xCipherDestroy(l_ptCtx);
            return l_iResult;
        }
    }
    
    // Calculer l'espace requis
    size_t l_ulMaxOutSize = p_ulInLen + _getBlockSize(l_ptCtx->t_ptCipher);
    if (*p_pulOutLen < l_ulMaxOutSize) {
        xCipherDestroy(l_ptCtx);
        return XOS_CIPHER_BUFFER_TOO_SMALL;
    }
    
    // Mettre à jour le chiffrement
    l_ulOutLen = *p_pulOutLen;
    l_iResult = xCipherUpdate(l_ptCtx, p_ptIn, p_ulInLen, p_ptOut, &l_ulOutLen);
    if (l_iResult != XOS_CIPHER_OK) {
        xCipherDestroy(l_ptCtx);
        return l_iResult;
    }
    
    // Finaliser le chiffrement
    l_ulFinalLen = *p_pulOutLen - l_ulOutLen;
    l_iResult = xCipherFinalize(l_ptCtx, p_ptOut + l_ulOutLen, &l_ulFinalLen);
    if (l_iResult != XOS_CIPHER_OK) {
        xCipherDestroy(l_ptCtx);
        return l_iResult;
    }
    
    // Mettre à jour la taille de sortie
    *p_pulOutLen = l_ulOutLen + l_ulFinalLen;
    
    // Récupérer le tag si mode authentifié et buffer fourni
    if ((p_ptConfig->t_eMode == XOS_CIPHER_MODE_GCM || p_ptConfig->t_eMode == XOS_CIPHER_MODE_CCM) && 
        p_ptConfig->t_ptTag != NULL && p_ptConfig->t_ulTagLen > 0) {
        l_iResult = xCipherGetTag(l_ptCtx, p_ptConfig->t_ptTag, p_ptConfig->t_ulTagLen);
        if (l_iResult != XOS_CIPHER_OK) {
            xCipherDestroy(l_ptCtx);
            return l_iResult;
        }
    }
    
    // Libérer le contexte
    xCipherDestroy(l_ptCtx);
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherDecrypt
////////////////////////////////////////////////////////////
int xCipherDecrypt(const xos_cipher_config_t* p_ptConfig,
                  const uint8_t* p_ptIn, size_t p_ulInLen,
                  uint8_t* p_ptOut, size_t* p_pulOutLen)
{
    int l_iResult;
    size_t l_ulOutLen = 0;
    size_t l_ulFinalLen = 0;
    xos_cipher_ctx_t* l_ptCtx = NULL;
    
    // Vérifier les paramètres essentiels
    X_ASSERT_RETURN(p_ptConfig != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptConfig->t_ptKey != NULL && p_ptConfig->t_ulKeyLen > 0, XOS_CIPHER_INVALID_KEY);
    X_ASSERT_RETURN(p_ptIn != NULL || p_ulInLen == 0, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptOut != NULL && p_pulOutLen != NULL, XOS_CIPHER_INVALID);
    
    // Vérifier les paramètres supplémentaires pour les modes authentifiés
    if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_GCM || p_ptConfig->t_eMode == XOS_CIPHER_MODE_CCM) {
        X_ASSERT_RETURN(p_ptConfig->t_ptTag != NULL && p_ptConfig->t_ulTagLen > 0, XOS_CIPHER_INVALID_TAG);
    }
    
    // Créer le contexte
    l_ptCtx = xCipherCreate(p_ptConfig->t_eAlgorithm, p_ptConfig->t_eMode, 
                           p_ptConfig->t_eKeySize, p_ptConfig->t_ePadding);
    if (l_ptCtx == NULL) {
        return XOS_CIPHER_ERROR;
    }
    
    // Initialiser le déchiffrement
    l_iResult = xCipherInit(l_ptCtx, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen, 
                           p_ptConfig->t_ptIv, p_ptConfig->t_ulIvLen, 0);
    if (l_iResult != XOS_CIPHER_OK) {
        xCipherDestroy(l_ptCtx);
        return l_iResult;
    }
    
    // Définir le tag pour les modes authentifiés
    if ((p_ptConfig->t_eMode == XOS_CIPHER_MODE_GCM || p_ptConfig->t_eMode == XOS_CIPHER_MODE_CCM) && 
        p_ptConfig->t_ptTag != NULL && p_ptConfig->t_ulTagLen > 0) {
        l_iResult = xCipherSetTag(l_ptCtx, p_ptConfig->t_ptTag, p_ptConfig->t_ulTagLen);
        if (l_iResult != XOS_CIPHER_OK) {
            xCipherDestroy(l_ptCtx);
            return l_iResult;
        }
    }
    
    // Ajouter les données authentifiées si nécessaire
    if ((p_ptConfig->t_eMode == XOS_CIPHER_MODE_GCM || p_ptConfig->t_eMode == XOS_CIPHER_MODE_CCM) && 
        p_ptConfig->t_ptAad != NULL && p_ptConfig->t_ulAadLen > 0) {
        l_iResult = xCipherAddAuthData(l_ptCtx, p_ptConfig->t_ptAad, p_ptConfig->t_ulAadLen);
        if (l_iResult != XOS_CIPHER_OK) {
            xCipherDestroy(l_ptCtx);
            return l_iResult;
        }
    }
    
    // Calculer l'espace requis
    if (*p_pulOutLen < p_ulInLen) {
        xCipherDestroy(l_ptCtx);
        return XOS_CIPHER_BUFFER_TOO_SMALL;
    }
    
    // Mettre à jour le déchiffrement
    l_ulOutLen = *p_pulOutLen;
    l_iResult = xCipherUpdate(l_ptCtx, p_ptIn, p_ulInLen, p_ptOut, &l_ulOutLen);
    if (l_iResult != XOS_CIPHER_OK) {
        xCipherDestroy(l_ptCtx);
        return l_iResult;
    }
    
    // Finaliser le déchiffrement
    l_ulFinalLen = *p_pulOutLen - l_ulOutLen;
    l_iResult = xCipherFinalize(l_ptCtx, p_ptOut + l_ulOutLen, &l_ulFinalLen);
    if (l_iResult != XOS_CIPHER_OK) {
        xCipherDestroy(l_ptCtx);
        return l_iResult;
    }
    
    // Mettre à jour la taille de sortie
    *p_pulOutLen = l_ulOutLen + l_ulFinalLen;
    
    // Libérer le contexte
    xCipherDestroy(l_ptCtx);
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherGenerateKey
////////////////////////////////////////////////////////////
int xCipherGenerateKey(uint8_t* p_ptKey, size_t p_ulKeyLen)
{
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptKey != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ulKeyLen > 0, XOS_CIPHER_INVALID);
    
    // Générer des octets aléatoires pour la clé
    if (RAND_bytes(p_ptKey, (int)p_ulKeyLen) != 1) {
        return XOS_CIPHER_ERROR;
    }
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherGenerateIV
////////////////////////////////////////////////////////////
int xCipherGenerateIV(uint8_t* p_ptIv, size_t p_ulIvLen)
{
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptIv != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ulIvLen > 0, XOS_CIPHER_INVALID);
    
    // Générer des octets aléatoires pour l'IV
    if (RAND_bytes(p_ptIv, (int)p_ulIvLen) != 1) {
        return XOS_CIPHER_ERROR;
    }
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherGetOutputSize
////////////////////////////////////////////////////////////
size_t xCipherGetOutputSize(t_cipherAlgorithm p_eAlgorithm,
                           t_cipherMode p_eMode,
                           t_cipherPadding p_ePadding,
                           size_t p_ulInputLen,
                           int p_bIsUpdate)
{
    // Obtenir la taille de bloc
    const EVP_CIPHER* l_ptCipher = _getCipher(p_eAlgorithm, p_eMode, XOS_CIPHER_KEY_128);
    if (l_ptCipher == NULL) {
        return 0;
    }
    
    int l_iBlockSize = _getBlockSize(l_ptCipher);
    
    // Pour les mises à jour, l'output peut être de taille input + 1 bloc au maximum
    if (p_bIsUpdate) {
        return p_ulInputLen + l_iBlockSize;
    }
    
    // Pour la finalisation, la taille dépend du padding et du mode
    if (p_eMode == XOS_CIPHER_MODE_CBC || p_eMode == XOS_CIPHER_MODE_XTS) {
        // Modes par bloc - la taille dépend du padding
        if (p_ePadding == XOS_CIPHER_PADDING_NONE) {
            return p_ulInputLen;
        } else {
            return p_ulInputLen + l_iBlockSize;
        }
    } else if (p_eMode == XOS_CIPHER_MODE_GCM || p_eMode == XOS_CIPHER_MODE_CCM) {
        // Pour les modes authentifiés, la taille de sortie est la même que l'entrée
        // (le tag est stocké séparément)
        return p_ulInputLen;
    } else {
        // Pour CFB (mode de flux), la taille de sortie est la même que l'entrée
        return p_ulInputLen;
    }
}
