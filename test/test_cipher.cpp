#include <gtest/gtest.h>
#include <crypto/cipher/xCipherEngine.h>
#include <string.h>

class CipherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialiser les données de test
        memset(&config, 0, sizeof(config));
        
        // Clé de test pour AES-256
        static const uint8_t testKey[32] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
        };
        
        // IV de test pour AES
        static const uint8_t testIV[16] = {
            0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
            0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF
        };
        
        // Données de test
        static const char testPlaintext[] = "Ceci est un test de chiffrement. Nous testons le moteur de chiffrement.";
        plaintextLen = strlen(testPlaintext);
        memcpy(plaintext, testPlaintext, plaintextLen);
        
        // Configuration AES-256 GCM
        config.t_eAlgorithm = XOS_CIPHER_ALG_AES;
        config.t_eMode = XOS_CIPHER_MODE_GCM;
        config.t_eKeySize = XOS_CIPHER_KEY_256;
        config.t_ePadding = XOS_CIPHER_PAD_NONE; // GCM n'utilise pas de padding
        config.t_pKey = testKey;
        config.t_pIV = testIV;
        config.t_ulIVLen = 12; // GCM utilise habituellement un nonce de 12 octets
    }

    void TearDown() override {
        // Nettoyage
    }

    // Données de test
    xos_cipher_config_t config;
    uint8_t plaintext[256];
    uint8_t ciphertext[256 + 16]; // +16 pour le tag d'authentification
    uint8_t decrypted[256];
    unsigned long plaintextLen;
};

// Test de base pour l'initialisation et la destruction du contexte de chiffrement
TEST_F(CipherTest, ContextCreateDestroy) {
    // Créer un contexte
    xos_cipher_ctx_t* ctx = xCipherCreate();
    ASSERT_NE(ctx, nullptr);
    
    // Détruire le contexte
    xCipherDestroy(ctx);
}

