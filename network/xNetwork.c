////////////////////////////////////////////////////////////
//  network core implementation file for embedded systems
//  Implements the network interface defined in network_core.h
//  IPv4-focused, thread-safe with TLS extension points
//  Memory-optimized for embedded environments
//
// general disclosure: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xNetwork.h"

//////////////////////////////////
/// Internal structures
//////////////////////////////////

// Socket structure with fixed array allocation
struct NetworkSocket {
    int t_iSocketFd;             // Socket file descriptor
    int t_iType;                 // Socket type (TCP/UDP)
    bool t_bNonBlocking;         // Non-blocking mode flag
    bool t_bConnected;           // Connection state
    bool t_bInUse;               // Whether this slot is in use
    pthread_mutex_t t_tMutex;    // Per-socket mutex for thread safety
};

// Network context with pre-allocated pools
struct NetworkContext {
    bool t_bInitialized;                               // Initialization flag
    struct NetworkSocket t_tSockets[NETWORK_MAX_SOCKETS]; // Socket pool
    pthread_mutex_t t_tPoolMutex;                      // Pool access mutex
};

//////////////////////////////////
/// Static (internal) helper functions
//////////////////////////////////

// Set socket to non-blocking mode
static int _setNonBlocking(int p_iSocketFd, bool p_bNonBlocking) {
    int l_iFlags = fcntl(p_iSocketFd, F_GETFL, 0);
    if (l_iFlags < 0) return NETWORK_ERROR;
    
    if (p_bNonBlocking)
        l_iFlags |= O_NONBLOCK;
    else
        l_iFlags &= ~O_NONBLOCK;
        
    if (fcntl(p_iSocketFd, F_SETFL, l_iFlags) < 0)
        return NETWORK_ERROR;
        
    return NETWORK_OK;
}

// Safe socket locking with error checking
static int _lockSocket(NetworkSocket* p_pSocket) {
    if (!p_pSocket || !p_pSocket->t_bInUse) 
        return NETWORK_INVALID_SOCKET;
    
    if (pthread_mutex_lock(&p_pSocket->t_tMutex) != 0)
        return NETWORK_MUTEX_ERROR;
    
    return NETWORK_OK;
}

// Safe socket unlocking with error checking
static int _unlockSocket(NetworkSocket* p_pSocket) {
    if (!p_pSocket || !p_pSocket->t_bInUse)
        return NETWORK_INVALID_SOCKET;
    
    if (pthread_mutex_unlock(&p_pSocket->t_tMutex) != 0)
        return NETWORK_MUTEX_ERROR;
    
    return NETWORK_OK;
}

// Get a free socket from the pool
static NetworkSocket* _getFreeSocket(NetworkContext* p_pContext) {
    if (!p_pContext || !p_pContext->t_bInitialized)
        return NULL;
    
    // Lock the pool mutex
    if (pthread_mutex_lock(&p_pContext->t_tPoolMutex) != 0)
        return NULL;
    
    // Find first free socket
    NetworkSocket* l_pSocket = NULL;
    for (int i = 0; i < NETWORK_MAX_SOCKETS; i++) {
        if (!p_pContext->t_tSockets[i].t_bInUse) {
            l_pSocket = &p_pContext->t_tSockets[i];
            l_pSocket->t_bInUse = true;
            break;
        }
    }
    
    // Unlock the pool mutex
    pthread_mutex_unlock(&p_pContext->t_tPoolMutex);
    
    return l_pSocket; // NULL if no free socket found
}

//////////////////////////////////
/// Core API Implementation
//////////////////////////////////

