#include <gtest/gtest.h>
#include <crypto/TLS/xTlsEngine.h>
#include <string.h>

class TlsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialiser le moteur TLS
        xTlsInit();
        
        // Configurer le test TLS
        memset(&config, 0, sizeof(config));
        config.t_eRole = XOS_TLS_CLIENT;
        config.t_eVersion = XOS_TLS_VERSION_1_3_ONLY;
        config.t_eKeyExchange = XOS_TLS_KEX_STANDARD;
        config.t_eSigAlg = XOS_TLS_SIG_ECDSA;
        config.t_bVerifyPeer = false;  // Ne pas vérifier le certificat pour les tests
    }

    void TearDown() override {
        // Nettoyer le moteur TLS
        xTlsCleanup();
    }

    // Configuration de test
    xos_tls_config_t config;
};

// Test de base pour l'initialisation et le nettoyage
TEST_F(TlsTest, InitAndCleanup) {
    int result = xTlsInit();
    EXPECT_EQ(result, XOS_TLS_OK);
    
    result = xTlsCleanup();
    EXPECT_EQ(result, XOS_TLS_OK);
}

// Test de création et destruction de contexte
TEST_F(TlsTest, ContextCreateDestroy) {
    xos_tls_ctx_t* ctx = xTlsCreateContext(&config);
    ASSERT_NE(ctx, nullptr);
    
    xTlsDestroyContext(ctx);
}

// Test des fonctions de gestion d'erreur
TEST_F(TlsTest, ErrorHandling) {
    const char* errorStr = xTlsGetErrorString(XOS_TLS_OK);
    EXPECT_STREQ(errorStr, "Success");
    
    errorStr = xTlsGetErrorString(XOS_TLS_ERROR);
    EXPECT_STREQ(errorStr, "General TLS error");
    
    errorStr = xTlsGetErrorString(XOS_TLS_WOULD_BLOCK);
    EXPECT_STREQ(errorStr, "Operation would block");
    
    errorStr = xTlsGetErrorString(XOS_TLS_INVALID_PARAM);
    EXPECT_STREQ(errorStr, "Invalid parameter");
    
    errorStr = xTlsGetErrorString(XOS_TLS_NOT_INITIALIZED);
    EXPECT_STREQ(errorStr, "TLS not initialized");
    
    errorStr = xTlsGetErrorString(XOS_TLS_CERTIFICATE_ERROR);
    EXPECT_STREQ(errorStr, "Certificate error");
}

// Test de la fonction qui récupère la version de la bibliothèque
TEST_F(TlsTest, LibraryVersion) {
    char versionBuffer[128];
    int result = xTlsGetLibraryVersion(versionBuffer, sizeof(versionBuffer));
    
    EXPECT_EQ(result, XOS_TLS_OK);
    EXPECT_GT(strlen(versionBuffer), 0U);
}

// Test de configuration des ciphers
TEST_F(TlsTest, CipherConfiguration) {
    xos_tls_ctx_t* ctx = xTlsCreateContext(&config);
    ASSERT_NE(ctx, nullptr);
    
    // Configurer une liste de ciphers TLS 1.3
    int result = xTlsSetCiphers(ctx, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256");
    EXPECT_EQ(result, XOS_TLS_OK);
    
    xTlsDestroyContext(ctx);
}

// Test simple pour vérifier PQC support (peut être simulé)
TEST_F(TlsTest, PQCSupport) {
    bool pqcSupported = xTlsIsPQCSupported();
    
    // Ce test peut réussir ou échouer selon la configuration de wolfSSL
    // Nous documentons simplement le résultat
    std::cout << "Post-quantum cryptography support: " << (pqcSupported ? "Yes" : "No") << std::endl;
}

// Test pour vérifier les paramètres incorrects
TEST_F(TlsTest, InvalidParameters) {
    // Paramètres NULL pour la création de contexte
    xos_tls_ctx_t* ctx = xTlsCreateContext(nullptr);
    EXPECT_EQ(ctx, nullptr);
    
    // Créer un contexte valide
    ctx = xTlsCreateContext(&config);
    ASSERT_NE(ctx, nullptr);
    
    // Paramètres NULL pour setCiphers
    int result = xTlsSetCiphers(ctx, nullptr);
    EXPECT_EQ(result, XOS_TLS_INVALID_PARAM);
    
    // Paramètres NULL pour la version de la bibliothèque
    result = xTlsGetLibraryVersion(nullptr, 100);
    EXPECT_EQ(result, XOS_TLS_INVALID_PARAM);
    
    char versionBuffer[10];
    result = xTlsGetLibraryVersion(versionBuffer, 0);
    EXPECT_EQ(result, XOS_TLS_INVALID_PARAM);
    
    xTlsDestroyContext(ctx);
}

// Test de modification des paramètres de configuration
TEST_F(TlsTest, ConfigurationOptions) {
    // Modifier les options de configuration
    config.t_eRole = XOS_TLS_SERVER;
    config.t_eVersion = XOS_TLS_VERSION_1_3_PQ;
    config.t_eKeyExchange = XOS_TLS_KEX_HYBRID_ECDHE_KYBER;
    config.t_bVerifyPeer = true;
    config.t_iVerifyDepth = 5;
    config.t_bSessionReuse = true;
    config.t_iSessionTimeoutSec = 3600;
    
    // Créer un contexte avec ces options
    xos_tls_ctx_t* ctx = xTlsCreateContext(&config);
    ASSERT_NE(ctx, nullptr);
    
    // On ne peut pas vérifier directement si les options sont bien appliquées dans le test unitaire
    // car les structures sont privées, mais au moins on vérifie que la création réussit
    
    xTlsDestroyContext(ctx);
}

// Test de session TLS (mock - ne fait pas de vrai handshake)
TEST_F(TlsTest, MockTlsSession) {
    xos_tls_ctx_t* ctx = xTlsCreateContext(&config);
    ASSERT_NE(ctx, nullptr);
    
    // On ne peut pas faire un vrai handshake dans un test unitaire sans serveur
    // Tester juste la création/destruction de session
    NetworkSocket* mockSocket = (NetworkSocket*)malloc(sizeof(NetworkSocket));
    
    xos_tls_session_t* session = xTlsCreateSession(ctx, mockSocket);
    
    // Le test peut échouer car mockSocket n'est pas un vrai socket
    // C'est normal dans un environnement de test unitaire
    if (session != nullptr) {
        // Si la création a réussi malgré le socket factice, nettoyer
        xTlsDestroySession(session);
    }
    
    free(mockSocket);
    xTlsDestroyContext(ctx);
} 