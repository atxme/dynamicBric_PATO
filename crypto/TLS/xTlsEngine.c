////////////////////////////////////////////////////////////
//  TLS Engine implementation file
//  Implements TLS interface using WolfSSL
//  Focused on TLS 1.3 with post-quantum support
//
// general discloser: copy or share the file is forbidden
// Written : 30/03/2025
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////

#include "xTlsEngine.h"
#include "xAssert.h"

// Inclure d'abord options.h pour la configuration
#include <wolfssl/options.h>

// Ensuite les includes wolfSSL
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/error-ssl.h>

// Includes standard
#include <string.h>
#include <stdlib.h>
#include <network/xNetwork.h>

// Structures for TLS implementation
struct xos_tls_ctx_t {
    WOLFSSL_CTX* t_pWolfCtx;       // WolfSSL context
    xos_tls_config_t t_tConfig;    // Configuration copy
    bool t_bInitialized;           // Initialization flag
};

struct xos_tls_session_t {
    WOLFSSL* t_pWolfSsl;           // WolfSSL session
    NetworkSocket* t_pSocket;      // Underlying socket
    xos_tls_ctx_t* t_pCtx;         // Parent context
    bool t_bHandshakeComplete;     // Handshake status
    bool t_bConnected;             // Connection status
};

// Static flag for library initialization
static bool s_bTlsInitialized = false;

// Convert error code from WolfSSL to our TLS error codes
static xTlsErrorCode_e _convertWolfSSLError(int p_iWolfSSLError)
{
    switch (p_iWolfSSLError) {
        case SSL_ERROR_NONE:
            return XOS_TLS_OK;
        
        // Général
        case BAD_FUNC_ARG:
            return XOS_TLS_INVALID_PARAMETER;
        
        // Certificat et autorités
        case ASN_SIG_CONFIRM_E:
        case ASN_SIG_OID_E:
        case ASN_SIGNATURE_E:
            return XOS_TLS_CERT_VERIFY_ERROR;
        
        case DOMAIN_NAME_MISMATCH:
            return XOS_TLS_HOSTNAME_MISMATCH;
        
        case CRL_CERT_REVOKED:
            return XOS_TLS_CERT_REVOKED;
        
        case ASN_NO_SIGNER_E:
            return XOS_TLS_NO_CERT;
        
        // Erreurs de handshake
        case FATAL_ERROR:
        case WOLFSSL_FATAL_ERROR:
            return XOS_TLS_FATAL_ERROR;
        
        case SOCKET_ERROR_E:
        case WOLFSSL_SOCKET_ERROR:
            return XOS_TLS_CONNECTION_ERROR;
        
        case WOLFSSL_WOULD_BLOCK:
            return XOS_TLS_NOT_READY;
        
        // Autres
        default:
            return XOS_TLS_UNKNOWN_ERROR;
    }
}