// Initialize network context
NetworkContext* networkInit(void) {
    // Static allocation to avoid malloc in embedded systems
    static struct NetworkContext s_tNetworkContext;
    
    // Check if already initialized
    if (s_tNetworkContext.t_bInitialized)
        return &s_tNetworkContext;
    
    // Initialize mutex
    if (pthread_mutex_init(&s_tNetworkContext.t_tPoolMutex, NULL) != 0)
        return NULL;
    
    // Initialize all sockets in the pool
    for (int i = 0; i < NETWORK_MAX_SOCKETS; i++) {
        s_tNetworkContext.t_tSockets[i].t_iSocketFd = -1;
        s_tNetworkContext.t_tSockets[i].t_bInUse = false;
        s_tNetworkContext.t_tSockets[i].t_bConnected = false;
        
        if (pthread_mutex_init(&s_tNetworkContext.t_tSockets[i].t_tMutex, NULL) != 0) {
            // Clean up on failure
            for (int j = 0; j < i; j++) {
                pthread_mutex_destroy(&s_tNetworkContext.t_tSockets[j].t_tMutex);
            }
            pthread_mutex_destroy(&s_tNetworkContext.t_tPoolMutex);
            return NULL;
        }
    }
    
    s_tNetworkContext.t_bInitialized = true;
    return &s_tNetworkContext;
}

// Clean up resources
int networkCleanup(NetworkContext* p_pContext) {
    if (!p_pContext || !p_pContext->t_bInitialized)
        return NETWORK_INVALID_PARAM;
    
    // Close all active sockets
    for (int i = 0; i < NETWORK_MAX_SOCKETS; i++) {
        if (p_pContext->t_tSockets[i].t_bInUse) {
            if (p_pContext->t_tSockets[i].t_iSocketFd >= 0) {
                close(p_pContext->t_tSockets[i].t_iSocketFd);
                p_pContext->t_tSockets[i].t_iSocketFd = -1;
            }
            p_pContext->t_tSockets[i].t_bInUse = false;
            p_pContext->t_tSockets[i].t_bConnected = false;
            pthread_mutex_destroy(&p_pContext->t_tSockets[i].t_tMutex);
        }
    }
    
    // Clean up pool mutex
    pthread_mutex_destroy(&p_pContext->t_tPoolMutex);
    
    p_pContext->t_bInitialized = false;
    return NETWORK_OK;
}

// Create a socket
NetworkSocket* networkCreateSocket(NetworkContext* p_pContext, int p_iType, int p_iBlocking) {
    if (!p_pContext || !p_pContext->t_bInitialized)
        return NULL;
    
    if (p_iType != NETWORK_SOCK_TCP && p_iType != NETWORK_SOCK_UDP)
        return NULL;
    
    // Get a free socket from the pool
    NetworkSocket* l_pSocket = _getFreeSocket(p_pContext);
    if (!l_pSocket)
        return NULL;
    
    // Create the actual socket
    l_pSocket->t_iSocketFd = socket(AF_INET, p_iType, 0);
    if (l_pSocket->t_iSocketFd < 0) {
        l_pSocket->t_bInUse = false;
        return NULL;
    }
    
    // Enable address reuse
    int l_iOption = 1;
    setsockopt(l_pSocket->t_iSocketFd, SOL_SOCKET, SO_REUSEADDR, &l_iOption, sizeof(l_iOption));
    
    // Set socket properties
    l_pSocket->t_iType = p_iType;
    l_pSocket->t_bNonBlocking = (p_iBlocking == NETWORK_SOCK_NONBLOCKING);
    l_pSocket->t_bConnected = false;
    
    // Set non-blocking mode if needed
    if (l_pSocket->t_bNonBlocking) {
        if (_setNonBlocking(l_pSocket->t_iSocketFd, true) != NETWORK_OK) {
            close(l_pSocket->t_iSocketFd);
            l_pSocket->t_iSocketFd = -1;
            l_pSocket->t_bInUse = false;
            return NULL;
        }
    }
    
    return l_pSocket;
}

// Create network address
NetworkAddress networkMakeAddress(const char* p_pcAddress, unsigned short p_usPort) {
    NetworkAddress l_tAddress;
    memset(&l_tAddress, 0, sizeof(NetworkAddress));
    
    l_tAddress.t_usPort = p_usPort;
    
    // Set IP address
    if (p_pcAddress && strlen(p_pcAddress) > 0) {
        // Validate IPv4 address format
        struct in_addr l_tAddr;
        if (inet_pton(AF_INET, p_pcAddress, &l_tAddr) == 1) {
            strncpy(l_tAddress.t_cAddress, p_pcAddress, INET_ADDRSTRLEN - 1);
        } else {
            // Use default if invalid
            strcpy(l_tAddress.t_cAddress, "0.0.0.0");
        }
    } else {
        // Use any address
        strcpy(l_tAddress.t_cAddress, "0.0.0.0");
    }
    
    return l_tAddress;
}

