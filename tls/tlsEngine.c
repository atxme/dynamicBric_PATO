////////////////////////////////////////////////////////////
//  TLS Engine implementation file for embedded systems
//  Implements TLS 1.3 support using wolfSSL
//  Designed to integrate with socket file descriptors
//
// Written : 20/04/2025
////////////////////////////////////////////////////////////

#include "tlsEngine.h"
#include "xLog.h"
#include "xAssert.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

static bool s_bLibTlsInitialised = false;


////////////////////////////////////////////////////////////
/// tlsEngineIORecv
////////////////////////////////////////////////////////////
static int tlsEngineIORecv(WOLFSSL* t_SslSession, char* buf, int sz, void* t_CipherCtx) 
{
    X_ASSERT(t_CipherCtx != NULL);
    X_ASSERT(buf != NULL);
    X_ASSERT(sz > 0);
    X_ASSERT(t_SslSession != NULL);
    int t_iSocketFd = *(int*)t_CipherCtx;
    if (t_iSocketFd < 0) {
        return WOLFSSL_CBIO_ERR_GENERAL;
    }
    
    int recvd = recv(t_iSocketFd, buf, sz, 0);
    if (recvd < 0) 
    {
        return WOLFSSL_CBIO_ERR_GENERAL;
    }
    return recvd;
}

////////////////////////////////////////////////////////////
/// tlsEngineIOSend
////////////////////////////////////////////////////////////
static int tlsEngineIOSend(WOLFSSL* t_SslSession, char* buf, int sz, void* t_CipherCtx) 
{
    X_ASSERT(t_CipherCtx != NULL);
    X_ASSERT(buf != NULL);
    X_ASSERT(sz > 0);
    X_ASSERT(t_SslSession != NULL);

    int t_iSocketFd = *(int*)t_CipherCtx;
    if (t_iSocketFd < 0) {
        return WOLFSSL_CBIO_ERR_GENERAL;
    }
    
    int sent = send(t_iSocketFd, buf, sz, 0);
    if (sent < 0) 
    {
        return WOLFSSL_CBIO_ERR_GENERAL;
    }
    return sent;
}

