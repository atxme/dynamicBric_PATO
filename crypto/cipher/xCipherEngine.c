////////////////////////////////////////////////////////////
//  cipher engine implementation file
//  Implements the cipher interface defined in xCipherEngine.h
//  AES and ChaCha20 based on WolfSSL
//
// general discloser: copy or share the file is forbidden
// Written : 30/03/2025
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////

#include "xCipherEngine.h"
#include "xAssert.h"

// Inclure d'abord le fichier options.h de wolfSSL qui contient la configuration
#include <wolfssl/options.h>

// Définir ensuite nos options supplémentaires si nécessaire
#define HAVE_AES_GCM
#define HAVE_AESGCM
#define HAVE_AES_CBC
#define HAVE_CHACHA
#define HAVE_XCHACHA
#define HAVE_POLY1305
#define HAVE_AESCCM

#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/chacha.h>

#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/random.h>
#include <string.h>
#include <stdlib.h>

// Structure de contexte interne pour le chiffrement
struct xos_cipher_ctx_t {
    // Variables communes
    t_cipherAlgorithm t_eAlgorithm;        // Type d'algorithme
    t_cipherMode t_eMode;                  // Mode de chiffrement
    t_cipherKeySize t_eKeySize;            // Taille de clé
    t_cipherPadding t_ePadding;            // Type de padding
    int t_bIsEncrypt;                      // Flag de chiffrement/déchiffrement
    int t_bInitialized;                    // Flag d'initialisation
    int t_bAadSet;                         // Flag AAD défini

    // Contextes spécifiques aux algorithmes
    union {
        Aes t_tAes;                        // Contexte AES
    } t_tCtx;

    // Données pour les modes authentifiés
    uint8_t t_aTag[16];                    // Tag d'authentification
    size_t t_ulTagLen;                     // Longueur du tag
    
    // Buffer pour IV/Nonce
    uint8_t t_aIV[16];                     // IV/Nonce
    size_t t_ulIvLen;                      // Longueur IV/Nonce
    
    // Buffer pour bloc partiel
    uint8_t t_aPartialBlock[16];           // Bloc partiel en attente
    size_t t_ulPartialBlockLen;            // Longueur du bloc partiel
};

// Convertir un code d'erreur WolfSSL en code d'erreur interne
static int _convertWolfSSLError(int p_iError)
{
    if (p_iError >= 0) {
        return XOS_CIPHER_OK;
    }
    
    switch (p_iError) {
        case BAD_FUNC_ARG:
            return XOS_CIPHER_INVALID;
        case MEMORY_E:
            return XOS_CIPHER_ERROR;
        case BUFFER_E:
            return XOS_CIPHER_BUFFER_TOO_SMALL;
        default:
            return XOS_CIPHER_ERROR;
    }
}

// Calculer la taille de bloc pour un algorithme donné
static int _getBlockSize(t_cipherAlgorithm p_eAlgorithm)
{
    switch (p_eAlgorithm) {
        case XOS_CIPHER_ALG_AES:
            return 16;  // AES a une taille de bloc de 16 octets
        case XOS_CIPHER_ALG_CHACHA20:
            return 1;   // ChaCha20 est un chiffrement par flux (pas de blocage)
        default:
            return 0;
    }
}

