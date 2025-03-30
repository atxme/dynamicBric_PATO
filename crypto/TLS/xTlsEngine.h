////////////////////////////////////////////////////////////
//  TLS Engine header file
//  Defines the TLS interface using WolfSSL
//  Focused on TLS 1.3 with post-quantum support
//
// general discloser: copy or share the file is forbidden
// Written : 30/03/2025
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////

#ifndef XOS_TLS_ENGINE_H_
#define XOS_TLS_ENGINE_H_

#include <stdint.h>
#include <stdbool.h>

// Forward declaration
typedef struct NetworkSocket NetworkSocket;
typedef struct NetworkAddress NetworkAddress;

// TLS error codes
#define XOS_TLS_OK                  0
#define XOS_TLS_ERROR              -1
#define XOS_TLS_WOULD_BLOCK        -2
#define XOS_TLS_INVALID_PARAM      -3
#define XOS_TLS_NOT_INITIALIZED    -4
#define XOS_TLS_CERTIFICATE_ERROR  -5
#define XOS_TLS_HANDSHAKE_FAILED   -6
#define XOS_TLS_CONNECTION_CLOSED  -7
#define XOS_TLS_NOT_TRUSTED        -8
#define XOS_TLS_TIMEOUT            -9
#define XOS_TLS_VERIFY_ERROR       -10
#define XOS_TLS_VERSION_ERROR      -11
#define XOS_TLS_MEMORY_ERROR       -12

// TLS roles
typedef enum {
    XOS_TLS_CLIENT,
    XOS_TLS_SERVER
} t_tlsRole;

// TLS versions (we focus on TLS 1.3 but keep compatibility modes)
typedef enum {
    XOS_TLS_VERSION_1_2_COMPAT = 0,   // TLS 1.2 compatibility mode
    XOS_TLS_VERSION_1_3_ONLY   = 1,   // TLS 1.3 only
    XOS_TLS_VERSION_1_3_PQ     = 2    // TLS 1.3 with Post-Quantum Cryptography
} t_tlsVersion;

// TLS key exchange options for post-quantum
typedef enum {
    XOS_TLS_KEX_STANDARD = 0,           // Standard ECDHE key exchange
    XOS_TLS_KEX_PQ_KYBER  = 1,          // Kyber key exchange (post-quantum)
    XOS_TLS_KEX_HYBRID_ECDHE_KYBER = 2  // Hybrid key exchange (classical + post-quantum)
} t_tlsKeyExchange;

// TLS signature algorithms for post-quantum
typedef enum {
    XOS_TLS_SIG_RSA_PSS = 0,       // RSA-PSS signatures
    XOS_TLS_SIG_ECDSA = 1,         // ECDSA signatures
    XOS_TLS_SIG_ED25519 = 2,       // Ed25519 signatures
    XOS_TLS_SIG_PQ_DILITHIUM = 3,  // Dilithium signatures (post-quantum)
    XOS_TLS_SIG_PQ_FALCON = 4      // Falcon signatures (post-quantum)
} t_tlsSigAlgorithm;

// TLS configuration structure
typedef struct {
    t_tlsRole t_eRole;             // Client or server
    t_tlsVersion t_eVersion;       // TLS version
    t_tlsKeyExchange t_eKeyExchange; // Key exchange algorithm
    t_tlsSigAlgorithm t_eSigAlg;   // Signature algorithm
    
    // Certificate paths
    const char* t_pcCertPath;      // Certificate path
    const char* t_pcKeyPath;       // Private key path
    const char* t_pcCaPath;        // CA certificate path/directory
    
    // Security options
    bool t_bVerifyPeer;            // Whether to verify peer certificates
    int t_iVerifyDepth;            // Verification chain depth
    bool t_bAllowSelfSigned;       // Allow self-signed certificates
    
    // Session parameters
    int t_iSessionTimeoutSec;      // Session timeout in seconds
    bool t_bSessionReuse;          // Allow session reuse
    bool t_bRenegotiation;         // Allow renegotiation
    
    // Advanced options
    const char* t_pcCipherList;    // Cipher suite list (NULL for defaults)
    const char* t_pcHostname;      // Server name for SNI (client only)
    bool t_bRequireAuth;           // Require client auth (server only)
    
    // Custom callback handlers (optional, can be NULL)
    void* t_pVerifyCallback;       // Certificate verification callback
    void* t_pUserData;             // User data for callbacks
} xos_tls_config_t;

// Opaque TLS context structure
typedef struct xos_tls_ctx_t xos_tls_ctx_t;

// Opaque TLS session structure
typedef struct xos_tls_session_t xos_tls_session_t;

//////////////////////////////////
/// @brief Initialize TLS engine (should be called once at program start)
/// @return int Error code
//////////////////////////////////
int xTlsInit(void);

//////////////////////////////////
/// @brief Cleanup TLS engine resources (called at program termination)
/// @return int Error code
//////////////////////////////////
int xTlsCleanup(void);