// Bind socket to address
int networkBind(NetworkSocket* p_pSocket, const NetworkAddress* p_pAddress) {
    if (!p_pSocket || !p_pSocket->t_bInUse || !p_pAddress)
        return NETWORK_INVALID_PARAM;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set up address structure
    struct sockaddr_in l_tAddr;
    memset(&l_tAddr, 0, sizeof(l_tAddr));
    l_tAddr.sin_family = AF_INET;
    l_tAddr.sin_port = htons(p_pAddress->t_usPort);
    
    if (strcmp(p_pAddress->t_cAddress, "0.0.0.0") == 0) {
        l_tAddr.sin_addr.s_addr = INADDR_ANY;
    } else if (inet_pton(AF_INET, p_pAddress->t_cAddress, &l_tAddr.sin_addr) != 1) {
        _unlockSocket(p_pSocket);
        return NETWORK_INVALID_ADDRESS;
    }
    
    // Bind socket
    if (bind(p_pSocket->t_iSocketFd, (struct sockaddr*)&l_tAddr, sizeof(l_tAddr)) < 0) {
        _unlockSocket(p_pSocket);
        return NETWORK_ERROR;
    }
    
    _unlockSocket(p_pSocket);
    return NETWORK_OK;
}

// Listen for connections
int networkListen(NetworkSocket* p_pSocket, int p_iBacklog) {
    if (!p_pSocket || !p_pSocket->t_bInUse)
        return NETWORK_INVALID_SOCKET;
    
    if (p_pSocket->t_iType != NETWORK_SOCK_TCP)
        return NETWORK_INVALID_PARAM; // Only TCP sockets can listen
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set backlog to default if not specified
    if (p_iBacklog <= 0)
        p_iBacklog = NETWORK_MAX_PENDING;
    
    // Start listening
    if (listen(p_pSocket->t_iSocketFd, p_iBacklog) < 0) {
        _unlockSocket(p_pSocket);
        return NETWORK_ERROR;
    }
    
    _unlockSocket(p_pSocket);
    return NETWORK_OK;
}

// Accept connection
NetworkSocket* networkAccept(NetworkSocket* p_pSocket, NetworkAddress* p_pClientAddress) {
    if (!p_pSocket || !p_pSocket->t_bInUse)
        return NULL;
    
    // Lock socket
    if (_lockSocket(p_pSocket) != NETWORK_OK)
        return NULL;
    
    // Accept incoming connection
    struct sockaddr_in l_tClientAddr;
    socklen_t l_iAddrLen = sizeof(l_tClientAddr);
    
    int l_iClientFd = accept(p_pSocket->t_iSocketFd, (struct sockaddr*)&l_tClientAddr, &l_iAddrLen);
    
    _unlockSocket(p_pSocket);
    
    if (l_iClientFd < 0) {
        if (p_pSocket->t_bNonBlocking && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // No connection available on non-blocking socket
            return NULL;
        }
        return NULL;
    }
    
    // Get context from the server socket handle
    // In an embedded system with static allocation, we can use the same context
    NetworkContext* l_pContext = networkInit();
    
    // Get a socket for the new connection
    NetworkSocket* l_pClientSocket = _getFreeSocket(l_pContext);
    if (!l_pClientSocket) {
        close(l_iClientFd);
        return NULL;
    }
    
    // Set up the new socket
    l_pClientSocket->t_iSocketFd = l_iClientFd;
    l_pClientSocket->t_iType = p_pSocket->t_iType;
    l_pClientSocket->t_bNonBlocking = p_pSocket->t_bNonBlocking;
    l_pClientSocket->t_bConnected = true;
    
    // Set non-blocking mode if needed
    if (l_pClientSocket->t_bNonBlocking) {
        _setNonBlocking(l_pClientSocket->t_iSocketFd, true);
    }
    
    // Fill client address if requested
    if (p_pClientAddress) {
        inet_ntop(AF_INET, &l_tClientAddr.sin_addr, 
                  p_pClientAddress->t_cAddress, INET_ADDRSTRLEN);
        p_pClientAddress->t_usPort = ntohs(l_tClientAddr.sin_port);
    }
    
    return l_pClientSocket;
}