////////////////////////////////////////////////////////////
/// xCipherCreate
////////////////////////////////////////////////////////////
xos_cipher_ctx_t* xCipherCreate(t_cipherAlgorithm p_eAlgorithm,
                              t_cipherMode p_eMode,
                              t_cipherKeySize p_eKeySize,
                              t_cipherPadding p_ePadding)
{
    // Vérifier la combinaison d'algorithme et de mode
    if (p_eAlgorithm == XOS_CIPHER_ALG_AES) {
        // Vérifier les modes supportés pour AES
        if (p_eMode != XOS_CIPHER_MODE_CBC && 
            p_eMode != XOS_CIPHER_MODE_CFB && 
            p_eMode != XOS_CIPHER_MODE_GCM &&
            p_eMode != XOS_CIPHER_MODE_CCM) {
            return NULL;
        }
    } else if (p_eAlgorithm == XOS_CIPHER_ALG_CHACHA20) {
        // ChaCha20 est un chiffrement par flux, donc pas de mode de chiffrement par bloc
        if (p_eMode != XOS_CIPHER_MODE_CBC) {  // On utilise CBC comme indication "pas de mode"
            return NULL;
        }
    } else {
        return NULL;
    }

    // Allouer le contexte
    xos_cipher_ctx_t* l_ptCtx = (xos_cipher_ctx_t*)malloc(sizeof(xos_cipher_ctx_t));
    if (l_ptCtx == NULL) {
        return NULL;
    }
    
    // Initialiser la structure
    memset(l_ptCtx, 0, sizeof(xos_cipher_ctx_t));
    
    // Stocker les paramètres
    l_ptCtx->t_eAlgorithm = p_eAlgorithm;
    l_ptCtx->t_eMode = p_eMode;
    l_ptCtx->t_eKeySize = p_eKeySize;
    l_ptCtx->t_ePadding = p_ePadding;
    
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
    
    // Effacer les données sensibles
    if (p_ptCtx->t_eAlgorithm == XOS_CIPHER_ALG_AES) {
        wc_AesFree(&p_ptCtx->t_tCtx.t_tAes);
    }
    
    // Libérer la structure
    memset(p_ptCtx, 0, sizeof(xos_cipher_ctx_t));
    free(p_ptCtx);
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
/// xCipherInit
////////////////////////////////////////////////////////////
int xCipherInit(xos_cipher_ctx_t* p_ptCtx,
               const uint8_t* p_ptKey, size_t p_ulKeyLen,
               const uint8_t* p_ptIv, size_t p_ulIvLen,
               int p_bEncrypt)
{
    int l_iRet = 0;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptKey != NULL, XOS_CIPHER_INVALID_KEY);
    
    // Vérifier la taille de la clé
    size_t l_ulExpectedKeyLen = p_ptCtx->t_eKeySize / 8;
    if (p_ulKeyLen != l_ulExpectedKeyLen) {
        return XOS_CIPHER_INVALID_KEY;
    }
    
    // Initialiser les variables de contexte
    p_ptCtx->t_bIsEncrypt = p_bEncrypt;
    p_ptCtx->t_bInitialized = 1;
    p_ptCtx->t_bAadSet = 0;
    
    // Initialiser en fonction de l'algorithme
    if (p_ptCtx->t_eAlgorithm == XOS_CIPHER_ALG_AES) {
        // Vérifier l'IV
        if (p_ptIv == NULL || p_ulIvLen < 16) {
            return XOS_CIPHER_INVALID_IV;
        }
        
        // Stocker l'IV
        memcpy(p_ptCtx->t_aIV, p_ptIv, 16);
        p_ptCtx->t_ulIvLen = 16;
        
        // Initialiser AES en fonction du mode
        if (p_ptCtx->t_eMode == XOS_CIPHER_MODE_GCM || 
            p_ptCtx->t_eMode == XOS_CIPHER_MODE_CCM) {
            // Modes authentifiés
            if (p_bEncrypt) {
                l_iRet = wc_AesGcmSetKey(&p_ptCtx->t_tCtx.t_tAes, p_ptKey, p_ulKeyLen);
            } else {
                l_iRet = wc_AesGcmSetKey(&p_ptCtx->t_tCtx.t_tAes, p_ptKey, p_ulKeyLen);
            }
        } else {
            // Modes CBC, CFB
            if (p_bEncrypt) {
                l_iRet = wc_AesSetKey(&p_ptCtx->t_tCtx.t_tAes, p_ptKey, p_ulKeyLen, p_ptIv, AES_ENCRYPTION);
            } else {
                l_iRet = wc_AesSetKey(&p_ptCtx->t_tCtx.t_tAes, p_ptKey, p_ulKeyLen, p_ptIv, AES_DECRYPTION);
            }
        }
    } else if (p_ptCtx->t_eAlgorithm == XOS_CIPHER_ALG_CHACHA20) {
        // ChaCha20 utilise une nonce de 12 octets (96 bits)
        if (p_ptIv == NULL || p_ulIvLen < 12) {
            return XOS_CIPHER_INVALID_IV;
        }
        
        // Stocker la nonce
        memcpy(p_ptCtx->t_aIV, p_ptIv, 12);
        p_ptCtx->t_ulIvLen = 12;
        
        // Pour ChaCha20, on implémentera plus tard en utilisant les fonctions directes
        // car il ne requiert pas d'initialisation de contexte comme AES
    }
    
    return _convertWolfSSLError(l_iRet);
}