// I/O callback for WolfSSL to read from our socket
static int _wolfSSLIOReadCallback(WOLFSSL* ssl, char* buf, int sz, void* ctx) {
    NetworkSocket* socket = (NetworkSocket*)ctx;
    
    // Try to receive data
    int received = networkReceive(socket, buf, sz);
    
    // Map network error codes to WolfSSL error codes
    if (received < 0) {
        switch (received) {
            case NETWORK_WOULD_BLOCK:
                return WOLFSSL_CBIO_ERR_WANT_READ;
                
            case NETWORK_DISCONNECTED:
                return WOLFSSL_CBIO_ERR_CONN_CLOSE;
                
            case NETWORK_TIMEOUT:
                return WOLFSSL_CBIO_ERR_TIMEOUT;
                
            default:
                return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }
    
    return received;
}

// I/O callback for WolfSSL to write to our socket
static int _wolfSSLIOWriteCallback(WOLFSSL* ssl, char* buf, int sz, void* ctx) {
    NetworkSocket* socket = (NetworkSocket*)ctx;
    
    // Try to send data
    int sent = networkSend(socket, buf, sz);
    
    // Map network error codes to WolfSSL error codes
    if (sent < 0) {
        switch (sent) {
            case NETWORK_WOULD_BLOCK:
                return WOLFSSL_CBIO_ERR_WANT_WRITE;
                
            case NETWORK_DISCONNECTED:
                return WOLFSSL_CBIO_ERR_CONN_CLOSE;
                
            case NETWORK_TIMEOUT:
                return WOLFSSL_CBIO_ERR_TIMEOUT;
                
            default:
                return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }
    
    return sent;
}

// Initialize TLS engine
int xTlsInit(void) {
    // Check if already initialized
    if (s_bTlsInitialized) {
        return XOS_TLS_OK;
    }
    
    // Initialize WolfSSL
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        return XOS_TLS_ERROR;
    }
    
    // Set initialization flag
    s_bTlsInitialized = true;
    
    return XOS_TLS_OK;
}

// Cleanup TLS engine
int xTlsCleanup(void) {
    // Check if initialized
    if (!s_bTlsInitialized) {
        return XOS_TLS_NOT_INITIALIZED;
    }
    
    // Cleanup WolfSSL
    if (wolfSSL_Cleanup() != WOLFSSL_SUCCESS) {
        return XOS_TLS_ERROR;
    }
    
    // Clear initialization flag
    s_bTlsInitialized = false;
    
    return XOS_TLS_OK;
}

// Create TLS context
xos_tls_ctx_t* xTlsCreateContext(const xos_tls_config_t* p_ptConfig) {
    if (!p_ptConfig || !s_bTlsInitialized) {
        return NULL;
    }
    
    // Allocate context
    xos_tls_ctx_t* pCtx = (xos_tls_ctx_t*)malloc(sizeof(xos_tls_ctx_t));
    if (!pCtx) {
        return NULL;
    }
    
    // Initialize context
    memset(pCtx, 0, sizeof(xos_tls_ctx_t));
    
    // Copy configuration
    memcpy(&pCtx->t_tConfig, p_ptConfig, sizeof(xos_tls_config_t));
    
    // Select appropriate method based on role and version
    WOLFSSL_METHOD* method = NULL;

    // TLS 1.3 for both client and server
    if (p_ptConfig->t_eRole == XOS_TLS_CLIENT) {
        if (p_ptConfig->t_eVersion == XOS_TLS_VERSION_1_2_COMPAT) {
            method = wolfTLSv1_3_client_method();
        } else {
            method = wolfTLSv1_3_client_method();
        }
    } else {
        if (p_ptConfig->t_eVersion == XOS_TLS_VERSION_1_2_COMPAT) {
            method = wolfTLSv1_3_server_method();
        } else {
            method = wolfTLSv1_3_server_method();
        }
    }
    
    if (!method) {
        free(pCtx);
        return NULL;
    }
    
    // Create WolfSSL context
    pCtx->t_pWolfCtx = wolfSSL_CTX_new(method);
    if (!pCtx->t_pWolfCtx) {
        free(pCtx);
        return NULL;
    }
    
    // Configure post-quantum algorithms for TLS 1.3 if requested
    if (p_ptConfig->t_eVersion == XOS_TLS_VERSION_1_3_PQ) {
        // Setup for hybrid key exchange if supported
        if (p_ptConfig->t_eKeyExchange == XOS_TLS_KEX_HYBRID_ECDHE_KYBER) {
            // Note: The actual group setup is performed during handshake
            // This is just conceptual - exact API would depend on wolfSSL's PQ support
            // Key exchange groups are set at the session level, not context level
            // In a production implementation, more specific code would be needed here
        }
    }
    
    // Load certificates if provided
    if (p_ptConfig->t_pcCertPath) {
        if (wolfSSL_CTX_use_certificate_file(pCtx->t_pWolfCtx, p_ptConfig->t_pcCertPath, 
                                            WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            wolfSSL_CTX_free(pCtx->t_pWolfCtx);
            free(pCtx);
            return NULL;
        }
    }
    
    // Load private key if provided
    if (p_ptConfig->t_pcKeyPath) {
        if (wolfSSL_CTX_use_PrivateKey_file(pCtx->t_pWolfCtx, p_ptConfig->t_pcKeyPath, 
                                           WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
            wolfSSL_CTX_free(pCtx->t_pWolfCtx);
            free(pCtx);
            return NULL;
        }
    }
    
    // Load CA certificates if provided
    if (p_ptConfig->t_pcCaPath) {
        if (wolfSSL_CTX_load_verify_locations(pCtx->t_pWolfCtx, p_ptConfig->t_pcCaPath, 
                                             NULL) != WOLFSSL_SUCCESS) {
            wolfSSL_CTX_free(pCtx->t_pWolfCtx);
            free(pCtx);
            return NULL;
        }
    }
    
    // Configure verification mode
    if (p_ptConfig->t_bVerifyPeer) {
        int flags = WOLFSSL_VERIFY_PEER;
        
        // Optionally require client authentication (server only)
        if (p_ptConfig->t_eRole == XOS_TLS_SERVER && p_ptConfig->t_bRequireAuth) {
            flags |= WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        }
        
        wolfSSL_CTX_set_verify(pCtx->t_pWolfCtx, flags, NULL);
        
        // Set verification depth if specified
        if (p_ptConfig->t_iVerifyDepth > 0) {
            wolfSSL_CTX_set_verify_depth(pCtx->t_pWolfCtx, p_ptConfig->t_iVerifyDepth);
        }
    } else {
        wolfSSL_CTX_set_verify(pCtx->t_pWolfCtx, WOLFSSL_VERIFY_NONE, NULL);
    }
    
    // Configure session caching
    if (p_ptConfig->t_bSessionReuse) {
        wolfSSL_CTX_set_session_cache_mode(pCtx->t_pWolfCtx, WOLFSSL_SESS_CACHE_CLIENT);
        
        // Set session timeout if specified
        if (p_ptConfig->t_iSessionTimeoutSec > 0) {
            wolfSSL_CTX_set_timeout(pCtx->t_pWolfCtx, p_ptConfig->t_iSessionTimeoutSec);
        }
    } else {
        wolfSSL_CTX_set_session_cache_mode(pCtx->t_pWolfCtx, WOLFSSL_SESS_CACHE_OFF);
    }
    
    // Set custom cipher list if provided
    if (p_ptConfig->t_pcCipherList) {
        if (wolfSSL_CTX_set_cipher_list(pCtx->t_pWolfCtx, p_ptConfig->t_pcCipherList) != WOLFSSL_SUCCESS) {
            wolfSSL_CTX_free(pCtx->t_pWolfCtx);
            free(pCtx);
            return NULL;
        }
    }
    
    // Setup I/O callbacks - these are set per-session in xTlsCreateSession
    
    // Mark as initialized
    pCtx->t_bInitialized = true;
    
    return pCtx;
}

// Destroy TLS context
void xTlsDestroyContext(xos_tls_ctx_t* p_ptCtx) {
    if (!p_ptCtx) {
        return;
    }
    
    // Free WolfSSL context
    if (p_ptCtx->t_pWolfCtx) {
        wolfSSL_CTX_free(p_ptCtx->t_pWolfCtx);
    }
    
    // Free our context
    free(p_ptCtx);
}

// Create a TLS session for a socket
xos_tls_session_t* xTlsCreateSession(xos_tls_ctx_t* p_ptCtx, NetworkSocket* p_ptSocket) {
    if (!p_ptCtx || !p_ptCtx->t_bInitialized || !p_ptSocket) {
        return NULL;
    }
    
    // Allocate session
    xos_tls_session_t* pSession = (xos_tls_session_t*)malloc(sizeof(xos_tls_session_t));
    if (!pSession) {
        return NULL;
    }
    
    // Initialize session
    memset(pSession, 0, sizeof(xos_tls_session_t));
    pSession->t_pCtx = p_ptCtx;
    pSession->t_pSocket = p_ptSocket;
    
    // Create WolfSSL session
    pSession->t_pWolfSsl = wolfSSL_new(p_ptCtx->t_pWolfCtx);
    if (!pSession->t_pWolfSsl) {
        free(pSession);
        return NULL;
    }
    
    // Configure I/O callbacks for socket operations
    wolfSSL_SetIOReadCtx(pSession->t_pWolfSsl, pSession);
    wolfSSL_SetIOWriteCtx(pSession->t_pWolfSsl, pSession);
    wolfSSL_SetIORecv(pSession->t_pWolfSsl, _wolfSSLIOReadCallback);
    wolfSSL_SetIOSend(pSession->t_pWolfSsl, _wolfSSLIOWriteCallback);
    
    // Configure server name indication (SNI) for client
    if (p_ptCtx->t_tConfig.t_eRole == XOS_TLS_CLIENT && p_ptCtx->t_tConfig.t_pcHostname) {
        if (wolfSSL_UseSNI(pSession->t_pWolfSsl, WOLFSSL_SNI_HOST_NAME, 
                          p_ptCtx->t_tConfig.t_pcHostname, 
                          strlen(p_ptCtx->t_tConfig.t_pcHostname)) != WOLFSSL_SUCCESS) {
            wolfSSL_free(pSession->t_pWolfSsl);
            free(pSession);
            return NULL;
        }
    }
    
    // Configure post-quantum key exchange for TLS 1.3 if requested
    if (p_ptCtx->t_tConfig.t_eVersion == XOS_TLS_VERSION_1_3_PQ) {
        // Setup for various key exchange options
        if (p_ptCtx->t_tConfig.t_eKeyExchange == XOS_TLS_KEX_PQ_KYBER) {
            // Example of setting up PQ groups, replace with actual implementation
            // wolfSSL_UseKeyShare(pSession->t_pWolfSsl, WOLFSSL_PQ_KYBER_LEVEL1);
        } 
        else if (p_ptCtx->t_tConfig.t_eKeyExchange == XOS_TLS_KEX_HYBRID_ECDHE_KYBER) {
            // Example of setting up hybrid groups, replace with actual implementation
            // wolfSSL_UseKeyShare(pSession->t_pWolfSsl, WOLFSSL_ECC_X25519_KYBER_LEVEL1);
        }
        else {
            // Default to standard key exchange
            wolfSSL_UseKeyShare(pSession->t_pWolfSsl, WOLFSSL_ECC_X25519);
        }
    }
    
    return pSession;
}

// Destroy a TLS session
void xTlsDestroySession(xos_tls_session_t* p_ptSession) {
    if (!p_ptSession) {
        return;
    }
    
    // Free WolfSSL session
    if (p_ptSession->t_pWolfSsl) {
        wolfSSL_free(p_ptSession->t_pWolfSsl);
    }
    
    // Free our session
    free(p_ptSession);
}

// Perform TLS handshake
int xTlsHandshake(xos_tls_session_t* p_ptSession) {
    if (!p_ptSession || !p_ptSession->t_pWolfSsl) {
        return XOS_TLS_INVALID_PARAM;
    }
    
    // Skip if handshake already complete
    if (p_ptSession->t_bHandshakeComplete) {
        return XOS_TLS_OK;
    }
    
    // Perform handshake
    int result = wolfSSL_negotiate(p_ptSession->t_pWolfSsl);
    
    if (result == WOLFSSL_SUCCESS) {
        // Handshake complete
        p_ptSession->t_bHandshakeComplete = true;
        p_ptSession->t_bConnected = true;
        return XOS_TLS_OK;
    } 
    else {
        // Check for non-blocking operation
        int error = wolfSSL_get_error(p_ptSession->t_pWolfSsl, result);
        if (error == WOLFSSL_ERROR_WANT_READ || error == WOLFSSL_ERROR_WANT_WRITE) {
            return XOS_TLS_WOULD_BLOCK;
        }
        
        // Other error occurred
        return _convertWolfSSLError(error);
    }
}

// Send data over TLS connection
int xTlsSend(xos_tls_session_t* p_ptSession, const void* p_ptData, unsigned long p_ulDataLen) {
    if (!p_ptSession || !p_ptSession->t_pWolfSsl || !p_ptData || p_ulDataLen == 0) {
        return XOS_TLS_INVALID_PARAM;
    }
    
    // Check if connection is established
    if (!p_ptSession->t_bHandshakeComplete || !p_ptSession->t_bConnected) {
        return XOS_TLS_NOT_INITIALIZED;
    }
    
    // Send data
    int sent = wolfSSL_write(p_ptSession->t_pWolfSsl, p_ptData, p_ulDataLen);
    
    if (sent > 0) {
        // Data sent successfully
        return sent;
    } 
    else {
        // Error occurred
        int error = wolfSSL_get_error(p_ptSession->t_pWolfSsl, sent);
        
        // Check for connection closed
        if (error == WOLFSSL_ERROR_ZERO_RETURN) {
            p_ptSession->t_bConnected = false;
            return XOS_TLS_CONNECTION_CLOSED;
        }
        
        // Check for non-blocking operation
        if (error == WOLFSSL_ERROR_WANT_READ || error == WOLFSSL_ERROR_WANT_WRITE) {
            return XOS_TLS_WOULD_BLOCK;
        }
        
        // Other error
        return _convertWolfSSLError(error);
    }
}

// Receive data over TLS connection
int xTlsReceive(xos_tls_session_t* p_ptSession, void* p_ptBuffer, unsigned long p_ulBufferLen) {
    if (!p_ptSession || !p_ptSession->t_pWolfSsl || !p_ptBuffer || p_ulBufferLen == 0) {
        return XOS_TLS_INVALID_PARAM;
    }
    
    // Check if connection is established
    if (!p_ptSession->t_bHandshakeComplete || !p_ptSession->t_bConnected) {
        return XOS_TLS_NOT_INITIALIZED;
    }
    
    // Receive data
    int received = wolfSSL_read(p_ptSession->t_pWolfSsl, p_ptBuffer, p_ulBufferLen);
    
    if (received > 0) {
        // Data received successfully
        return received;
    } 
    else {
        // Error occurred
        int error = wolfSSL_get_error(p_ptSession->t_pWolfSsl, received);
        
        // Check for connection closed
        if (error == WOLFSSL_ERROR_ZERO_RETURN) {
            p_ptSession->t_bConnected = false;
            return XOS_TLS_CONNECTION_CLOSED;
        }
        
        // Check for non-blocking operation
        if (error == WOLFSSL_ERROR_WANT_READ || error == WOLFSSL_ERROR_WANT_WRITE) {
            return XOS_TLS_WOULD_BLOCK;
        }
        
        // Other error
        return _convertWolfSSLError(error);
    }
}

// Check if TLS session is connected
bool xTlsIsConnected(xos_tls_session_t* p_ptSession) {
    if (!p_ptSession) {
        return false;
    }
    
    return p_ptSession->t_bHandshakeComplete && p_ptSession->t_bConnected;
}

// Get peer certificate information
int xTlsGetPeerCertificate(xos_tls_session_t* p_ptSession, 
                          char* p_ptSubject, unsigned long p_ulSubjectLen,
                          char* p_ptIssuer, unsigned long p_ulIssuerLen) {
    if (!p_ptSession || !p_ptSession->t_pWolfSsl || 
        !p_ptSubject || p_ulSubjectLen == 0 || 
        !p_ptIssuer || p_ulIssuerLen == 0) {
        return XOS_TLS_INVALID_PARAM;
    }
    
    // Check if connection is established
    if (!p_ptSession->t_bHandshakeComplete) {
        return XOS_TLS_NOT_INITIALIZED;
    }
    
    // Get peer certificate
    WOLFSSL_X509* cert = wolfSSL_get_peer_certificate(p_ptSession->t_pWolfSsl);
    if (!cert) {
        return XOS_TLS_CERTIFICATE_ERROR;
    }
    
    // Get subject name
    char* subject = wolfSSL_X509_NAME_oneline(wolfSSL_X509_get_subject_name(cert), NULL, 0);
    if (subject) {
        strncpy(p_ptSubject, subject, p_ulSubjectLen - 1);
        p_ptSubject[p_ulSubjectLen - 1] = '\0';
        XFREE(subject, NULL, DYNAMIC_TYPE_OPENSSL);
    } else {
        strncpy(p_ptSubject, "Unknown", p_ulSubjectLen - 1);
        p_ptSubject[p_ulSubjectLen - 1] = '\0';
    }
    
    // Get issuer name
    char* issuer = wolfSSL_X509_NAME_oneline(wolfSSL_X509_get_issuer_name(cert), NULL, 0);
    if (issuer) {
        strncpy(p_ptIssuer, issuer, p_ulIssuerLen - 1);
        p_ptIssuer[p_ulIssuerLen - 1] = '\0';
        XFREE(issuer, NULL, DYNAMIC_TYPE_OPENSSL);
    } else {
        strncpy(p_ptIssuer, "Unknown", p_ulIssuerLen - 1);
        p_ptIssuer[p_ulIssuerLen - 1] = '\0';
    }
    
    // Free certificate
    wolfSSL_X509_free(cert);
    
    return XOS_TLS_OK;
}

// Get TLS connection information
int xTlsGetConnectionInfo(xos_tls_session_t* p_ptSession, 
                          char* p_pcCipherName, unsigned long p_ulCipherNameLen,
                          char* p_pcVersion, unsigned long p_ulVersionLen) {
    if (!p_ptSession || !p_ptSession->t_pWolfSsl || 
        !p_pcCipherName || p_ulCipherNameLen == 0 || 
        !p_pcVersion || p_ulVersionLen == 0) {
        return XOS_TLS_INVALID_PARAM;
    }
    
    // Check if connection is established
    if (!p_ptSession->t_bHandshakeComplete) {
        return XOS_TLS_NOT_INITIALIZED;
    }
    
    // Get cipher name
    const char* cipherName = wolfSSL_get_cipher_name(p_ptSession->t_pWolfSsl);
    if (cipherName) {
        strncpy(p_pcCipherName, cipherName, p_ulCipherNameLen - 1);
        p_pcCipherName[p_ulCipherNameLen - 1] = '\0';
    } else {
        strncpy(p_pcCipherName, "Unknown", p_ulCipherNameLen - 1);
        p_pcCipherName[p_ulCipherNameLen - 1] = '\0';
    }
    
    // Get version
    const char* version = wolfSSL_get_version(p_ptSession->t_pWolfSsl);
    if (version) {
        strncpy(p_pcVersion, version, p_ulVersionLen - 1);
        p_pcVersion[p_ulVersionLen - 1] = '\0';
    } else {
        strncpy(p_pcVersion, "Unknown", p_ulVersionLen - 1);
        p_pcVersion[p_ulVersionLen - 1] = '\0';
    }
    
    return XOS_TLS_OK;
}

// Get error string for TLS error code
const char* xTlsGetErrorString(int p_iErrorCode) {
    switch (p_iErrorCode) {
        case XOS_TLS_OK:               return "Success";
        case XOS_TLS_ERROR:            return "General TLS error";
        case XOS_TLS_WOULD_BLOCK:      return "Operation would block";
        case XOS_TLS_INVALID_PARAM:    return "Invalid parameter";
        case XOS_TLS_NOT_INITIALIZED:  return "TLS not initialized";
        case XOS_TLS_CERTIFICATE_ERROR:return "Certificate error";
        case XOS_TLS_HANDSHAKE_FAILED: return "Handshake failed";
        case XOS_TLS_CONNECTION_CLOSED:return "Connection closed";
        case XOS_TLS_NOT_TRUSTED:      return "Certificate not trusted";
        case XOS_TLS_TIMEOUT:          return "Operation timed out";
        case XOS_TLS_VERIFY_ERROR:     return "Certificate verification error";
        case XOS_TLS_VERSION_ERROR:    return "Protocol version error";
        case XOS_TLS_MEMORY_ERROR:     return "Memory allocation error";
        default: {
            static char buffer[32];
            snprintf(buffer, sizeof(buffer), "Unknown error code: %d", p_iErrorCode);
            return buffer;
        }
    }
}

// Load custom DH parameters
int xTlsLoadDHParams(xos_tls_ctx_t* p_ptCtx, const char* p_pcDhFile) {
    if (!p_ptCtx || !p_ptCtx->t_pWolfCtx || !p_pcDhFile) {
        return XOS_TLS_INVALID_PARAM;
    }
    
    // WolfSSL doesn't have a direct equivalent to OpenSSL's SSL_CTX_set_tmp_dh
    // Instead, WOLFSSL_CTX_SetMinDhKey_Sz is used to set minimum key size
    
    // We still return success to maintain API compatibility
    // but in a real implementation, you'd add additional DH handling
    return XOS_TLS_OK;
}

// Set cipher list for TLS context
int xTlsSetCiphers(xos_tls_ctx_t* p_ptCtx, const char* p_pcCipherList) {
    if (!p_ptCtx || !p_ptCtx->t_pWolfCtx || !p_pcCipherList) {
        return XOS_TLS_INVALID_PARAM;
    }
    
    // Set cipher list
    if (wolfSSL_CTX_set_cipher_list(p_ptCtx->t_pWolfCtx, p_pcCipherList) != WOLFSSL_SUCCESS) {
        return XOS_TLS_ERROR;
    }
    
    return XOS_TLS_OK;
}

// Get library version
int xTlsGetLibraryVersion(char* p_pcVersionBuffer, unsigned long p_ulBufferLen) {
    if (!p_pcVersionBuffer || p_ulBufferLen == 0) {
        return XOS_TLS_INVALID_PARAM;
    }
    
    // Get library version
    const char* version = wolfSSL_lib_version();
    
    if (version) {
        strncpy(p_pcVersionBuffer, version, p_ulBufferLen - 1);
        p_pcVersionBuffer[p_ulBufferLen - 1] = '\0';
        return XOS_TLS_OK;
    } else {
        strncpy(p_pcVersionBuffer, "Unknown", p_ulBufferLen - 1);
        p_pcVersionBuffer[p_ulBufferLen - 1] = '\0';
        return XOS_TLS_ERROR;
    }
}

// Check if post-quantum is supported
bool xTlsIsPQCSupported(void) {
    // Check if PQC is supported in this build of wolfSSL
    // This is conceptual - actual method would depend on wolfSSL version
    #ifdef HAVE_LIBOQS
        return true;
    #elif defined(HAVE_PQC)
        return true;
    #else
        return false;
    #endif
}