// Connect to server
int networkConnect(NetworkSocket* p_pSocket, const NetworkAddress* p_pAddress) {
    if (!p_pSocket || !p_pSocket->t_bInUse || !p_pAddress)
        return NETWORK_INVALID_PARAM;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set up address structure
    struct sockaddr_in l_tAddr;
    memset(&l_tAddr, 0, sizeof(l_tAddr));
    l_tAddr.sin_family = AF_INET;
    l_tAddr.sin_port = htons(p_pAddress->t_usPort);
    
    if (inet_pton(AF_INET, p_pAddress->t_cAddress, &l_tAddr.sin_addr) != 1) {
        _unlockSocket(p_pSocket);
        return NETWORK_INVALID_ADDRESS;
    }
    
    // Connect
    int l_iConnectResult = connect(p_pSocket->t_iSocketFd, 
                                  (struct sockaddr*)&l_tAddr, sizeof(l_tAddr));
    
    if (l_iConnectResult < 0) {
        if (p_pSocket->t_bNonBlocking && (errno == EINPROGRESS || errno == EAGAIN)) {
            _unlockSocket(p_pSocket);
            return NETWORK_WOULD_BLOCK;
        }
        
        _unlockSocket(p_pSocket);
        return NETWORK_ERROR;
    }
    
    p_pSocket->t_bConnected = true;
    _unlockSocket(p_pSocket);
    
    return NETWORK_OK;
}

// Send data
int networkSend(NetworkSocket* p_pSocket, const void* p_pBuffer, unsigned long p_ulSize) {
    if (!p_pSocket || !p_pSocket->t_bInUse || !p_pBuffer)
        return NETWORK_INVALID_PARAM;
    
    if (p_ulSize == 0)
        return NETWORK_INVALID_SIZE;
    
    // Limit size for embedded systems
    if (p_ulSize > NETWORK_BUFFER_SIZE)
        p_ulSize = NETWORK_BUFFER_SIZE;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    if (!p_pSocket->t_bConnected) {
        _unlockSocket(p_pSocket);
        return NETWORK_DISCONNECTED;
    }
    
    // Send data
    ssize_t l_iSent = send(p_pSocket->t_iSocketFd, p_pBuffer, p_ulSize, 0);
    
    _unlockSocket(p_pSocket);
    
    if (l_iSent < 0) {
        if (p_pSocket->t_bNonBlocking && (errno == EAGAIN || errno == EWOULDBLOCK))
            return NETWORK_WOULD_BLOCK;
        return NETWORK_ERROR;
    }
    
    return (int)l_iSent;
}

// Receive data
int networkReceive(NetworkSocket* p_pSocket, void* p_pBuffer, unsigned long p_ulSize) {
    if (!p_pSocket || !p_pSocket->t_bInUse || !p_pBuffer)
        return NETWORK_INVALID_PARAM;
    
    if (p_ulSize == 0)
        return NETWORK_INVALID_SIZE;
    
    // Limit size for embedded systems
    if (p_ulSize > NETWORK_BUFFER_SIZE)
        p_ulSize = NETWORK_BUFFER_SIZE;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    if (!p_pSocket->t_bConnected) {
        _unlockSocket(p_pSocket);
        return NETWORK_DISCONNECTED;
    }
    
    // Receive data
    ssize_t l_iReceived = recv(p_pSocket->t_iSocketFd, p_pBuffer, p_ulSize, 0);
    
    if (l_iReceived == 0) {
        // Connection closed
        p_pSocket->t_bConnected = false;
    }
    
    _unlockSocket(p_pSocket);
    
    if (l_iReceived < 0) {
        if (p_pSocket->t_bNonBlocking && (errno == EAGAIN || errno == EWOULDBLOCK))
            return NETWORK_WOULD_BLOCK;
        return NETWORK_ERROR;
    }
    
    return (int)l_iReceived;
}