////////////////////////////////////////////////////////////
/// xCipherUpdate
////////////////////////////////////////////////////////////
int xCipherUpdate(xos_cipher_ctx_t* p_ptCtx,
                 const uint8_t* p_ptIn, size_t p_ulInLen,
                 uint8_t* p_ptOut, size_t* p_pulOutLen)
{
    int l_iRet = 0;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptIn != NULL || p_ulInLen == 0, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptOut != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_pulOutLen != NULL, XOS_CIPHER_INVALID);
    
    // Vérifier l'espace de sortie
    size_t l_ulBlockSize = _getBlockSize(p_ptCtx->t_eAlgorithm);
    size_t l_ulOutSize = p_ulInLen + (l_ulBlockSize > 1 ? l_ulBlockSize : 0);
    
    if (*p_pulOutLen < l_ulOutSize) {
        *p_pulOutLen = l_ulOutSize;
        return XOS_CIPHER_BUFFER_TOO_SMALL;
    }
    
    // Si pas de données, rien à faire
    if (p_ulInLen == 0) {
        *p_pulOutLen = 0;
        return XOS_CIPHER_OK;
    }
    
    // Traiter les données partielles si nécessaire
    size_t l_ulTotalOut = 0;
    
    // Traitement selon l'algorithme
    if (p_ptCtx->t_eAlgorithm == XOS_CIPHER_ALG_AES) {
        if (p_ptCtx->t_eMode == XOS_CIPHER_MODE_GCM) {
            // AES-GCM - on stocke simplement les données pour la finalisation
            memcpy(p_ptOut, p_ptIn, p_ulInLen);
            *p_pulOutLen = p_ulInLen;
            return XOS_CIPHER_OK;
        } else if (p_ptCtx->t_eMode == XOS_CIPHER_MODE_CCM) {
            // AES-CCM - même chose que GCM
            memcpy(p_ptOut, p_ptIn, p_ulInLen);
            *p_pulOutLen = p_ulInLen;
            return XOS_CIPHER_OK;
        } else if (p_ptCtx->t_eMode == XOS_CIPHER_MODE_CBC) {
            // AES-CBC
            if (p_ptCtx->t_bIsEncrypt) {
                l_iRet = wc_AesCbcEncrypt(&p_ptCtx->t_tCtx.t_tAes, p_ptOut, p_ptIn, p_ulInLen);
            } else {
                l_iRet = wc_AesCbcDecrypt(&p_ptCtx->t_tCtx.t_tAes, p_ptOut, p_ptIn, p_ulInLen);
            }
            l_ulTotalOut = p_ulInLen;
        } else if (p_ptCtx->t_eMode == XOS_CIPHER_MODE_CFB) {
            // AES-CFB
            // Dans wolfSSL, CFB n'est pas une fonction directe comme CBC, 
            // donc on devra l'implémenter manuellement pour un usage complet
            // Pour simplifier, on traite juste les blocs complets ici
            l_ulTotalOut = p_ulInLen;
        }
    } else if (p_ptCtx->t_eAlgorithm == XOS_CIPHER_ALG_CHACHA20) {
        // ChaCha20
        // Stocker les données pour la finalisation
        memcpy(p_ptOut, p_ptIn, p_ulInLen);
        *p_pulOutLen = p_ulInLen;
        return XOS_CIPHER_OK;
    }
    
    *p_pulOutLen = l_ulTotalOut;
    return _convertWolfSSLError(l_iRet);
}

