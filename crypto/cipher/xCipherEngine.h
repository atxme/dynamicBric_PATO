////////////////////////////////////////////////////////////
//  cipher engine header file
//  defines the cipher engine types and functions using WolfSSL
//
// general discloser: copy or share the file is forbidden
// Written : 30/03/2025
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////
#pragma once

#ifndef XOS_CIPHER_ENGINE_H_
#define XOS_CIPHER_ENGINE_H_

#include <stdint.h>
#include <stddef.h>

// Cipher error codes
#define XOS_CIPHER_OK                 0
#define XOS_CIPHER_ERROR             -1
#define XOS_CIPHER_INVALID           -2
#define XOS_CIPHER_INVALID_KEY       -3
#define XOS_CIPHER_INVALID_IV        -4
#define XOS_CIPHER_INVALID_TAG       -5
#define XOS_CIPHER_INVALID_AAD       -6
#define XOS_CIPHER_AUTH_FAILED       -7
#define XOS_CIPHER_BUFFER_TOO_SMALL  -8

// Cipher modes
typedef enum {
    XOS_CIPHER_MODE_CBC,      // Cipher Block Chaining
    XOS_CIPHER_MODE_CFB,      // Cipher Feedback
    XOS_CIPHER_MODE_GCM,      // Galois/Counter Mode (authentifié)
    XOS_CIPHER_MODE_CCM       // Counter with CBC-MAC (authentifié)
} t_cipherMode;

// Cipher algorithms
typedef enum {
    XOS_CIPHER_ALG_AES,       // Advanced Encryption Standard
    XOS_CIPHER_ALG_CHACHA20   // ChaCha20
} t_cipherAlgorithm;

// Cipher key sizes (en bits)
typedef enum {
    XOS_CIPHER_KEY_128 = 128,
    XOS_CIPHER_KEY_192 = 192,
    XOS_CIPHER_KEY_256 = 256
} t_cipherKeySize;

// Cipher padding modes
typedef enum {
    XOS_CIPHER_PADDING_NONE,  // Pas de padding
    XOS_CIPHER_PADDING_PKCS7  // PKCS#7 padding
} t_cipherPadding;

// Structure de contexte cipher (opaque)
typedef struct xos_cipher_ctx_t xos_cipher_ctx_t;

// Structure de configuration pour le chiffrement/déchiffrement
typedef struct {
    t_cipherAlgorithm t_eAlgorithm;    // Algorithme de chiffrement
    t_cipherMode t_eMode;              // Mode de chiffrement
    t_cipherKeySize t_eKeySize;        // Taille de clé
    t_cipherPadding t_ePadding;        // Type de padding
    const uint8_t* t_ptKey;            // Clé de chiffrement
    size_t t_ulKeyLen;                 // Longueur de la clé en octets
    const uint8_t* t_ptIv;             // Vecteur d'initialisation
    size_t t_ulIvLen;                  // Longueur du vecteur d'initialisation
    const uint8_t* t_ptAad;            // Données authentifiées (pour modes GCM/CCM)
    size_t t_ulAadLen;                 // Longueur des données authentifiées
    uint8_t* t_ptTag;                  // Tag d'authentification (pour modes GCM/CCM)
    size_t t_ulTagLen;                 // Longueur du tag
} xos_cipher_config_t;

//////////////////////////////////
/// @brief Initialiser le contexte de chiffrement
/// @param p_eAlgorithm : algorithme de chiffrement
/// @param p_eMode : mode de chiffrement
/// @param p_eKeySize : taille de clé
/// @param p_ePadding : type de padding
/// @return : contexte de chiffrement ou NULL si erreur
//////////////////////////////////
xos_cipher_ctx_t* xCipherCreate(t_cipherAlgorithm p_eAlgorithm,
                              t_cipherMode p_eMode,
                              t_cipherKeySize p_eKeySize,
                              t_cipherPadding p_ePadding);

//////////////////////////////////
/// @brief Libérer le contexte de chiffrement
/// @param p_ptCtx : contexte de chiffrement
//////////////////////////////////
void xCipherDestroy(xos_cipher_ctx_t* p_ptCtx);

//////////////////////////////////
/// @brief Initialiser le chiffrement avec clé et vecteur d'initialisation
/// @param p_ptCtx : contexte de chiffrement
/// @param p_ptKey : clé de chiffrement
/// @param p_ulKeyLen : longueur de la clé en octets
/// @param p_ptIv : vecteur d'initialisation (NULL si non applicable)
/// @param p_ulIvLen : longueur du vecteur d'initialisation en octets
/// @param p_bEncrypt : true pour chiffrer, false pour déchiffrer
/// @return : code d'erreur
//////////////////////////////////
int xCipherInit(xos_cipher_ctx_t* p_ptCtx,
               const uint8_t* p_ptKey, size_t p_ulKeyLen,
               const uint8_t* p_ptIv, size_t p_ulIvLen,
               int p_bEncrypt);

//////////////////////////////////
/// @brief Mise à jour du chiffrement avec bloc de données
/// @param p_ptCtx : contexte de chiffrement
/// @param p_ptIn : données en entrée
/// @param p_ulInLen : longueur des données en entrée
/// @param p_ptOut : tampon de sortie
/// @param p_pulOutLen : pointeur vers la variable contenant la taille du tampon de sortie,
///                     mise à jour avec la taille réelle écrite
/// @return : code d'erreur
//////////////////////////////////
int xCipherUpdate(xos_cipher_ctx_t* p_ptCtx,
                 const uint8_t* p_ptIn, size_t p_ulInLen,
                 uint8_t* p_ptOut, size_t* p_pulOutLen);