// Send data to specific address (UDP)
int networkSendTo(NetworkSocket* p_pSocket, const void* p_pBuffer, 
                 unsigned long p_ulSize, const NetworkAddress* p_pAddress) {
    if (!p_pSocket || !p_pSocket->t_bInUse || !p_pBuffer || !p_pAddress)
        return NETWORK_INVALID_PARAM;
    
    if (p_pSocket->t_iType != NETWORK_SOCK_UDP)
        return NETWORK_INVALID_PARAM; // Only UDP sockets can use sendto
    
    if (p_ulSize == 0)
        return NETWORK_INVALID_SIZE;
    
    // Limit size for embedded systems
    if (p_ulSize > NETWORK_BUFFER_SIZE)
        p_ulSize = NETWORK_BUFFER_SIZE;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set up address structure
    struct sockaddr_in l_tAddr;
    memset(&l_tAddr, 0, sizeof(l_tAddr));
    l_tAddr.sin_family = AF_INET;
    l_tAddr.sin_port = htons(p_pAddress->t_usPort);
    
    if (inet_pton(AF_INET, p_pAddress->t_cAddress, &l_tAddr.sin_addr) != 1) {
        _unlockSocket(p_pSocket);
        return NETWORK_INVALID_ADDRESS;
    }
    
    // Send data
    ssize_t l_iSent = sendto(p_pSocket->t_iSocketFd, p_pBuffer, p_ulSize, 0,
                           (struct sockaddr*)&l_tAddr, sizeof(l_tAddr));
    
    _unlockSocket(p_pSocket);
    
    if (l_iSent < 0) {
        if (p_pSocket->t_bNonBlocking && (errno == EAGAIN || errno == EWOULDBLOCK))
            return NETWORK_WOULD_BLOCK;
        return NETWORK_ERROR;
    }
    
    return (int)l_iSent;
}

// Receive data with sender info (UDP)
int networkReceiveFrom(NetworkSocket* p_pSocket, void* p_pBuffer, 
                      unsigned long p_ulSize, NetworkAddress* p_pAddress) {
    if (!p_pSocket || !p_pSocket->t_bInUse || !p_pBuffer)
        return NETWORK_INVALID_PARAM;
    
    if (p_pSocket->t_iType != NETWORK_SOCK_UDP)
        return NETWORK_INVALID_PARAM; // Only UDP sockets can use recvfrom
    
    if (p_ulSize == 0)
        return NETWORK_INVALID_SIZE;
    
    // Limit size for embedded systems
    if (p_ulSize > NETWORK_BUFFER_SIZE)
        p_ulSize = NETWORK_BUFFER_SIZE;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set up address structure
    struct sockaddr_in l_tAddr;
    socklen_t l_iAddrLen = sizeof(l_tAddr);
    
    // Receive data
    ssize_t l_iReceived = recvfrom(p_pSocket->t_iSocketFd, p_pBuffer, p_ulSize, 0,
                                 (struct sockaddr*)&l_tAddr, &l_iAddrLen);
    
    // Fill sender address if requested
    if (p_pAddress && l_iReceived > 0) {
        inet_ntop(AF_INET, &l_tAddr.sin_addr, p_pAddress->t_cAddress, INET_ADDRSTRLEN);
        p_pAddress->t_usPort = ntohs(l_tAddr.sin_port);
    }
    
    _unlockSocket(p_pSocket);
    
    if (l_iReceived < 0) {
        if (p_pSocket->t_bNonBlocking && (errno == EAGAIN || errno == EWOULDBLOCK))
            return NETWORK_WOULD_BLOCK;
        return NETWORK_ERROR;
    }
    
    return (int)l_iReceived;
}

// Close socket
int networkCloseSocket(NetworkSocket* p_pSocket) {
    if (!p_pSocket || !p_pSocket->t_bInUse)
        return NETWORK_INVALID_SOCKET;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Close the socket
    if (p_pSocket->t_iSocketFd >= 0) {
        close(p_pSocket->t_iSocketFd);
        p_pSocket->t_iSocketFd = -1;
    }
    
    p_pSocket->t_bConnected = false;
    
    // Mark as available after unlocking
    _unlockSocket(p_pSocket);
    
    p_pSocket->t_bInUse = false;
    
    return NETWORK_OK;
}