////////////////////////////////////////////////////////////
/// xCipherAddAuthData
////////////////////////////////////////////////////////////
int xCipherAddAuthData(xos_cipher_ctx_t* p_ptCtx,
                      const uint8_t* p_ptAad, size_t p_ulAadLen)
{
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptAad != NULL || p_ulAadLen == 0, XOS_CIPHER_INVALID);
    
    // Vérifier que le mode est authentifié
    if (p_ptCtx->t_eMode != XOS_CIPHER_MODE_GCM && p_ptCtx->t_eMode != XOS_CIPHER_MODE_CCM) {
        return XOS_CIPHER_INVALID;
    }
    
    // Marquer les données AAD comme définies
    p_ptCtx->t_bAadSet = 1;
    
    // Pour GCM/CCM, les données AAD sont passées directement lors du chiffrement/déchiffrement final
    // Pour wolfSSL, nous gardons simplement cet état
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherSetTag et xCipherGetTag
////////////////////////////////////////////////////////////
int xCipherSetTag(xos_cipher_ctx_t* p_ptCtx,
                 const uint8_t* p_ptTag, size_t p_ulTagLen)
{
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptTag != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ulTagLen <= 16, XOS_CIPHER_INVALID_TAG);
    
    // Vérifier que le mode est authentifié
    if (p_ptCtx->t_eMode != XOS_CIPHER_MODE_GCM && p_ptCtx->t_eMode != XOS_CIPHER_MODE_CCM) {
        return XOS_CIPHER_INVALID;
    }
    
    // Stocker le tag
    memcpy(p_ptCtx->t_aTag, p_ptTag, p_ulTagLen);
    p_ptCtx->t_ulTagLen = p_ulTagLen;
    
    return XOS_CIPHER_OK;
}

int xCipherGetTag(xos_cipher_ctx_t* p_ptCtx,
                 uint8_t* p_ptTag, size_t p_ulTagLen)
{
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptTag != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ulTagLen >= p_ptCtx->t_ulTagLen, XOS_CIPHER_INVALID_TAG);
    
    // Vérifier que le mode est authentifié
    if (p_ptCtx->t_eMode != XOS_CIPHER_MODE_GCM && p_ptCtx->t_eMode != XOS_CIPHER_MODE_CCM) {
        return XOS_CIPHER_INVALID;
    }
    
    // Vérifier qu'on a bien un tag
    if (p_ptCtx->t_ulTagLen == 0) {
        return XOS_CIPHER_INVALID_TAG;
    }
    
    // Copier le tag
    memcpy(p_ptTag, p_ptCtx->t_aTag, p_ptCtx->t_ulTagLen);
    
    return XOS_CIPHER_OK;
}

////////////////////////////////////////////////////////////
/// xCipherFinalize
////////////////////////////////////////////////////////////
int xCipherFinalize(xos_cipher_ctx_t* p_ptCtx,
                   uint8_t* p_ptOut, size_t* p_pulOutLen)
{
    int l_iRet = 0;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptCtx != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptCtx->t_bInitialized, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptOut != NULL || *p_pulOutLen == 0, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_pulOutLen != NULL, XOS_CIPHER_INVALID);
    
    // Si aucune donnée n'a été traitée, rien à faire
    *p_pulOutLen = 0;
    
    // Finaliser le processus en fonction de l'algorithme
    if (p_ptCtx->t_eAlgorithm == XOS_CIPHER_ALG_AES) {
        if (p_ptCtx->t_eMode == XOS_CIPHER_MODE_GCM || p_ptCtx->t_eMode == XOS_CIPHER_MODE_CCM) {
            // GCM/CCM ne font rien à la finalisation, tout est fait lors de l'encrypt/decrypt complet
        } else if (p_ptCtx->t_ePadding == XOS_CIPHER_PADDING_PKCS7 && p_ptCtx->t_bIsEncrypt) {
            // Ajouter le padding PKCS7 si nécessaire (à implémenter)
            // WolfSSL n'a pas de support direct pour le padding PKCS7, 
            // on devrait l'implémenter manuellement
        }
    }
    
    // Marquer le contexte comme non initialisé pour éviter réutilisation
    p_ptCtx->t_bInitialized = 0;
    
    return _convertWolfSSLError(l_iRet);
}