//////////////////////////////////
/// @brief Finaliser le chiffrement
/// @param p_ptCtx : contexte de chiffrement
/// @param p_ptOut : tampon de sortie
/// @param p_pulOutLen : pointeur vers la variable contenant la taille du tampon de sortie,
///                     mise à jour avec la taille réelle écrite
/// @return : code d'erreur
//////////////////////////////////
int xCipherFinalize(xos_cipher_ctx_t* p_ptCtx,
                   uint8_t* p_ptOut, size_t* p_pulOutLen);

//////////////////////////////////
/// @brief Ajouter des données authentifiées pour les modes authentifiés (GCM, CCM)
/// @param p_ptCtx : contexte de chiffrement
/// @param p_ptAad : données authentifiées
/// @param p_ulAadLen : longueur des données authentifiées
/// @return : code d'erreur
//////////////////////////////////
int xCipherAddAuthData(xos_cipher_ctx_t* p_ptCtx,
                      const uint8_t* p_ptAad, size_t p_ulAadLen);

//////////////////////////////////
/// @brief Définir ou vérifier le tag d'authentification pour les modes authentifiés
/// @param p_ptCtx : contexte de chiffrement
/// @param p_ptTag : tag d'authentification
/// @param p_ulTagLen : longueur du tag
/// @return : code d'erreur
//////////////////////////////////
int xCipherSetTag(xos_cipher_ctx_t* p_ptCtx,
                 const uint8_t* p_ptTag, size_t p_ulTagLen);

//////////////////////////////////
/// @brief Obtenir le tag d'authentification généré pour les modes authentifiés
/// @param p_ptCtx : contexte de chiffrement
/// @param p_ptTag : tampon pour recevoir le tag
/// @param p_ulTagLen : taille du tampon
/// @return : code d'erreur
//////////////////////////////////
int xCipherGetTag(xos_cipher_ctx_t* p_ptCtx,
                 uint8_t* p_ptTag, size_t p_ulTagLen);

//////////////////////////////////
/// @brief Initialiser une structure de configuration avec des valeurs par défaut
/// @param p_ptConfig : structure de configuration à initialiser
/// @return : code d'erreur
//////////////////////////////////
int xCipherInitConfig(xos_cipher_config_t* p_ptConfig);

//////////////////////////////////
/// @brief Chiffrer des données en une seule opération
/// @param p_ptConfig : configuration du chiffrement
/// @param p_ptIn : données à chiffrer
/// @param p_ulInLen : longueur des données à chiffrer
/// @param p_ptOut : tampon de sortie
/// @param p_pulOutLen : pointeur vers la taille du tampon, mis à jour avec la taille réelle
/// @return : code d'erreur
//////////////////////////////////
int xCipherEncrypt(const xos_cipher_config_t* p_ptConfig,
                  const uint8_t* p_ptIn, size_t p_ulInLen,
                  uint8_t* p_ptOut, size_t* p_pulOutLen);

//////////////////////////////////
/// @brief Déchiffrer des données en une seule opération
/// @param p_ptConfig : configuration du déchiffrement
/// @param p_ptIn : données à déchiffrer
/// @param p_ulInLen : longueur des données à déchiffrer
/// @param p_ptOut : tampon de sortie
/// @param p_pulOutLen : pointeur vers la taille du tampon, mis à jour avec la taille réelle
/// @return : code d'erreur
//////////////////////////////////
int xCipherDecrypt(const xos_cipher_config_t* p_ptConfig,
                  const uint8_t* p_ptIn, size_t p_ulInLen,
                  uint8_t* p_ptOut, size_t* p_pulOutLen);

//////////////////////////////////
/// @brief Générer une clé aléatoire
/// @param p_ptKey : tampon pour la clé
/// @param p_ulKeyLen : taille du tampon
/// @return : code d'erreur
//////////////////////////////////
int xCipherGenerateKey(uint8_t* p_ptKey, size_t p_ulKeyLen);

//////////////////////////////////
/// @brief Générer un vecteur d'initialisation aléatoire
/// @param p_ptIv : tampon pour l'IV
/// @param p_ulIvLen : taille du tampon
/// @return : code d'erreur
//////////////////////////////////
int xCipherGenerateIV(uint8_t* p_ptIv, size_t p_ulIvLen);

//////////////////////////////////
/// @brief Obtenir la taille de sortie maximale pour une opération donnée
/// @param p_eAlgorithm : algorithme de chiffrement
/// @param p_eMode : mode de chiffrement
/// @param p_ePadding : type de padding
/// @param p_ulInputLen : taille des données d'entrée
/// @param p_bIsUpdate : indique si c'est pour une mise à jour ou finalisation
/// @return : taille maximale nécessaire pour le tampon de sortie
//////////////////////////////////
size_t xCipherGetOutputSize(t_cipherAlgorithm p_eAlgorithm,
                           t_cipherMode p_eMode,
                           t_cipherPadding p_ePadding,
                           size_t p_ulInputLen,
                           int p_bIsUpdate);

#endif // XOS_CIPHER_ENGINE_H_ 