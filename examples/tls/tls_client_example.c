////////////////////////////////////////////////////////////
//  TLS client example
//  Example of using xTlsEngine for a secure client
//  Uses xNetwork for connectivity and xTlsEngine for security
//
// general discloser: copy or share the file is forbidden
// Written : 30/03/2025
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypto/TLS/xTlsEngine.h>
#include <network/xNetwork.h>

// Example of a simple TLS client connecting to a server
int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 3) {
        printf("Usage: %s <server_address> <port>\n", argv[0]);
        return 1;
    }
    
    const char* serverAddress = argv[1];
    int serverPort = atoi(argv[2]);
    
    // Initialize the network
    NetworkContext* networkContext = networkInit();
    if (!networkContext) {
        printf("Failed to initialize network\n");
        return 1;
    }
    
    // Initialize TLS engine
    if (xTlsInit() != XOS_TLS_OK) {
        printf("Failed to initialize TLS engine\n");
        networkCleanup(networkContext);
        return 1;
    }
    
    // Create a TCP socket
    NetworkSocket* socket = networkCreateSocket(networkContext, NETWORK_SOCK_TCP, NETWORK_SOCK_BLOCKING);
    if (!socket) {
        printf("Failed to create socket\n");
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    // Connect to server
    NetworkAddress serverAddr = networkMakeAddress(serverAddress, serverPort);
    int result = networkConnect(socket, &serverAddr);
    if (result != NETWORK_OK) {
        printf("Failed to connect to server: %s\n", networkGetErrorString(result));
        networkCloseSocket(socket);
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    printf("Connected to server %s:%d\n", serverAddress, serverPort);
    
    // Create TLS configuration
    xos_tls_config_t tlsConfig;
    memset(&tlsConfig, 0, sizeof(tlsConfig));
    
    tlsConfig.t_eRole = XOS_TLS_CLIENT;
    tlsConfig.t_eVersion = XOS_TLS_VERSION_1_3_ONLY;  // Use TLS 1.3
    tlsConfig.t_eKeyExchange = XOS_TLS_KEX_STANDARD;  // Standard key exchange
    tlsConfig.t_eSigAlg = XOS_TLS_SIG_ECDSA;
    
    // Set hostname for SNI
    tlsConfig.t_pcHostname = serverAddress;
    
    // Optional: Use CA certificate for verification
    // tlsConfig.t_pcCaPath = "path/to/ca/cert.pem";
    
    // We'll skip server certificate verification for this example
    tlsConfig.t_bVerifyPeer = false;
    
    // Create TLS context
    xos_tls_ctx_t* tlsContext = xTlsCreateContext(&tlsConfig);
    if (!tlsContext) {
        printf("Failed to create TLS context\n");
        networkCloseSocket(socket);
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    // Create TLS session
    xos_tls_session_t* tlsSession = xTlsCreateSession(tlsContext, socket);
    if (!tlsSession) {
        printf("Failed to create TLS session\n");
        xTlsDestroyContext(tlsContext);
        networkCloseSocket(socket);
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    // Perform TLS handshake
    result = xTlsHandshake(tlsSession);
    if (result != XOS_TLS_OK) {
        printf("TLS handshake failed: %s\n", xTlsGetErrorString(result));
        xTlsDestroySession(tlsSession);
        xTlsDestroyContext(tlsContext);
        networkCloseSocket(socket);
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    printf("TLS handshake successful\n");
    
    // Get connection info
    char cipherName[128];
    char versionName[128];
    result = xTlsGetConnectionInfo(tlsSession, cipherName, sizeof(cipherName), 
                                 versionName, sizeof(versionName));
    
    if (result == XOS_TLS_OK) {
        printf("TLS version: %s\nCipher suite: %s\n", versionName, cipherName);
    }
    
    // Send a simple HTTP GET request
    const char* httpRequest = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    result = xTlsSend(tlsSession, httpRequest, strlen(httpRequest));
    
    if (result <= 0) {
        printf("Failed to send request: %s\n", xTlsGetErrorString(result));
        xTlsDestroySession(tlsSession);
        xTlsDestroyContext(tlsContext);
        networkCloseSocket(socket);
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    printf("Request sent successfully\n");
    
    // Receive response
    char buffer[1024];
    bool receivedData = false;
    
    while (1) {
        result = xTlsReceive(tlsSession, buffer, sizeof(buffer) - 1);
        
        if (result == XOS_TLS_CONNECTION_CLOSED) {
            // Connection closed by peer
            break;
        } else if (result < 0) {
            printf("Receive error: %s\n", xTlsGetErrorString(result));
            break;
        } else if (result > 0) {
            // Null-terminate and print received data
            buffer[result] = '\0';
            printf("Received %d bytes:\n%s\n", result, buffer);
            receivedData = true;
        }
    }
    
    if (!receivedData) {
        printf("No data received from server\n");
    }
    
    // Clean up
    xTlsDestroySession(tlsSession);
    xTlsDestroyContext(tlsContext);
    networkCloseSocket(socket);
    xTlsCleanup();
    networkCleanup(networkContext);
    
    printf("Connection closed\n");
    
    return 0;
} 