// Set socket option
int networkSetOption(NetworkSocket* p_pSocket, int p_iOption, int p_iValue) {
    if (!p_pSocket || !p_pSocket->t_bInUse)
        return NETWORK_INVALID_SOCKET;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set option
    if (setsockopt(p_pSocket->t_iSocketFd, SOL_SOCKET, p_iOption, 
                 &p_iValue, sizeof(p_iValue)) < 0) {
        _unlockSocket(p_pSocket);
        return NETWORK_ERROR;
    }
    
    _unlockSocket(p_pSocket);
    return NETWORK_OK;
}

// Set socket blocking mode
int networkSetBlocking(NetworkSocket* p_pSocket, int p_iBlocking) {
    if (!p_pSocket || !p_pSocket->t_bInUse)
        return NETWORK_INVALID_SOCKET;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set blocking mode
    bool l_bNonBlocking = (p_iBlocking == NETWORK_SOCK_NONBLOCKING);
    l_iResult = _setNonBlocking(p_pSocket->t_iSocketFd, l_bNonBlocking);
    
    if (l_iResult == NETWORK_OK) {
        p_pSocket->t_bNonBlocking = l_bNonBlocking;
    }
    
    _unlockSocket(p_pSocket);
    return l_iResult;
}

// Set socket timeout
int networkSetTimeout(NetworkSocket* p_pSocket, int p_iTimeoutMs, bool p_bSendTimeout) {
    if (!p_pSocket || !p_pSocket->t_bInUse)
        return NETWORK_INVALID_SOCKET;
    
    // Lock socket
    int l_iResult = _lockSocket(p_pSocket);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set timeout
    struct timeval l_tTimeout;
    l_tTimeout.tv_sec = p_iTimeoutMs / 1000;
    l_tTimeout.tv_usec = (p_iTimeoutMs % 1000) * 1000;
    
    int l_iOption = p_bSendTimeout ? SO_SNDTIMEO : SO_RCVTIMEO;
    
    if (setsockopt(p_pSocket->t_iSocketFd, SOL_SOCKET, l_iOption, 
                 &l_tTimeout, sizeof(l_tTimeout)) < 0) {
        _unlockSocket(p_pSocket);
        return NETWORK_ERROR;
    }
    
    _unlockSocket(p_pSocket);
    return NETWORK_OK;
}

// Configure TLS (placeholder for future implementation)
int networkSetTls(NetworkSocket* p_pSocket, const NetworkTlsConfig* p_pTlsConfig) {
    if (!p_pSocket || !p_pSocket->t_bInUse || !p_pTlsConfig)
        return NETWORK_INVALID_PARAM;
    
    // This is just a placeholder - TLS implementation would go here
    // For now, just return an error code indicating the feature isn't available
    return NETWORK_ERROR;
}

