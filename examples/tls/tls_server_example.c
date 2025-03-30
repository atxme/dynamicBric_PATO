////////////////////////////////////////////////////////////
//  TLS server example
//  Example of using xTlsEngine for a secure server
//  Uses xNetwork for connectivity and xTlsEngine for security
//
// general discloser: copy or share the file is forbidden
// Written : 30/03/2025
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <crypto/TLS/xTlsEngine.h>
#include <network/xNetwork.h>

// Global flag for termination
volatile int g_bRunning = 1;

// Signal handler for clean shutdown
void handleSignal(int sig) {
    g_bRunning = 0;
    printf("\nReceived signal %d, shutting down...\n", sig);
}

// Simple handler for TLS connections
void handleClient(xos_tls_session_t* tlsSession) {
    if (!tlsSession) {
        return;
    }
    
    // Buffer for incoming data
    char buffer[4096];
    int result;
    bool receivedRequest = false;
    
    // Simple HTTP response
    const char* httpResponse =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html>\r\n"
        "<html>\r\n"
        "<head><title>TLS Server Example</title></head>\r\n"
        "<body>\r\n"
        "<h1>Secure TLS Server</h1>\r\n"
        "<p>This server uses TLS 1.3 with post-quantum support.</p>\r\n"
        "</body>\r\n"
        "</html>\r\n";
    
    // Wait for client request
    while (g_bRunning) {
        result = xTlsReceive(tlsSession, buffer, sizeof(buffer) - 1);
        
        if (result == XOS_TLS_CONNECTION_CLOSED) {
            printf("Connection closed by client\n");
            break;
        } else if (result < 0) {
            if (result == XOS_TLS_WOULD_BLOCK) {
                // For non-blocking sockets, we would handle this differently
                continue;
            }
            printf("Receive error: %s\n", xTlsGetErrorString(result));
            break;
        } else if (result > 0) {
            // We received some data
            buffer[result] = '\0';
            printf("Received request (%d bytes):\n%s\n", result, buffer);
            receivedRequest = true;
            break;  // In a real server, you might continue reading
        }
    }
    
    // If we received a request, send a response
    if (receivedRequest) {
        result = xTlsSend(tlsSession, httpResponse, strlen(httpResponse));
        
        if (result <= 0) {
            printf("Failed to send response: %s\n", xTlsGetErrorString(result));
        } else {
            printf("Response sent successfully (%d bytes)\n", result);
        }
    }
}

// Example of a simple TLS server
int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 4) {
        printf("Usage: %s <listen_address> <port> <certificate> <private_key>\n", argv[0]);
        return 1;
    }
    
    const char* listenAddress = argv[1];
    int listenPort = atoi(argv[2]);
    const char* certPath = argv[3];
    const char* keyPath = (argc > 4) ? argv[4] : argv[3];  // Allow certificate and key to be in the same file
    
    // Set up signal handling
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    
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
    
    // Create TLS configuration
    xos_tls_config_t tlsConfig;
    memset(&tlsConfig, 0, sizeof(tlsConfig));
    
    tlsConfig.t_eRole = XOS_TLS_SERVER;
    tlsConfig.t_eVersion = XOS_TLS_VERSION_1_3_ONLY;  // Use TLS 1.3
    tlsConfig.t_eKeyExchange = XOS_TLS_KEX_STANDARD;  // Standard key exchange
    tlsConfig.t_eSigAlg = XOS_TLS_SIG_ECDSA;
    
    // Set certificate and key paths
    tlsConfig.t_pcCertPath = certPath;
    tlsConfig.t_pcKeyPath = keyPath;
    
    // Don't require client auth for this example
    tlsConfig.t_bRequireAuth = false;
    
    // Create TLS context
    xos_tls_ctx_t* tlsContext = xTlsCreateContext(&tlsConfig);
    if (!tlsContext) {
        printf("Failed to create TLS context\n");
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    // Create server socket
    NetworkSocket* serverSocket = networkCreateSocket(networkContext, NETWORK_SOCK_TCP, NETWORK_SOCK_BLOCKING);
    if (!serverSocket) {
        printf("Failed to create server socket\n");
        xTlsDestroyContext(tlsContext);
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    // Bind to address and port
    NetworkAddress bindAddr = networkMakeAddress(listenAddress, listenPort);
    int result = networkBind(serverSocket, &bindAddr);
    if (result != NETWORK_OK) {
        printf("Failed to bind: %s\n", networkGetErrorString(result));
        networkCloseSocket(serverSocket);
        xTlsDestroyContext(tlsContext);
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    // Start listening
    result = networkListen(serverSocket, 5);
    if (result != NETWORK_OK) {
        printf("Failed to listen: %s\n", networkGetErrorString(result));
        networkCloseSocket(serverSocket);
        xTlsDestroyContext(tlsContext);
        xTlsCleanup();
        networkCleanup(networkContext);
        return 1;
    }
    
    printf("Server listening on %s:%d\n", listenAddress, listenPort);
    printf("Press Ctrl+C to stop the server\n");
    
    // Main server loop
    while (g_bRunning) {
        NetworkAddress clientAddr;
        NetworkSocket* clientSocket = networkAccept(serverSocket, &clientAddr);
        
        if (!clientSocket) {
            // No client or error
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", clientAddr.t_cAddress, clientAddr.t_usPort);
        
        // Create TLS session for this client
        xos_tls_session_t* tlsSession = xTlsCreateSession(tlsContext, clientSocket);
        if (!tlsSession) {
            printf("Failed to create TLS session\n");
            networkCloseSocket(clientSocket);
            continue;
        }
        
        // Perform TLS handshake
        result = xTlsHandshake(tlsSession);
        if (result != XOS_TLS_OK) {
            printf("TLS handshake failed: %s\n", xTlsGetErrorString(result));
            xTlsDestroySession(tlsSession);
            networkCloseSocket(clientSocket);
            continue;
        }
        
        printf("TLS handshake successful\n");
        
        // Get connection info
        char cipherName[128];
        char versionName[128];
        if (xTlsGetConnectionInfo(tlsSession, cipherName, sizeof(cipherName), 
                                versionName, sizeof(versionName)) == XOS_TLS_OK) {
            printf("TLS version: %s\nCipher suite: %s\n", versionName, cipherName);
        }
        
        // Handle client connection
        handleClient(tlsSession);
        
        // Clean up client
        xTlsDestroySession(tlsSession);
        networkCloseSocket(clientSocket);
        printf("Client connection closed\n");
    }
    
    // Clean up server
    networkCloseSocket(serverSocket);
    xTlsDestroyContext(tlsContext);
    xTlsCleanup();
    networkCleanup(networkContext);
    
    printf("Server shut down\n");
    
    return 0;
} 