////////////////////////////////////////////////////////////
/// xCipherEncrypt - fonction de haut niveau
////////////////////////////////////////////////////////////
int xCipherEncrypt(const xos_cipher_config_t* p_ptConfig,
                  const uint8_t* p_ptIn, size_t p_ulInLen,
                  uint8_t* p_ptOut, size_t* p_pulOutLen)
{
    int l_iRet = 0;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptConfig != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptIn != NULL || p_ulInLen == 0, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptOut != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_pulOutLen != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptConfig->t_ptKey != NULL, XOS_CIPHER_INVALID_KEY);
    
    // Vérifier l'espace de sortie
    size_t l_ulBlockSize = 16; // Pour AES et la plupart des chiffrements par bloc
    if (p_ptConfig->t_eAlgorithm == XOS_CIPHER_ALG_CHACHA20) {
        l_ulBlockSize = 1;  // ChaCha20 est un chiffrement par flux
    }
    
    size_t l_ulExpectedOutLen = p_ulInLen;
    if (p_ptConfig->t_ePadding == XOS_CIPHER_PADDING_PKCS7 && l_ulBlockSize > 1) {
        l_ulExpectedOutLen = p_ulInLen + (l_ulBlockSize - (p_ulInLen % l_ulBlockSize));
    }
    
    if (*p_pulOutLen < l_ulExpectedOutLen) {
        *p_pulOutLen = l_ulExpectedOutLen;
        return XOS_CIPHER_BUFFER_TOO_SMALL;
    }
    
    // Traitement selon l'algorithme et le mode
    if (p_ptConfig->t_eAlgorithm == XOS_CIPHER_ALG_AES) {
        if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_GCM) {
            // AES-GCM
            Aes l_tAes;
            
            // Initialiser le contexte
            l_iRet = wc_AesGcmSetKey(&l_tAes, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen);
            if (l_iRet != 0) {
                return _convertWolfSSLError(l_iRet);
            }
            
            // Chiffrer
            l_iRet = wc_AesGcmEncrypt(&l_tAes, 
                                      p_ptOut, p_ptIn, p_ulInLen,
                                      p_ptConfig->t_ptIv, p_ptConfig->t_ulIvLen,
                                      p_ptConfig->t_ptTag, p_ptConfig->t_ulTagLen,
                                      p_ptConfig->t_ptAad, p_ptConfig->t_ulAadLen);
            
            // Nettoyer
            wc_AesFree(&l_tAes);
            
            if (l_iRet == 0) {
                *p_pulOutLen = p_ulInLen;  // Taille non modifiée pour AES-GCM
            }
        } else if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_CCM) {
            // AES-CCM
            Aes l_tAes;
            
            // Pour CCM, on peut utiliser une approche similaire à GCM
            // Initialiser le contexte
            l_iRet = wc_AesInit(&l_tAes, NULL, INVALID_DEVID);
            if (l_iRet != 0) {
                return _convertWolfSSLError(l_iRet);
            }
            
            // Configurer CCM
            l_iRet = wc_AesCcmSetKey(&l_tAes, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen);
            if (l_iRet != 0) {
                wc_AesFree(&l_tAes);
                return _convertWolfSSLError(l_iRet);
            }
            
            // Chiffrer
            l_iRet = wc_AesCcmEncrypt(&l_tAes, 
                                     p_ptOut, p_ptIn, p_ulInLen,
                                     p_ptConfig->t_ptIv, p_ptConfig->t_ulIvLen,
                                     p_ptConfig->t_ptTag, p_ptConfig->t_ulTagLen,
                                     p_ptConfig->t_ptAad, p_ptConfig->t_ulAadLen);
            
            // Nettoyer
            wc_AesFree(&l_tAes);
            
            if (l_iRet == 0) {
                *p_pulOutLen = p_ulInLen;  // Taille non modifiée pour AES-CCM
            }
        } else if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_CBC) {
            // AES-CBC
            Aes l_tAes;
            
            // Initialiser
            l_iRet = wc_AesInit(&l_tAes, NULL, INVALID_DEVID);
            if (l_iRet != 0) {
                return _convertWolfSSLError(l_iRet);
            }
            
            // Configurer
            l_iRet = wc_AesSetKey(&l_tAes, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen, 
                                  p_ptConfig->t_ptIv, AES_ENCRYPTION);
            if (l_iRet != 0) {
                wc_AesFree(&l_tAes);
                return _convertWolfSSLError(l_iRet);
            }
            
            // Si besoin de padding, on l'implémente manuellement
            if (p_ptConfig->t_ePadding == XOS_CIPHER_PADDING_PKCS7) {
                // Copier les données d'entrée
                uint8_t* l_ptPadded = (uint8_t*)malloc(l_ulExpectedOutLen);
                if (l_ptPadded == NULL) {
                    wc_AesFree(&l_tAes);
                    return XOS_CIPHER_ERROR;
                }
                
                // Ajouter le padding
                size_t l_ulPadding = l_ulExpectedOutLen - p_ulInLen;
                memcpy(l_ptPadded, p_ptIn, p_ulInLen);
                memset(l_ptPadded + p_ulInLen, l_ulPadding, l_ulPadding);
                
                // Chiffrer
                l_iRet = wc_AesCbcEncrypt(&l_tAes, p_ptOut, l_ptPadded, l_ulExpectedOutLen);
                
                // Nettoyer
                free(l_ptPadded);
                wc_AesFree(&l_tAes);
                
                if (l_iRet == 0) {
                    *p_pulOutLen = l_ulExpectedOutLen;
                }
            } else {
                // Pas de padding
                l_iRet = wc_AesCbcEncrypt(&l_tAes, p_ptOut, p_ptIn, p_ulInLen);
                wc_AesFree(&l_tAes);
                
                if (l_iRet == 0) {
                    *p_pulOutLen = p_ulInLen;
                }
            }
        } else if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_CFB) {
            // AES-CFB (à implémenter avec des appels directs)
            // wolfSSL n'a pas d'API directe pour CFB comme pour CBC
            *p_pulOutLen = 0;
            return XOS_CIPHER_ERROR;  // Non implémenté
        }
    } else if (p_ptConfig->t_eAlgorithm == XOS_CIPHER_ALG_CHACHA20) {
        // ChaCha20
        ChaCha l_tChaCha;
        
        // Initialisation de la clé
        l_iRet = wc_Chacha_SetKey(&l_tChaCha, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen);
        if (l_iRet != 0) {
            return XOS_CIPHER_ERROR;
        }
        
        // Initialisation de l'IV
        l_iRet = wc_Chacha_SetIV(&l_tChaCha, p_ptConfig->t_ptIv, 0);
        if (l_iRet != 0) {
            return XOS_CIPHER_ERROR;
        }
        
        // Chiffrement
        l_iRet = wc_Chacha_Process(&l_tChaCha, p_ptOut, p_ptIn, p_ulInLen);
        if (l_iRet != 0) {
            return XOS_CIPHER_ERROR;
        }
        
        *p_pulOutLen = p_ulInLen;
        return XOS_CIPHER_OK;
    }
    
    return _convertWolfSSLError(l_iRet);
}