//////////////////////////////////
/// @brief Create a new TLS context based on configuration
/// @param p_ptConfig TLS configuration structure
/// @return xos_tls_ctx_t* TLS context or NULL on failure
//////////////////////////////////
xos_tls_ctx_t* xTlsCreateContext(const xos_tls_config_t* p_ptConfig);

//////////////////////////////////
/// @brief Free a TLS context
/// @param p_ptCtx TLS context
//////////////////////////////////
void xTlsDestroyContext(xos_tls_ctx_t* p_ptCtx);

//////////////////////////////////
/// @brief Create a new TLS session for a socket
/// @param p_ptCtx TLS context
/// @param p_ptSocket Network socket
/// @return xos_tls_session_t* TLS session or NULL on failure
//////////////////////////////////
xos_tls_session_t* xTlsCreateSession(xos_tls_ctx_t* p_ptCtx, NetworkSocket* p_ptSocket);

//////////////////////////////////
/// @brief Free a TLS session
/// @param p_ptSession TLS session
//////////////////////////////////
void xTlsDestroySession(xos_tls_session_t* p_ptSession);

//////////////////////////////////
/// @brief Perform TLS handshake
/// @param p_ptSession TLS session
/// @return int Error code (XOS_TLS_WOULD_BLOCK may be returned for non-blocking sockets)
//////////////////////////////////
int xTlsHandshake(xos_tls_session_t* p_ptSession);

//////////////////////////////////
/// @brief Send data over TLS connection
/// @param p_ptSession TLS session
/// @param p_ptData Data buffer
/// @param p_ulDataLen Data length
/// @return int Bytes sent or error code
//////////////////////////////////
int xTlsSend(xos_tls_session_t* p_ptSession, const void* p_ptData, unsigned long p_ulDataLen);

//////////////////////////////////
/// @brief Receive data over TLS connection
/// @param p_ptSession TLS session
/// @param p_ptBuffer Receive buffer
/// @param p_ulBufferLen Buffer length
/// @return int Bytes received or error code
//////////////////////////////////
int xTlsReceive(xos_tls_session_t* p_ptSession, void* p_ptBuffer, unsigned long p_ulBufferLen);

//////////////////////////////////
/// @brief Check if TLS session is established
/// @param p_ptSession TLS session
/// @return bool True if handshake is complete and connection is active
//////////////////////////////////
bool xTlsIsConnected(xos_tls_session_t* p_ptSession);

//////////////////////////////////
/// @brief Get peer certificate information
/// @param p_ptSession TLS session
/// @param p_ptSubject Buffer to store subject name
/// @param p_ulSubjectLen Subject buffer length
/// @param p_ptIssuer Buffer to store issuer name
/// @param p_ulIssuerLen Issuer buffer length
/// @return int Error code
//////////////////////////////////
int xTlsGetPeerCertificate(xos_tls_session_t* p_ptSession, 
                          char* p_ptSubject, unsigned long p_ulSubjectLen,
                          char* p_ptIssuer, unsigned long p_ulIssuerLen);

//////////////////////////////////
/// @brief Get TLS connection information
/// @param p_ptSession TLS session
/// @param p_pcCipherName Buffer to store cipher name
/// @param p_ulCipherNameLen Cipher name buffer length
/// @param p_pcVersion Buffer to store TLS version
/// @param p_ulVersionLen Version buffer length
/// @return int Error code
//////////////////////////////////
int xTlsGetConnectionInfo(xos_tls_session_t* p_ptSession, 
                          char* p_pcCipherName, unsigned long p_ulCipherNameLen,
                          char* p_pcVersion, unsigned long p_ulVersionLen);

//////////////////////////////////
/// @brief Get error description for TLS error code
/// @param p_iErrorCode Error code
/// @return const char* Error description
//////////////////////////////////
const char* xTlsGetErrorString(int p_iErrorCode);

//////////////////////////////////
/// @brief Load custom Diffie-Hellman parameters for key exchange
/// @param p_ptCtx TLS context
/// @param p_pcDhFile Path to DH parameters file
/// @return int Error code
//////////////////////////////////
int xTlsLoadDHParams(xos_tls_ctx_t* p_ptCtx, const char* p_pcDhFile);

//////////////////////////////////
/// @brief Set explicit cipher list for TLS context
/// @param p_ptCtx TLS context
/// @param p_pcCipherList Comma-separated list of ciphers
/// @return int Error code
//////////////////////////////////
int xTlsSetCiphers(xos_tls_ctx_t* p_ptCtx, const char* p_pcCipherList);

//////////////////////////////////
/// @brief Check TLS library version
/// @param p_pcVersionBuffer Buffer to store version string
/// @param p_ulBufferLen Buffer length
/// @return int Error code
//////////////////////////////////
int xTlsGetLibraryVersion(char* p_pcVersionBuffer, unsigned long p_ulBufferLen);

//////////////////////////////////
/// @brief Check if post-quantum is supported in this build
/// @return bool True if post-quantum functionality is available
//////////////////////////////////
bool xTlsIsPQCSupported(void);

#endif // XOS_TLS_ENGINE_H_