// Poll sockets for events
int networkPoll(NetworkSocket** p_ppSockets, int* p_piEvents, 
               int p_iCount, int p_iTimeoutMs) {
    if (!p_ppSockets || !p_piEvents || p_iCount <= 0)
        return NETWORK_INVALID_PARAM;
    
    // Set up fd_sets for select()
    fd_set l_tReadSet, l_tWriteSet, l_tErrorSet;
    FD_ZERO(&l_tReadSet);
    FD_ZERO(&l_tWriteSet);
    FD_ZERO(&l_tErrorSet);
    
    int l_iMaxFd = -1;
    
    // Add sockets to appropriate sets
    for (int i = 0; i < p_iCount; i++) {
        if (!p_ppSockets[i] || !p_ppSockets[i]->t_bInUse || p_ppSockets[i]->t_iSocketFd < 0) {
            p_piEvents[i] = 0;
            continue;
        }
        
        if (p_piEvents[i] & NETWORK_EVENT_READ)
            FD_SET(p_ppSockets[i]->t_iSocketFd, &l_tReadSet);
            
        if (p_piEvents[i] & NETWORK_EVENT_WRITE)
            FD_SET(p_ppSockets[i]->t_iSocketFd, &l_tWriteSet);
            
        if (p_piEvents[i] & NETWORK_EVENT_ERROR)
            FD_SET(p_ppSockets[i]->t_iSocketFd, &l_tErrorSet);
            
        if (p_ppSockets[i]->t_iSocketFd > l_iMaxFd)
            l_iMaxFd = p_ppSockets[i]->t_iSocketFd;
    }
    
    if (l_iMaxFd < 0)
        return 0; // No valid sockets
    
    // Set up timeout
    struct timeval l_tTimeout, *l_pTimeout = NULL;
    if (p_iTimeoutMs >= 0) {
        l_tTimeout.tv_sec = p_iTimeoutMs / 1000;
        l_tTimeout.tv_usec = (p_iTimeoutMs % 1000) * 1000;
        l_pTimeout = &l_tTimeout;
    }
    
    // Wait for events
    int l_iResult = select(l_iMaxFd + 1, &l_tReadSet, &l_tWriteSet, &l_tErrorSet, l_pTimeout);
    
    if (l_iResult < 0)
        return NETWORK_ERROR;
        
    if (l_iResult == 0)
        return 0; // Timeout, no events
    
    // Process results
    int l_iReadyCount = 0;
    
    for (int i = 0; i < p_iCount; i++) {
        if (!p_ppSockets[i] || !p_ppSockets[i]->t_bInUse || p_ppSockets[i]->t_iSocketFd < 0) {
            p_piEvents[i] = 0;
            continue;
        }
        
        int l_iEvents = 0;
        
        if (FD_ISSET(p_ppSockets[i]->t_iSocketFd, &l_tReadSet))
            l_iEvents |= NETWORK_EVENT_READ;
            
        if (FD_ISSET(p_ppSockets[i]->t_iSocketFd, &l_tWriteSet))
            l_iEvents |= NETWORK_EVENT_WRITE;
            
        if (FD_ISSET(p_ppSockets[i]->t_iSocketFd, &l_tErrorSet))
            l_iEvents |= NETWORK_EVENT_ERROR;
            
        p_piEvents[i] = l_iEvents;
        
        if (l_iEvents)
            l_iReadyCount++;
    }
    
    return l_iReadyCount;
}

// Get error string
const char* networkGetErrorString(int p_iError) {
    switch (p_iError) {
        case NETWORK_OK:             return "Success";
        case NETWORK_ERROR:          return "General network error";
        case NETWORK_TIMEOUT:        return "Operation timed out";
        case NETWORK_DISCONNECTED:   return "Connection closed";
        case NETWORK_INVALID_SOCKET: return "Invalid socket";
        case NETWORK_INVALID_ADDRESS:return "Invalid address";
        case NETWORK_INVALID_PORT:   return "Invalid port number";
        case NETWORK_INVALID_BUFFER: return "Invalid buffer";
        case NETWORK_INVALID_SIZE:   return "Invalid size";
        case NETWORK_WOULD_BLOCK:    return "Operation would block";
        case NETWORK_MUTEX_ERROR:    return "Thread synchronization error";
        case NETWORK_NO_RESOURCES:   return "No resources available";
        case NETWORK_INVALID_PARAM:  return "Invalid parameter";
        default: {
            static char l_cBuffer[32];
            snprintf(l_cBuffer, sizeof(l_cBuffer), "Unknown error code: %d", p_iError);
            return l_cBuffer;
        }
    }
}

// Check if socket is connected
bool networkIsConnected(NetworkSocket* p_pSocket) {
    if (!p_pSocket || !p_pSocket->t_bInUse)
        return false;
    
    // Lock socket
    if (_lockSocket(p_pSocket) != NETWORK_OK)
        return false;
    
    bool l_bConnected = p_pSocket->t_bConnected;
    
    _unlockSocket(p_pSocket);
    
    return l_bConnected;
}

// Get socket index (for debugging)
int networkGetSocketIndex(NetworkSocket* p_pSocket) {
    if (!p_pSocket)
        return -1;
    
    NetworkContext* l_pContext = networkInit();
    
    for (int i = 0; i < NETWORK_MAX_SOCKETS; i++) {
        if (&l_pContext->t_tSockets[i] == p_pSocket)
            return i;
    }
    
    return -1;
}