// Test du chiffrement/déchiffrement AES-GCM
TEST_F(CipherTest, AES_GCM_EncryptDecrypt) {
    // Créer un contexte
    xos_cipher_ctx_t* ctx = xCipherCreate();
    ASSERT_NE(ctx, nullptr);
    
    // Chiffrer les données
    unsigned long ciphertextLen = sizeof(ciphertext);
    int result = xCipherEncrypt(ctx, &config, plaintext, plaintextLen, ciphertext, &ciphertextLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    EXPECT_GT(ciphertextLen, 0U);
    
    // Déchiffrer les données
    unsigned long decryptedLen = sizeof(decrypted);
    result = xCipherDecrypt(ctx, &config, ciphertext, ciphertextLen, decrypted, &decryptedLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    
    // Vérifier que les données déchiffrées correspondent aux données d'origine
    EXPECT_EQ(decryptedLen, plaintextLen);
    EXPECT_EQ(memcmp(plaintext, decrypted, plaintextLen), 0);
    
    // Détruire le contexte
    xCipherDestroy(ctx);
}

// Test du chiffrement/déchiffrement AES-CBC
TEST_F(CipherTest, AES_CBC_EncryptDecrypt) {
    // Modifier la configuration pour CBC
    config.t_eMode = XOS_CIPHER_MODE_CBC;
    config.t_ePadding = XOS_CIPHER_PAD_PKCS7;
    config.t_ulIVLen = 16; // IV complet pour CBC
    
    // Créer un contexte
    xos_cipher_ctx_t* ctx = xCipherCreate();
    ASSERT_NE(ctx, nullptr);
    
    // Chiffrer les données
    unsigned long ciphertextLen = sizeof(ciphertext);
    int result = xCipherEncrypt(ctx, &config, plaintext, plaintextLen, ciphertext, &ciphertextLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    EXPECT_GT(ciphertextLen, 0U);
    
    // Déchiffrer les données
    unsigned long decryptedLen = sizeof(decrypted);
    result = xCipherDecrypt(ctx, &config, ciphertext, ciphertextLen, decrypted, &decryptedLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    
    // Vérifier que les données déchiffrées correspondent aux données d'origine
    EXPECT_EQ(decryptedLen, plaintextLen);
    EXPECT_EQ(memcmp(plaintext, decrypted, plaintextLen), 0);
    
    // Détruire le contexte
    xCipherDestroy(ctx);
}

// Test du chiffrement/déchiffrement ChaCha20
TEST_F(CipherTest, ChaCha20_EncryptDecrypt) {
    // Modifier la configuration pour ChaCha20
    config.t_eAlgorithm = XOS_CIPHER_ALG_CHACHA20;
    config.t_eMode = XOS_CIPHER_MODE_STREAM;
    config.t_ePadding = XOS_CIPHER_PAD_NONE;
    config.t_ulIVLen = 12; // Nonce pour ChaCha20
    
    // Créer un contexte
    xos_cipher_ctx_t* ctx = xCipherCreate();
    ASSERT_NE(ctx, nullptr);
    
    // Chiffrer les données
    unsigned long ciphertextLen = sizeof(ciphertext);
    int result = xCipherEncrypt(ctx, &config, plaintext, plaintextLen, ciphertext, &ciphertextLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    EXPECT_GT(ciphertextLen, 0U);
    
    // Déchiffrer les données
    unsigned long decryptedLen = sizeof(decrypted);
    result = xCipherDecrypt(ctx, &config, ciphertext, ciphertextLen, decrypted, &decryptedLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    
    // Vérifier que les données déchiffrées correspondent aux données d'origine
    EXPECT_EQ(decryptedLen, plaintextLen);
    EXPECT_EQ(memcmp(plaintext, decrypted, plaintextLen), 0);
    
    // Détruire le contexte
    xCipherDestroy(ctx);
}

// Test de la fonction d'initialisation de configuration
TEST_F(CipherTest, InitConfig) {
    // Initialiser une configuration
    xos_cipher_config_t testConfig;
    xCipherInitConfig(&testConfig);
    
    // Vérifier les valeurs par défaut
    EXPECT_EQ(testConfig.t_eAlgorithm, XOS_CIPHER_ALG_AES);
    EXPECT_EQ(testConfig.t_eMode, XOS_CIPHER_MODE_GCM);
    EXPECT_EQ(testConfig.t_eKeySize, XOS_CIPHER_KEY_256);
    EXPECT_EQ(testConfig.t_ePadding, XOS_CIPHER_PAD_NONE);
    EXPECT_EQ(testConfig.t_pKey, nullptr);
    EXPECT_EQ(testConfig.t_pIV, nullptr);
    EXPECT_EQ(testConfig.t_ulIVLen, 0U);
    EXPECT_EQ(testConfig.t_pAAD, nullptr);
    EXPECT_EQ(testConfig.t_ulAADLen, 0U);
    EXPECT_EQ(testConfig.t_pTag, nullptr);
    EXPECT_EQ(testConfig.t_ulTagLen, 0U);
}

// Test de l'API détaillée (init, update, finalize)
TEST_F(CipherTest, DetailedAPI) {
    // Créer un contexte
    xos_cipher_ctx_t* ctx = xCipherCreate();
    ASSERT_NE(ctx, nullptr);
    
    // Initialiser le contexte pour le chiffrement
    int result = xCipherInit(ctx, config.t_eAlgorithm, config.t_eMode, 
                            config.t_eKeySize, config.t_ePadding,
                            config.t_pKey, config.t_pIV, config.t_ulIVLen, 1);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    
    // Chiffrer par morceaux
    unsigned long offset = 0;
    unsigned long chunkSize = 16;
    unsigned long outputLen = 0;
    
    // Traiter les données par blocs de 16 octets
    while (offset < plaintextLen) {
        unsigned long currentChunk = std::min(chunkSize, plaintextLen - offset);
        unsigned long currentOutputLen = sizeof(ciphertext) - outputLen;
        
        result = xCipherUpdate(ctx, plaintext + offset, currentChunk, 
                              ciphertext + outputLen, &currentOutputLen);
        EXPECT_EQ(result, XOS_CIPHER_OK);
        
        offset += currentChunk;
        outputLen += currentOutputLen;
    }
    
    // Finaliser le chiffrement
    unsigned long finalLen = sizeof(ciphertext) - outputLen;
    result = xCipherFinalize(ctx, ciphertext + outputLen, &finalLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    outputLen += finalLen;
    
    // Réinitialiser le contexte pour le déchiffrement
    result = xCipherInit(ctx, config.t_eAlgorithm, config.t_eMode, 
                        config.t_eKeySize, config.t_ePadding,
                        config.t_pKey, config.t_pIV, config.t_ulIVLen, 0);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    
    // Déchiffrer en une fois pour simplifier
    unsigned long decryptedLen = sizeof(decrypted);
    result = xCipherUpdate(ctx, ciphertext, outputLen, decrypted, &decryptedLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    
    // Finaliser le déchiffrement
    unsigned long finalDecLen = sizeof(decrypted) - decryptedLen;
    result = xCipherFinalize(ctx, decrypted + decryptedLen, &finalDecLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    decryptedLen += finalDecLen;
    
    // Vérifier que les données déchiffrées correspondent aux données d'origine
    EXPECT_EQ(decryptedLen, plaintextLen);
    EXPECT_EQ(memcmp(plaintext, decrypted, plaintextLen), 0);
    
    // Détruire le contexte
    xCipherDestroy(ctx);
}

// Test de la gestion des données d'authentification (AAD) pour GCM
TEST_F(CipherTest, GCM_Authentication) {
    // Créer un contexte
    xos_cipher_ctx_t* ctx = xCipherCreate();
    ASSERT_NE(ctx, nullptr);
    
    // Données d'authentification
    const uint8_t aad[] = "Données authentifiées supplémentaires";
    const unsigned long aadLen = strlen((const char*)aad);
    
    // Configurer les données d'authentification
    config.t_pAAD = aad;
    config.t_ulAADLen = aadLen;
    
    // Allouer un espace pour le tag d'authentification
    uint8_t tag[16];
    config.t_pTag = tag;
    config.t_ulTagLen = sizeof(tag);
    
    // Chiffrer les données
    unsigned long ciphertextLen = sizeof(ciphertext);
    int result = xCipherEncrypt(ctx, &config, plaintext, plaintextLen, ciphertext, &ciphertextLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    
    // Vérifier que le tag a été généré
    EXPECT_GT(config.t_ulTagLen, 0U);
    
    // Déchiffrer les données en vérifiant l'authentification
    unsigned long decryptedLen = sizeof(decrypted);
    result = xCipherDecrypt(ctx, &config, ciphertext, ciphertextLen, decrypted, &decryptedLen);
    EXPECT_EQ(result, XOS_CIPHER_OK);
    
    // Vérifier que les données déchiffrées correspondent aux données d'origine
    EXPECT_EQ(decryptedLen, plaintextLen);
    EXPECT_EQ(memcmp(plaintext, decrypted, plaintextLen), 0);
    
    // Modifier les AAD et tenter de déchiffrer à nouveau (doit échouer)
    uint8_t modifiedAAD[] = "Données authentifiées modifiées";
    config.t_pAAD = modifiedAAD;
    config.t_ulAADLen = strlen((const char*)modifiedAAD);
    
    decryptedLen = sizeof(decrypted);
    result = xCipherDecrypt(ctx, &config, ciphertext, ciphertextLen, decrypted, &decryptedLen);
    EXPECT_NE(result, XOS_CIPHER_OK);
    
    // Détruire le contexte
    xCipherDestroy(ctx);
} 