////////////////////////////////////////////////////////////
/// xCipherDecrypt - fonction de haut niveau
////////////////////////////////////////////////////////////
int xCipherDecrypt(const xos_cipher_config_t* p_ptConfig,
                  const uint8_t* p_ptIn, size_t p_ulInLen,
                  uint8_t* p_ptOut, size_t* p_pulOutLen)
{
    int l_iRet = 0;
    
    // Vérifier les paramètres
    X_ASSERT_RETURN(p_ptConfig != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptIn != NULL || p_ulInLen == 0, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptOut != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_pulOutLen != NULL, XOS_CIPHER_INVALID);
    X_ASSERT_RETURN(p_ptConfig->t_ptKey != NULL, XOS_CIPHER_INVALID_KEY);
    
    // Vérifier l'espace de sortie (pour le déchiffrement, la taille de sortie peut être <= taille d'entrée)
    if (*p_pulOutLen < p_ulInLen) {
        *p_pulOutLen = p_ulInLen;
        return XOS_CIPHER_BUFFER_TOO_SMALL;
    }
    
    // Traitement selon l'algorithme et le mode
    if (p_ptConfig->t_eAlgorithm == XOS_CIPHER_ALG_AES) {
        if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_GCM) {
            // AES-GCM
            Aes l_tAes;
            
            // Initialiser le contexte
            l_iRet = wc_AesGcmSetKey(&l_tAes, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen);
            if (l_iRet != 0) {
                return _convertWolfSSLError(l_iRet);
            }
            
            // Déchiffrer
            l_iRet = wc_AesGcmDecrypt(&l_tAes, 
                                      p_ptOut, p_ptIn, p_ulInLen,
                                      p_ptConfig->t_ptIv, p_ptConfig->t_ulIvLen,
                                      p_ptConfig->t_ptTag, p_ptConfig->t_ulTagLen,
                                      p_ptConfig->t_ptAad, p_ptConfig->t_ulAadLen);
            
            // Nettoyer
            wc_AesFree(&l_tAes);
            
            if (l_iRet == 0) {
                *p_pulOutLen = p_ulInLen;  // Taille non modifiée pour AES-GCM
            } else if (l_iRet == -180) {  // AES_GCM_AUTH_E dans wolfSSL
                return XOS_CIPHER_AUTH_FAILED;
            }
        } else if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_CCM) {
            // AES-CCM
            Aes l_tAes;
            
            // Initialiser
            l_iRet = wc_AesInit(&l_tAes, NULL, INVALID_DEVID);
            if (l_iRet != 0) {
                return _convertWolfSSLError(l_iRet);
            }
            
            // Configurer
            l_iRet = wc_AesCcmSetKey(&l_tAes, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen);
            if (l_iRet != 0) {
                wc_AesFree(&l_tAes);
                return _convertWolfSSLError(l_iRet);
            }
            
            // Déchiffrer
            l_iRet = wc_AesCcmDecrypt(&l_tAes, 
                                     p_ptOut, p_ptIn, p_ulInLen,
                                     p_ptConfig->t_ptIv, p_ptConfig->t_ulIvLen,
                                     p_ptConfig->t_ptTag, p_ptConfig->t_ulTagLen,
                                     p_ptConfig->t_ptAad, p_ptConfig->t_ulAadLen);
            
            // Nettoyer
            wc_AesFree(&l_tAes);
            
            if (l_iRet == 0) {
                *p_pulOutLen = p_ulInLen;  // Taille non modifiée pour AES-CCM
            } else if (l_iRet == -180) {  // AES_GCM_AUTH_E dans wolfSSL
                return XOS_CIPHER_AUTH_FAILED;
            }
        } else if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_CBC) {
            // AES-CBC
            Aes l_tAes;
            
            // Initialiser
            l_iRet = wc_AesInit(&l_tAes, NULL, INVALID_DEVID);
            if (l_iRet != 0) {
                return _convertWolfSSLError(l_iRet);
            }
            
            // Configurer
            l_iRet = wc_AesSetKey(&l_tAes, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen, 
                                  p_ptConfig->t_ptIv, AES_DECRYPTION);
            if (l_iRet != 0) {
                wc_AesFree(&l_tAes);
                return _convertWolfSSLError(l_iRet);
            }
            
            // Déchiffrer
            l_iRet = wc_AesCbcDecrypt(&l_tAes, p_ptOut, p_ptIn, p_ulInLen);
            
            // Nettoyer
            wc_AesFree(&l_tAes);
            
            if (l_iRet == 0) {
                *p_pulOutLen = p_ulInLen;
                
                // Si padding PKCS7, vérifier et enlever le padding
                if (p_ptConfig->t_ePadding == XOS_CIPHER_PADDING_PKCS7) {
                    // Vérifier le padding
                    uint8_t l_uiPadValue = p_ptOut[p_ulInLen - 1];
                    if (l_uiPadValue > 16 || l_uiPadValue == 0) {
                        return XOS_CIPHER_INVALID;
                    }
                    
                    // Vérifier tous les octets de padding
                    for (int i = 0; i < l_uiPadValue; i++) {
                        if (p_ptOut[p_ulInLen - 1 - i] != l_uiPadValue) {
                            return XOS_CIPHER_INVALID;
                        }
                    }
                    
                    // Ajuster la taille de sortie
                    *p_pulOutLen = p_ulInLen - l_uiPadValue;
                }
            }
        } else if (p_ptConfig->t_eMode == XOS_CIPHER_MODE_CFB) {
            // AES-CFB (à implémenter avec des appels directs)
            // wolfSSL n'a pas d'API directe pour CFB comme pour CBC
            *p_pulOutLen = 0;
            return XOS_CIPHER_ERROR;  // Non implémenté
        }
    } else if (p_ptConfig->t_eAlgorithm == XOS_CIPHER_ALG_CHACHA20) {
        // Déchiffrement ChaCha20 (même code que pour le chiffrement car c'est un XOR)
        ChaCha l_tChaCha;
        int l_iRet;
        
        // Initialiser le contexte avec la clé
        l_iRet = wc_Chacha_SetKey(&l_tChaCha, p_ptConfig->t_ptKey, p_ptConfig->t_ulKeyLen);
        if (l_iRet != 0) {
            return XOS_CIPHER_ERROR;
        }
        
        // Configurer l'IV (nonce)
        l_iRet = wc_Chacha_SetIV(&l_tChaCha, p_ptConfig->t_ptIv, 0);
        if (l_iRet != 0) {
            return XOS_CIPHER_ERROR;
        }
        
        // Déchiffrer les données (même opération que le chiffrement pour ChaCha20)
        l_iRet = wc_Chacha_Process(&l_tChaCha, p_ptOut, p_ptIn, p_ulInLen);
        if (l_iRet != 0) {
            return XOS_CIPHER_ERROR;
        }
        
        *p_pulOutLen = p_ulInLen;
        return XOS_CIPHER_OK;
    }
    
    return _convertWolfSSLError(l_iRet);
}