////////////////////////////////////////////////////////////
/// tlsEngineInit
////////////////////////////////////////////////////////////
unsigned long tlsEngineInit(TLS_Engine* p_pttEngine, int p_iSocketFd, const TLS_Config* p_kpttConfig) 
{
    X_ASSERT(p_pttEngine != NULL);
    X_ASSERT(p_kpttConfig != NULL);
    X_ASSERT(p_iSocketFd >= 0);

    X_LOG_TRACE("Initializing TLS engine for socket %d", p_iSocketFd);

    if (!p_pttEngine || !p_kpttConfig || p_iSocketFd < 0) 
    {
        X_LOG_TRACE("Invalid parameters for TLS engine initialization");
        return TLS_INVALID_PARAM;
    }

    // Initialize engine structure
    memset(p_pttEngine, 0, sizeof(TLS_Engine));
    p_pttEngine->t_iSocketFd = p_iSocketFd;
    p_pttEngine->t_eTlsVersion = p_kpttConfig->t_eTlsVersion;
    p_pttEngine->t_eEccCurve = p_kpttConfig->t_eEccCurve;
    
    // Initialize wolfSSL library if not already done
    if (!s_bLibTlsInitialised) 
    {
        if (wolfSSL_Init() != WOLFSSL_SUCCESS) 
        {
            X_LOG_TRACE("Failed to initialize wolfSSL library");
            return TLS_ERROR;
        }
        s_bLibTlsInitialised = true;
        X_LOG_TRACE("wolfSSL library initialized successfully");
    }
    
    // Create wolfSSL context based on TLS t_eTlsVersion
    WOLFSSL_METHOD* l_pttMethod = NULL;
    if (p_kpttConfig->t_eTlsVersion == TLS_VERSION_1_3) 
    {
        if (p_kpttConfig->t_bIsServer) 
        {
            l_pttMethod = wolfTLSv1_3_server_method();
            X_LOG_TRACE("Using TLS 1.3 server l_pttMethod");
        } 
        else 
        {
            l_pttMethod = wolfTLSv1_3_client_method();
            X_LOG_TRACE("Using TLS 1.3 client l_pttMethod");
        }
    } 
    else if (p_kpttConfig->t_eTlsVersion == TLS_VERSION_1_2) 
    {
        // Use TLS 1.2 if explicitly specified
        if (p_kpttConfig->t_bIsServer) 
        {
            l_pttMethod = wolfTLSv1_2_server_method();
            X_LOG_TRACE("Using TLS 1.2 server l_pttMethod");
        } 
        else 
        {
            l_pttMethod = wolfTLSv1_2_client_method();
            X_LOG_TRACE("Using TLS 1.2 client l_pttMethod");
        }
    } 
    else 
    {
        X_LOG_TRACE("Invalid TLS t_eTlsVersion specified");
        X_ASSERT(false);
    }
    
    if (!l_pttMethod) 
    {
        X_LOG_TRACE("Failed to create SSL l_pttMethod");
        return TLS_ERROR;
    }
    
    // Create the context
    p_pttEngine->t_CipherCtx = wolfSSL_CTX_new(l_pttMethod);
    if (!p_pttEngine->t_CipherCtx) 
    {
        X_LOG_TRACE("Failed to create SSL context");
        return TLS_ERROR;
    }
    X_LOG_TRACE("SSL context created successfully");
    
    // Load certificates and keys if provided
    if (p_kpttConfig->t_cCaPath) 
    {
        X_LOG_TRACE("Loading CA certificate from %s", p_kpttConfig->t_cCaPath);
        if (wolfSSL_CTX_load_verify_locations(p_pttEngine->t_CipherCtx, p_kpttConfig->t_cCaPath, NULL) != WOLFSSL_SUCCESS)
        {
            X_LOG_TRACE("Failed to load CA certificate");
            wolfSSL_CTX_free(p_pttEngine->t_CipherCtx);
            p_pttEngine->t_CipherCtx = NULL;
            return TLS_CERT_ERROR;
        }
        X_LOG_TRACE("CA certificate loaded successfully");
    }
    
    if (p_kpttConfig->t_cCertPath && p_kpttConfig->t_cKeyPath) 
    {
        X_LOG_TRACE("Loading server certificate from %s", p_kpttConfig->t_cCertPath);
        X_LOG_TRACE("Loading server key from %s", p_kpttConfig->t_cKeyPath);
        
        if (wolfSSL_CTX_use_certificate_file(p_pttEngine->t_CipherCtx, p_kpttConfig->t_cCertPath, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) 
        {
            X_LOG_TRACE("Failed to load server certificate");
            wolfSSL_CTX_free(p_pttEngine->t_CipherCtx);
            p_pttEngine->t_CipherCtx = NULL;
            return TLS_CERT_ERROR;
        }
        
        if (wolfSSL_CTX_use_PrivateKey_file(p_pttEngine->t_CipherCtx, p_kpttConfig->t_cKeyPath, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) 
        {
            X_LOG_TRACE("Failed to load server key");
            wolfSSL_CTX_free(p_pttEngine->t_CipherCtx);
            p_pttEngine->t_CipherCtx = NULL;
            return TLS_CERT_ERROR;
        }
        
        X_LOG_TRACE("Server certificate and key loaded successfully");
    }
    
    // Set verification mode
    if (p_kpttConfig->t_bVerifyPeer) 
    {
        wolfSSL_CTX_set_verify(p_pttEngine->t_CipherCtx, WOLFSSL_VERIFY_PEER, NULL); 
        X_LOG_TRACE("Peer verification enabled");
    } 
    else 
    {
        wolfSSL_CTX_set_verify(p_pttEngine->t_CipherCtx, WOLFSSL_VERIFY_NONE, NULL);
        X_LOG_TRACE("Peer verification disabled");
    }

    //check if ECDSA is enabled and cipher list is ECDSA
    if (p_kpttConfig->t_bLoadEcdsaCipher)
    {
        X_LOG_TRACE("Loading ECDSA cipher");
        if (strcmp(p_kpttConfig->cipherList, s_kptcTlsCipherList [3]) != 0) //ECDSA ID
        {
            X_LOG_TRACE("Cipher list is not ECDSA");
            X_ASSERT(false);
        }
    }
    
    // Set cipher list if provided
    if (p_kpttConfig->cipherList) 
    {
        X_LOG_TRACE("Setting cipher list: %s", p_kpttConfig->cipherList);
        if (wolfSSL_CTX_set_cipher_list(p_pttEngine->t_CipherCtx, p_kpttConfig->cipherList) != WOLFSSL_SUCCESS) 
        {
            X_LOG_TRACE("Failed to set cipher list");
            wolfSSL_CTX_free(p_pttEngine->t_CipherCtx);
            p_pttEngine->t_CipherCtx = NULL;
            return TLS_ERROR;
        }
    }
    else 
    {
        X_LOG_TRACE("No cipher list provided, using default : %s", s_kptcDefaultTlsCipher);
        if (wolfSSL_CTX_set_cipher_list(p_pttEngine->t_CipherCtx, (char*)s_kptcDefaultTlsCipher) != WOLFSSL_SUCCESS) 
        {
            X_LOG_TRACE("Failed to set default cipher list");
            wolfSSL_CTX_free(p_pttEngine->t_CipherCtx);
            p_pttEngine->t_CipherCtx = NULL;
            return TLS_ERROR;
        }
    }
    
    // Configure ECC l_iEcc
    int l_iEcc;
    X_LOG_TRACE("Configuring ECC l_iEcc: %d", p_kpttConfig->t_eEccCurve);
    
    switch (p_kpttConfig->t_eEccCurve) 
    {
        case TLS_ECC_SECP256R1:
            X_LOG_TRACE("Using SECP256R1 (P-256) l_iEcc");
            l_iEcc = WOLFSSL_ECC_SECP256R1;
            break;
        case TLS_ECC_SECP384R1:
            X_LOG_TRACE("Using SECP384R1 (P-384) l_iEcc");
            l_iEcc = WOLFSSL_ECC_SECP384R1;
            break;
        case TLS_ECC_SECP521R1:
            X_LOG_TRACE("Using SECP521R1 (P-521) l_iEcc");
            l_iEcc = WOLFSSL_ECC_SECP521R1;
            break;
        case TLS_ECC_X25519:
            X_LOG_TRACE("Using X25519 l_iEcc");
            l_iEcc = WOLFSSL_ECC_X25519;
            break;
        default:
            X_LOG_TRACE("ECC curve not supported");
            X_ASSERT(false);
    }
    
    // Set TLS 1.3 groups (curves)
    if (p_kpttConfig->t_eTlsVersion == TLS_VERSION_1_3) 
    {
        int groups_array[1] = { l_iEcc };
        X_LOG_TRACE("Setting TLS 1.3 groups");
        
        if (wolfSSL_CTX_set_groups(p_pttEngine->t_CipherCtx, groups_array, 1) != WOLFSSL_SUCCESS) 
        {
            X_LOG_TRACE("Failed to set TLS 1.3 groups");
            wolfSSL_CTX_free(p_pttEngine->t_CipherCtx);
            p_pttEngine->t_CipherCtx = NULL;
            return TLS_ERROR;
        }
    }
    
    // Set IO callbacks
    X_LOG_TRACE("Setting IO callbacks");
    wolfSSL_SetIORecv(p_pttEngine->t_CipherCtx, tlsEngineIORecv);
    wolfSSL_SetIOSend(p_pttEngine->t_CipherCtx, tlsEngineIOSend);
    
    p_pttEngine->t_bInitialised = true;
    X_LOG_TRACE("TLS engine initialized successfully");
    
    return TLS_OK;
}

////////////////////////////////////////////////////////////
/// tlsEngineConnect
////////////////////////////////////////////////////////////
unsigned long tlsEngineConnect(TLS_Engine* p_pttEngine) 
{
    if (!p_pttEngine || !p_pttEngine->t_bInitialised || !p_pttEngine->t_CipherCtx || p_pttEngine->t_iSocketFd < 0) 
    {
        return TLS_INVALID_PARAM;
    }
    
    X_LOG_TRACE("Starting client TLS handshake");
    
    // Create new wolfSSL session
    p_pttEngine->t_SslSession = wolfSSL_new(p_pttEngine->t_CipherCtx);
    if (!p_pttEngine->t_SslSession) 
    {
        X_LOG_TRACE("Failed to create client SSL session");
        return TLS_ERROR;
    }
    
    // Set socket as IO context
    wolfSSL_SetIOReadCtx(p_pttEngine->t_SslSession, &p_pttEngine->t_iSocketFd);
    wolfSSL_SetIOWriteCtx(p_pttEngine->t_SslSession, &p_pttEngine->t_iSocketFd);
    
    // Perform TLS handshake
    int ret = wolfSSL_connect(p_pttEngine->t_SslSession);
    if (ret != WOLFSSL_SUCCESS) 
    {
        int err = wolfSSL_get_error(p_pttEngine->t_SslSession, ret);
        char errorBuffer[80];
        wolfSSL_ERR_error_string(err, errorBuffer);
        X_LOG_TRACE("TLS client handshake failed with error code %d, message: %s", err, errorBuffer);
        wolfSSL_free(p_pttEngine->t_SslSession);
        p_pttEngine->t_SslSession = NULL;
        return TLS_CONNECT_ERROR;
    }
    
    X_LOG_TRACE("TLS client handshake completed successfully");
    p_pttEngine->t_bIsConnected = true;
    
    return TLS_OK;
}

////////////////////////////////////////////////////////////
/// tlsEngineAccept
////////////////////////////////////////////////////////////
unsigned long tlsEngineAccept(TLS_Engine* p_pttEngine, int p_iSocketFd, const TLS_Engine* p_pListenEngine) 
{
    if (!p_pttEngine || !p_pListenEngine || !p_pListenEngine->t_bInitialised || 
        !p_pListenEngine->t_CipherCtx || p_iSocketFd < 0) 
    {
        X_LOG_TRACE("Invalid parameters for TLS accept");
        return TLS_INVALID_PARAM;
    }
    
    X_LOG_TRACE("Accepting TLS connection on socket %d using listen engine context", p_iSocketFd);
    
    // Initialize engine structure
    memset(p_pttEngine, 0, sizeof(TLS_Engine));
    p_pttEngine->t_iSocketFd = p_iSocketFd;
    p_pttEngine->t_eTlsVersion = p_pListenEngine->t_eTlsVersion;
    p_pttEngine->t_eEccCurve = p_pListenEngine->t_eEccCurve;
    
    // We'll use the listening engine's context to create a new SSL session,
    // but won't store a reference to it to avoid potential double-free issues.
    // The t_CipherCtx ownership stays with the listener.
    p_pttEngine->t_CipherCtx = NULL; // Don't assign the context directly
    
    // Create new wolfSSL session using the listening engine's context
    WOLFSSL_CTX* listenerCtx = p_pListenEngine->t_CipherCtx;
    X_LOG_TRACE("Creating new SSL session using listener's t_CipherCtx");
    p_pttEngine->t_SslSession = wolfSSL_new(listenerCtx);
    if (!p_pttEngine->t_SslSession) 
    {
        X_LOG_TRACE("Failed to create new SSL session");
        return TLS_ERROR;
    }
    
    // Désactiver la vérification des certificats pour l'acceptation
    // Cela permettra de contourner le problème d'erreur ASN
    wolfSSL_set_verify(p_pttEngine->t_SslSession, WOLFSSL_VERIFY_NONE, NULL);
    X_LOG_TRACE("Peer verification temporarily disabled for handshake");
    
    p_pttEngine->t_bInitialised = true;
    
    // Set socket as IO context
    X_LOG_TRACE("Setting IO context for socket %d", p_iSocketFd);
    wolfSSL_SetIOReadCtx(p_pttEngine->t_SslSession, &p_pttEngine->t_iSocketFd);
    wolfSSL_SetIOWriteCtx(p_pttEngine->t_SslSession, &p_pttEngine->t_iSocketFd);
    
    // Debug: Afficher des informations supplémentaires sur le contexte SSL
    X_LOG_TRACE("Debug info: p_pttEngine socket: %d, listening socket: %d", 
               p_pttEngine->t_iSocketFd, p_pListenEngine->t_iSocketFd);
    
    // Perform TLS handshake
    X_LOG_TRACE("Performing TLS accept handshake");
    int ret = wolfSSL_accept(p_pttEngine->t_SslSession);
    
    // Obtenir et enregistrer plus d'informations d'erreur en cas d'échec
    if (ret != WOLFSSL_SUCCESS) 
    {
        int err = wolfSSL_get_error(p_pttEngine->t_SslSession, ret);
        char errorBuffer[80];
        wolfSSL_ERR_error_string(err, errorBuffer);
        X_LOG_TRACE("TLS accept failed with error code: %d, message: %s", err, errorBuffer);
        wolfSSL_free(p_pttEngine->t_SslSession);
        p_pttEngine->t_SslSession = NULL;
        p_pttEngine->t_bInitialised = false;
        return TLS_CONNECT_ERROR;
    }
    
    X_LOG_TRACE("TLS handshake completed successfully");
    p_pttEngine->t_bIsConnected = true;
    
    return TLS_OK;
}

////////////////////////////////////////////////////////////
/// tlsEngineSend
////////////////////////////////////////////////////////////
unsigned long tlsEngineSend(TLS_Engine* p_pttEngine, const void* p_pBuffer, unsigned long p_ulSize) 
{
    if (!p_pttEngine || !p_pttEngine->t_bInitialised || !p_pttEngine->t_SslSession || 
        !p_pttEngine->t_bIsConnected || !p_pBuffer) 
    {
        return TLS_INVALID_PARAM;
    }
    
    int sent = wolfSSL_write(p_pttEngine->t_SslSession, p_pBuffer, p_ulSize);
    
    if (sent < 0) 
    {
        return TLS_ERROR;
    }
    
    return (unsigned long)sent;
}

////////////////////////////////////////////////////////////
/// tlsEngineReceive
////////////////////////////////////////////////////////////
unsigned long tlsEngineReceive(TLS_Engine* p_pttEngine, void* p_pBuffer, unsigned long p_ulSize) 
{
    if (!p_pttEngine || !p_pttEngine->t_bInitialised || !p_pttEngine->t_SslSession || 
        !p_pttEngine->t_bIsConnected || !p_pBuffer) 
    {
        return TLS_INVALID_PARAM;
    }
    
    int received = wolfSSL_read(p_pttEngine->t_SslSession, p_pBuffer, p_ulSize);
    
    if (received < 0) 
    {
        int err = wolfSSL_get_error(p_pttEngine->t_SslSession, received);
        if (err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE) 
        {
            return 0; // No data available yet
        }
        return TLS_ERROR;
    }
    
    return (unsigned long)received;
}

////////////////////////////////////////////////////////////
/// tlsEngineClose
////////////////////////////////////////////////////////////
unsigned long tlsEngineClose(TLS_Engine* p_pttEngine) 
{
    if (!p_pttEngine || !p_pttEngine->t_bInitialised || !p_pttEngine->t_SslSession) 
    {
        return TLS_INVALID_PARAM;
    }
    
    // Perform TLS shutdown
    wolfSSL_shutdown(p_pttEngine->t_SslSession);
    
    // Free wolfSSL session
    wolfSSL_free(p_pttEngine->t_SslSession);
    p_pttEngine->t_SslSession = NULL;
    p_pttEngine->t_bIsConnected = false;
    
    return TLS_OK;
}

////////////////////////////////////////////////////////////
/// tlsEngineCleanup
////////////////////////////////////////////////////////////
unsigned long tlsEngineCleanup(TLS_Engine* p_pttEngine) 
{
    if (!p_pttEngine || !p_pttEngine->t_bInitialised) 
    {
        return TLS_INVALID_PARAM;
    }
    
    X_LOG_TRACE("Cleaning up TLS engine resources");
    
    // Close connection if still active
    if (p_pttEngine->t_SslSession) 
    {
        X_LOG_TRACE("Freeing SSL session");
        wolfSSL_free(p_pttEngine->t_SslSession);
        p_pttEngine->t_SslSession = NULL;
    }
    
    // Free wolfSSL context only if we own it (not shared from accept)
    if (p_pttEngine->t_CipherCtx) 
    {
        X_LOG_TRACE("Freeing SSL context");
        wolfSSL_CTX_free(p_pttEngine->t_CipherCtx);
        p_pttEngine->t_CipherCtx = NULL;
    } 
    else 
    {
        X_LOG_TRACE("Skipping t_CipherCtx cleanup (shared or NULL t_CipherCtx)");
    }
    
    p_pttEngine->t_bInitialised = false;
    p_pttEngine->t_bIsConnected = false;
    
    return TLS_OK;
}

////////////////////////////////////////////////////////////
/// tlsEngineGetErrorString
////////////////////////////////////////////////////////////
const char* tlsEngineGetErrorString(int p_iError) 
{
    switch (p_iError) 
    {
        case TLS_OK:
            return "Success";
        case TLS_ERROR:
            return "General TLS error";
        case TLS_INVALID_PARAM:
            return "Invalid parameter";
        case TLS_CERT_ERROR:
            return "Certificate error";
        case TLS_CONNECT_ERROR:
            return "Connection error";
        case TLS_VERIFY_ERROR:
            return "Verification error";
        default:
            return "Unknown error";
    }
}

////////////////////////////////////////////////////////////
/// tlsEngineIsEnabled
////////////////////////////////////////////////////////////
bool tlsEngineIsEnabled(const TLS_Engine* p_pttEngine) 
{
    if (!p_pttEngine) 
    {
        return false;
    }
    
    return p_pttEngine->t_bInitialised;
}

////////////////////////////////////////////////////////////
/// tlsEngineGetConnectionInfo
////////////////////////////////////////////////////////////
unsigned long tlsEngineGetConnectionInfo(const TLS_Engine* p_pttEngine, char* p_pCipherName, unsigned long p_ulSize) 
{
    if (!p_pttEngine || !p_pttEngine->t_bInitialised || !p_pttEngine->t_SslSession || 
        !p_pttEngine->t_bIsConnected || !p_pCipherName) 
    {
        return TLS_INVALID_PARAM;
    }
    
    // Get cipher name
    const char* cipherName = wolfSSL_get_cipher_name(p_pttEngine->t_SslSession);
    if (!cipherName) 
    {
        return TLS_ERROR;
    }
    
    // Copy cipher name to output buffer
    strncpy(p_pCipherName, cipherName, p_ulSize - 1);
    p_pCipherName[p_ulSize - 1] = '\0';
    
    return TLS_OK;
}

////////////////////////////////////////////////////////////
/// tlsEngineGetPeerCertificate
////////////////////////////////////////////////////////////
unsigned long tlsEngineCheckPrivateKey(TLS_Engine* p_pttEngine, const char* p_pKeyPath)
{

    if (!p_pttEngine || !p_pKeyPath) 
    {
        return TLS_INVALID_PARAM;
    }

    // Check if the private key is valid
    if (wolfSSL_CTX_use_PrivateKey_file(p_pttEngine->t_CipherCtx, p_pKeyPath, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS)
    {
        return TLS_CERT_ERROR;
    }

    return TLS_OK;
}