////////////////////////////////////////////////////////////
//  Network Keep-Alive example usage
//  Demonstrates how to use the keep-alive mechanism
//  
// general disclosure: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xNetworkKeepAlive.h"
#include <stdio.h>
#include <signal.h>

// Global flag for main loop
static volatile bool g_bRunning = true;

// Signal handler for clean shutdown
void signalHandler(int signum) 
{
    printf("Signal %d received, shutting down...\n", signum);
    g_bRunning = false;
}

// Keep-alive callback function
void keepAliveCallback(NetworkKeepAlive* p_pKeepAlive, int p_iEvent, void* p_pUserData) 
{
    const char* l_pcUserData = (const char*)p_pUserData;
    
    switch (p_iEvent) {
        case KEEPALIVE_EVENT_SENT:
            printf("[%s] Keep-alive probe sent\n", l_pcUserData);
            break;
            
        case KEEPALIVE_EVENT_RECEIVED:
            printf("[%s] Keep-alive response received\n", l_pcUserData);
            break;
            
        case KEEPALIVE_EVENT_TIMEOUT:
            printf("[%s] Keep-alive timed out, retrying...\n", l_pcUserData);
            break;
            
        case KEEPALIVE_EVENT_FAILED:
            printf("[%s] Keep-alive failed after max retries\n", l_pcUserData);
            break;
            
        case KEEPALIVE_EVENT_RECOVERED:
            printf("[%s] Keep-alive connection recovered\n", l_pcUserData);
            break;
            
        default:
            printf("[%s] Unknown keep-alive event: %d\n", l_pcUserData, p_iEvent);
            break;
    }
}

int main(int argc, char* argv[]) 
{
    // Set signal handlers for clean shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Check command line arguments
    if (argc < 3) 
    {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    
    const char* l_pcServerIP = argv[1];
    unsigned short l_usPort = (unsigned short)atoi(argv[2]);
    
    // Initialize network
    printf("Initializing network...\n");
    NetworkContext* l_pNetworkContext = networkInit();
    if (!l_pNetworkContext) {
        printf("Failed to initialize network\n");
        return 1;
    }
    
    // Create TCP socket
    printf("Creating TCP socket...\n");
    NetworkSocket* l_pSocket = networkCreateSocket(l_pNetworkContext, NETWORK_SOCK_TCP, NETWORK_SOCK_BLOCKING);
    if (!l_pSocket) {
        printf("Failed to create socket\n");
        networkCleanup(l_pNetworkContext);
        return 1;
    }
    
    // Connect to server
    printf("Connecting to %s:%d...\n", l_pcServerIP, l_usPort);
    NetworkAddress l_tServerAddr = networkMakeAddress(l_pcServerIP, l_usPort);
    if (networkConnect(l_pSocket, &l_tServerAddr) != NETWORK_OK) {
        printf("Failed to connect to server\n");
        networkCloseSocket(l_pSocket);
        networkCleanup(l_pNetworkContext);
        return 1;
    }
    
    printf("Connected to server\n");
    
    // Initialize keep-alive with default settings
    printf("Initializing keep-alive...\n");
    NetworkKeepAlive* l_pKeepAlive = networkKeepAliveInit(l_pSocket, 
                                                       30,  // 30 second interval
                                                       5,   // 5 second timeout
                                                       3);  // 3 retries
    if (!l_pKeepAlive) {
        printf("Failed to initialize keep-alive\n");
        networkCloseSocket(l_pSocket);
        networkCleanup(l_pNetworkContext);
        return 1;
    }
    
    // Set keep-alive callback
    static const char l_cCallbackId[] = "CLIENT";
    networkKeepAliveSetCallback(l_pKeepAlive, keepAliveCallback, (void*)l_cCallbackId);
    
    // Custom keep-alive data (optional)
    const char l_cCustomKeepAlive[] = "PING";
    
    // Start keep-alive
    printf("Starting keep-alive...\n");
    if (networkKeepAliveStart(l_pKeepAlive, l_cCustomKeepAlive, sizeof(l_cCustomKeepAlive) - 1) != NETWORK_OK) {
        printf("Failed to start keep-alive\n");
        networkKeepAliveCleanup(l_pKeepAlive);
        networkCloseSocket(l_pSocket);
        networkCleanup(l_pNetworkContext);
        return 1;
    }
    
    printf("Keep-alive started with 30 second interval\n");
    
    // Buffer for receiving data
    char l_cBuffer[NETWORK_BUFFER_SIZE];
    
    // Main loop
    printf("Entering main loop, press Ctrl+C to exit\n");
    while (g_bRunning) {
        // Receive data
        int l_iReceived = networkReceive(l_pSocket, l_cBuffer, sizeof(l_cBuffer));
        
        if (l_iReceived > 0) {
            // Process received data
            l_cBuffer[l_iReceived] = '\0'; // Null-terminate for printing
            printf("Received: %s\n", l_cBuffer);
            
            // Check if this is a keep-alive response (in a real application, 
            // you would need a more sophisticated protocol)
            if (strcmp(l_cBuffer, "PONG") == 0) {
                // Process as keep-alive response
                networkKeepAliveProcessResponse(l_pKeepAlive, l_cBuffer, l_iReceived);
            }
        } else if (l_iReceived == NETWORK_DISCONNECTED) {
            printf("Disconnected from server\n");
            break;
        } else if (l_iReceived == NETWORK_ERROR) {
            printf("Network error\n");
            break;
        }
        
        // Short sleep to avoid busy waiting
        sleep(1);
    }
    
    // Cleanup
    printf("Stopping keep-alive...\n");
    networkKeepAliveStop(l_pKeepAlive);
    
    printf("Cleaning up resources...\n");
    networkKeepAliveCleanup(l_pKeepAlive);
    networkCloseSocket(l_pSocket);
    networkCleanup(l_pNetworkContext);
    
    printf("Exiting\n");
    
    return 0;
} 