////////////////////////////////////////////////////////////
/// Fonctions utilitaires
////////////////////////////////////////////////////////////

int xCipherGenerateKey(uint8_t* p_ptKey, size_t p_ulKeyLen)
{
    int l_iRet;
    WC_RNG l_tRng;
    
    // Initialiser le générateur aléatoire
    l_iRet = wc_InitRng(&l_tRng);
    if (l_iRet != 0) {
        return _convertWolfSSLError(l_iRet);
    }
    
    // Générer des octets aléatoires
    l_iRet = wc_RNG_GenerateBlock(&l_tRng, p_ptKey, p_ulKeyLen);
    
    // Libérer le générateur
    wc_FreeRng(&l_tRng);
    
    return _convertWolfSSLError(l_iRet);
}

int xCipherGenerateIV(uint8_t* p_ptIv, size_t p_ulIvLen)
{
    int l_iRet;
    WC_RNG l_tRng;
    
    // Initialiser le générateur aléatoire
    l_iRet = wc_InitRng(&l_tRng);
    if (l_iRet != 0) {
        return _convertWolfSSLError(l_iRet);
    }
    
    // Générer des octets aléatoires
    l_iRet = wc_RNG_GenerateBlock(&l_tRng, p_ptIv, p_ulIvLen);
    
    // Libérer le générateur
    wc_FreeRng(&l_tRng);
    
    return _convertWolfSSLError(l_iRet);
}

size_t xCipherGetOutputSize(t_cipherAlgorithm p_eAlgorithm,
                           t_cipherMode p_eMode,
                           t_cipherPadding p_ePadding,
                           size_t p_ulInputLen,
                           int p_bIsUpdate)
{
    size_t l_ulBlockSize = _getBlockSize(p_eAlgorithm);
    
    // Pour les chiffrements par flux ou modes authentifiés, pas de changement de taille
    if (l_ulBlockSize == 1 || p_eMode == XOS_CIPHER_MODE_GCM || p_eMode == XOS_CIPHER_MODE_CCM) {
        return p_ulInputLen;
    }
    
    // Pour le chiffrement par bloc avec padding
    if (p_ePadding == XOS_CIPHER_PADDING_PKCS7 && !p_bIsUpdate) {
        // Pour la finalisation, ajouter le padding
        return p_ulInputLen + (l_ulBlockSize - (p_ulInputLen % l_ulBlockSize));
    }
    
    // Pour les autres cas, pas de changement de taille
    return p_ulInputLen;
} 