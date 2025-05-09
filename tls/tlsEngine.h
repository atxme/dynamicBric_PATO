////////////////////////////////////////////////////////////
//  TLS Engine header file for embedded systems
//  Provides TLS 1.3 support using wolfSSL
//  Designed to integrate with xNetwork socket API
//
// Written : 20/04/2025
////////////////////////////////////////////////////////////

#ifndef TLS_ENGINE_H_
#define TLS_ENGINE_H_

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <stdbool.h>

// Configuration constants
#define TLS_OK                  0xF1E92D80
#define TLS_ERROR               0xF1E92D81
#define TLS_INVALID_PARAM       0xF1E92D82
#define TLS_CERT_ERROR          0xF1E92D83
#define TLS_CONNECT_ERROR       0xF1E92D84
#define TLS_VERIFY_ERROR        0xF1E92D85

// tls cipher list available for the API
static const char* s_kptcTlsCipherList [] = 
{
    "TLS_CHACHA20_POLY1305_SHA256",
    "TLS_AES_256_GCM_SHA384",
    "TLS_AES_128_GCM_SHA256",
    "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384",
};

// default tls cipher
static const char *s_kptcDefaultTlsCipher = "TLS_AES_256_GCM_SHA384";

// TLS versions
typedef enum 
{
    TLS_VERSION_1_2 = 0,
    TLS_VERSION_1_3 = 1  // Default and recommended
} TLS_Version;

// ECC Curve types
typedef enum 
{
    TLS_ECC_SECP256R1 = 0,  // NIST P-256
    TLS_ECC_SECP384R1 = 1,  // NIST P-384
    TLS_ECC_SECP521R1 = 2,  // NIST P-521
    TLS_ECC_X25519    = 3   // Curve25519
} TLS_ECC_Curve;

// TLS Engine context
typedef struct tlsEngine_t
{
    WOLFSSL_CTX* t_CipherCtx;   // wolfSSL context
    WOLFSSL* t_SslSession;      // wolfSSL session
    int t_iSocketFd;               // Socket file descriptor
    bool t_bInitialised;        // Initialization state
    bool t_bIsConnected;           // Connection state
    TLS_Version t_eTlsVersion;        // TLS version
    TLS_ECC_Curve t_eEccCurve;     // ECC curve type
} TLS_Engine;

// TLS configuration structure
typedef struct tlsConfig_t
{
    TLS_Version t_eTlsVersion;      // TLS version to use
    TLS_ECC_Curve t_eEccCurve;      // ECC curve to use
    bool t_bVerifyPeer;             // Verify peer certificate : this feature is not necessary for a controlled environment
    const char* t_cCaPath;          // CA certificate path
    const char* t_cCertPath;        // Certificate path
    const char* t_cKeyPath;         // Private key path
    const char* cipherList;         // Cipher list (NULL for defaults) TODO: add cipher list ECDHE and ECDSA and RSA support
    bool t_bIsServer;               // True for server, false for client
    bool t_bLoadEcdsaCipher;        // True to load ECDSA cipher and set the ecc key as ECDSA instead of ecdh
} TLS_Config;

//////////////////////////////////
/// Core API functions - designed to work with socket file descriptors
//////////////////////////////////

//////////////////////////////////
/// @brief Initialize the TLS engine
/// @param p_pttEngine Pointer to TLS_Engine structure to initialize
/// @param p_iSocketFd Socket file descriptor
/// @param p_kpttConfig TLS configuration
/// @return int Error code
//////////////////////////////////
unsigned long tlsEngineInit(TLS_Engine* p_pttEngine, int p_iSocketFd, const TLS_Config* p_kpttConfig);

//////////////////////////////////
/// @brief Perform TLS handshake for client connection
/// @param p_pttEngine TLS engine
/// @return int Error code
//////////////////////////////////
unsigned long tlsEngineConnect(TLS_Engine* p_pttEngine);

//////////////////////////////////
/// @brief Accept TLS connection as server
/// @param p_pttEngine TLS engine for new connection
/// @param p_iSocketFd Socket file descriptor for the accepted connection
/// @param p_pListenEngine TLS engine with server context
/// @return int Error code
//////////////////////////////////
unsigned long tlsEngineAccept(TLS_Engine* p_pttEngine, int p_iSocketFd, const TLS_Engine* p_pListenEngine);

//////////////////////////////////
/// @brief Send data over TLS connection
/// @param p_pttEngine TLS engine
/// @param p_pBuffer Data buffer
/// @param p_ulSize Data size
/// @return int Bytes sent or error code
//////////////////////////////////
unsigned long tlsEngineSend(TLS_Engine* p_pttEngine, const void* p_pBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief Receive data over TLS connection
/// @param p_pttEngine TLS engine
/// @param p_pBuffer Data buffer
/// @param p_ulSize Buffer size
/// @return int Bytes received or error code
//////////////////////////////////
unsigned long tlsEngineReceive(TLS_Engine* p_pttEngine, void* p_pBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief Close TLS connection
/// @param p_pttEngine TLS engine
/// @return int Error code
//////////////////////////////////
unsigned long tlsEngineClose(TLS_Engine* p_pttEngine);

//////////////////////////////////
/// @brief Clean up TLS engine resources
/// @param p_pttEngine TLS engine
/// @return int Error code
//////////////////////////////////
unsigned long tlsEngineCleanup(TLS_Engine* p_pttEngine);

//////////////////////////////////
/// @brief Get last TLS error description
/// @param p_iError Error code
/// @return const char* Error description
//////////////////////////////////
const char* tlsEngineGetErrorString(int p_iError);

//////////////////////////////////
/// @brief Check if TLS is enabled
/// @param p_pttEngine TLS engine
/// @return bool True if TLS is enabled and initialized
//////////////////////////////////
bool tlsEngineIsEnabled(const TLS_Engine* p_pttEngine);

//////////////////////////////////
/// @brief Get TLS connection info
/// @param p_pttEngine TLS engine
/// @param p_pCipherName Buffer to store cipher name
/// @param p_ulSize Buffer size
/// @return int Error code
//////////////////////////////////
unsigned long tlsEngineGetConnectionInfo(const TLS_Engine* p_pttEngine, char* p_pCipherName, unsigned long p_ulSize);

//////////////////////////////////
/// @brief Get the certificate presented by the peer
/// @param p_pttEngine TLS engine
/// @param p_pCertInfo Buffer to store certificate information
/// @param p_ulSize Buffer size
/// @return int Error code
//////////////////////////////////
unsigned long tlsEngineGetPeerCertificate(TLS_Engine* p_pttEngine, char* p_pCertInfo, unsigned long p_ulSize);

//////////////////////////////////
/// @brief Check if the private key with the PEM format
/// @param p_pttEngine TLS engine
/// @param p_pKeyPath Path to the private key file
/// @return int Error code
//////////////////////////////////
unsigned long tlsEngineCheckPrivateKey(TLS_Engine* p_pttEngine, const char* p_pKeyPath);

#endif // TLS_ENGINE_H_
