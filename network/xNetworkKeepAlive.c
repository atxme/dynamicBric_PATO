////////////////////////////////////////////////////////////
//  Network Keep-Alive implementation file for embedded systems
//  Provides keep-alive functionality for network connections
//  Compatible with xNetwork API, memory-optimized
//
// general disclosure: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xNetworkKeepAlive.h"
#include <time.h>

//////////////////////////////////
/// Internal structures
//////////////////////////////////

// Default keep-alive message
static const char s_cDefaultKeepAliveMsg[] = "KEEPALIVE";

// Keep-alive structure
struct NetworkKeepAlive {
    NetworkSocket* t_pSocket;            // Associated socket
    int t_iState;                        // Current state
    int t_iInterval;                     // Interval in seconds
    int t_iTimeout;                      // Timeout in seconds
    int t_iMaxRetries;                   // Maximum retries
    int t_iCurrentRetries;               // Current retry count
    long t_lLastSent;                    // Last sent timestamp
    long t_lLastReceived;                // Last received timestamp
    pthread_mutex_t t_tMutex;            // Internal mutex
    pthread_t t_tThread;                 // Keep-alive thread
    bool t_bRunning;                     // Thread running flag
    bool t_bThreadActive;                // Thread active flag
    KeepAliveCallback t_pfCallback;      // User callback
    void* t_pUserData;                   // User callback data
    unsigned char t_cData[KEEPALIVE_MAX_DATA_SIZE]; // Keep-alive data
    unsigned long t_ulDataSize;          // Keep-alive data size
};

//////////////////////////////////
/// Static (internal) helper functions
//////////////////////////////////

// Get current timestamp in seconds
static long _getCurrentTimestamp(void) {
    return (long)time(NULL);
}

// Safe keep-alive locking with error checking
static int _lockKeepAlive(NetworkKeepAlive* p_pKeepAlive) {
    if (!p_pKeepAlive)
        return NETWORK_INVALID_PARAM;
    
    if (pthread_mutex_lock(&p_pKeepAlive->t_tMutex) != 0)
        return NETWORK_MUTEX_ERROR;
    
    return NETWORK_OK;
}

// Safe keep-alive unlocking with error checking
static int _unlockKeepAlive(NetworkKeepAlive* p_pKeepAlive) {
    if (!p_pKeepAlive)
        return NETWORK_INVALID_PARAM;
    
    if (pthread_mutex_unlock(&p_pKeepAlive->t_tMutex) != 0)
        return NETWORK_MUTEX_ERROR;
    
    return NETWORK_OK;
}

// Send keep-alive probe
static int _sendKeepAliveProbe(NetworkKeepAlive* p_pKeepAlive) {
    if (!p_pKeepAlive || !p_pKeepAlive->t_pSocket)
        return NETWORK_INVALID_PARAM;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Send the keep-alive data
    const void* l_pData = p_pKeepAlive->t_ulDataSize > 0 ? 
                          p_pKeepAlive->t_cData : 
                          s_cDefaultKeepAliveMsg;
                          
    unsigned long l_ulDataSize = p_pKeepAlive->t_ulDataSize > 0 ? 
                               p_pKeepAlive->t_ulDataSize : 
                               sizeof(s_cDefaultKeepAliveMsg) - 1;
    
    l_iResult = networkSend(p_pKeepAlive->t_pSocket, l_pData, l_ulDataSize);
    
    if (l_iResult >= 0) {
        // Success
        p_pKeepAlive->t_lLastSent = _getCurrentTimestamp();
        p_pKeepAlive->t_iState = KEEPALIVE_STATE_ACTIVE;
        
        // Call callback if registered
        if (p_pKeepAlive->t_pfCallback) {
            KeepAliveCallback l_pfCallback = p_pKeepAlive->t_pfCallback;
            void* l_pUserData = p_pKeepAlive->t_pUserData;
            
            _unlockKeepAlive(p_pKeepAlive);
            l_pfCallback(p_pKeepAlive, KEEPALIVE_EVENT_SENT, l_pUserData);
            return NETWORK_OK;
        }
    } else {
        // Increment retry counter on failure
        p_pKeepAlive->t_iCurrentRetries++;
        
        // Check if we've exceeded max retries
        if (p_pKeepAlive->t_iCurrentRetries >= p_pKeepAlive->t_iMaxRetries) {
            p_pKeepAlive->t_iState = KEEPALIVE_STATE_FAILED;
            
            // Call callback if registered
            if (p_pKeepAlive->t_pfCallback) {
                KeepAliveCallback l_pfCallback = p_pKeepAlive->t_pfCallback;
                void* l_pUserData = p_pKeepAlive->t_pUserData;
                
                _unlockKeepAlive(p_pKeepAlive);
                l_pfCallback(p_pKeepAlive, KEEPALIVE_EVENT_FAILED, l_pUserData);
                return NETWORK_DISCONNECTED;
            }
        } else {
            // Call timeout callback if registered
            if (p_pKeepAlive->t_pfCallback) {
                KeepAliveCallback l_pfCallback = p_pKeepAlive->t_pfCallback;
                void* l_pUserData = p_pKeepAlive->t_pUserData;
                
                _unlockKeepAlive(p_pKeepAlive);
                l_pfCallback(p_pKeepAlive, KEEPALIVE_EVENT_TIMEOUT, l_pUserData);
                return NETWORK_TIMEOUT;
            }
        }
    }
    
    _unlockKeepAlive(p_pKeepAlive);
    return l_iResult;
}

// Keep-alive thread function
static void* _keepAliveThreadFunc(void* p_pArg) {
    NetworkKeepAlive* l_pKeepAlive = (NetworkKeepAlive*)p_pArg;
    
    if (!l_pKeepAlive) {
        return NULL;
    }
    
    // Mark thread as active
    if (_lockKeepAlive(l_pKeepAlive) == NETWORK_OK) {
        l_pKeepAlive->t_bThreadActive = true;
        _unlockKeepAlive(l_pKeepAlive);
    } else {
        return NULL;
    }
    
    // Keep-alive loop
    while (1) {
        // Check if thread should continue running
        bool l_bRunning = false;
        if (_lockKeepAlive(l_pKeepAlive) == NETWORK_OK) {
            l_bRunning = l_pKeepAlive->t_bRunning;
            _unlockKeepAlive(l_pKeepAlive);
        }
        
        if (!l_bRunning) {
            break;
        }
        
        // Get current time
        long l_lCurrentTime = _getCurrentTimestamp();
        bool l_bSendProbe = false;
        bool l_bCheckTimeout = false;
        int l_iState = KEEPALIVE_STATE_IDLE;
        long l_lLastSent = 0;
        
        // Check if it's time to send or check timeout
        if (_lockKeepAlive(l_pKeepAlive) == NETWORK_OK) {
            l_iState = l_pKeepAlive->t_iState;
            l_lLastSent = l_pKeepAlive->t_lLastSent;
            
            if (l_iState == KEEPALIVE_STATE_IDLE) {
                // Check if it's time to send a probe
                if (l_lCurrentTime - l_pKeepAlive->t_lLastSent >= l_pKeepAlive->t_iInterval) {
                    l_bSendProbe = true;
                }
            } else if (l_iState == KEEPALIVE_STATE_ACTIVE) {
                // Check for timeout
                if (l_lCurrentTime - l_pKeepAlive->t_lLastSent >= l_pKeepAlive->t_iTimeout) {
                    l_bCheckTimeout = true;
                }
            }
            
            _unlockKeepAlive(l_pKeepAlive);
        }
        
        // Act based on current state
        if (l_bSendProbe) {
            _sendKeepAliveProbe(l_pKeepAlive);
        } else if (l_bCheckTimeout) {
            // Timeout - send a new probe (retry)
            _sendKeepAliveProbe(l_pKeepAlive);
        }
        
        // Sleep for a shorter interval to be responsive
        // Sleep for 1 second to avoid busy waiting
        sleep(1);
    }
    
    // Mark thread as inactive
    if (_lockKeepAlive(l_pKeepAlive) == NETWORK_OK) {
        l_pKeepAlive->t_bThreadActive = false;
        _unlockKeepAlive(l_pKeepAlive);
    }
    
    return NULL;
}

//////////////////////////////////
/// Core API Implementation
//////////////////////////////////

// Initialize keep-alive for a socket
NetworkKeepAlive* networkKeepAliveInit(NetworkSocket* p_pSocket, 
                                    int p_iInterval, 
                                    int p_iTimeout, 
                                    int p_iMaxRetries) {
    if (!p_pSocket)
        return NULL;
    
    // Allocate keep-alive structure
    NetworkKeepAlive* l_pKeepAlive = (NetworkKeepAlive*)malloc(sizeof(NetworkKeepAlive));
    if (!l_pKeepAlive)
        return NULL;
    
    // Initialize structure
    memset(l_pKeepAlive, 0, sizeof(NetworkKeepAlive));
    
    l_pKeepAlive->t_pSocket = p_pSocket;
    l_pKeepAlive->t_iState = KEEPALIVE_STATE_DISABLED;
    
    // Set parameters (use defaults if 0)
    l_pKeepAlive->t_iInterval = (p_iInterval > 0) ? p_iInterval : KEEPALIVE_DEFAULT_INTERVAL;
    l_pKeepAlive->t_iTimeout = (p_iTimeout > 0) ? p_iTimeout : KEEPALIVE_DEFAULT_TIMEOUT;
    l_pKeepAlive->t_iMaxRetries = (p_iMaxRetries > 0) ? p_iMaxRetries : KEEPALIVE_DEFAULT_RETRIES;
    
    // Initialize timestamps
    l_pKeepAlive->t_lLastSent = _getCurrentTimestamp();
    l_pKeepAlive->t_lLastReceived = l_pKeepAlive->t_lLastSent;
    
    // Initialize mutex
    if (pthread_mutex_init(&l_pKeepAlive->t_tMutex, NULL) != 0) {
        free(l_pKeepAlive);
        return NULL;
    }
    
    return l_pKeepAlive;
}

// Start keep-alive mechanism
int networkKeepAliveStart(NetworkKeepAlive* p_pKeepAlive, 
                        const void* p_pData, 
                        unsigned long p_ulDataSize) {
    if (!p_pKeepAlive)
        return NETWORK_INVALID_PARAM;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Check if already running
    if (p_pKeepAlive->t_bRunning) {
        _unlockKeepAlive(p_pKeepAlive);
        return NETWORK_OK; // Already running
    }
    
    // Set custom data if provided
    if (p_pData && p_ulDataSize > 0) {
        // Limit data size
        if (p_ulDataSize > KEEPALIVE_MAX_DATA_SIZE)
            p_ulDataSize = KEEPALIVE_MAX_DATA_SIZE;
            
        memcpy(p_pKeepAlive->t_cData, p_pData, p_ulDataSize);
        p_pKeepAlive->t_ulDataSize = p_ulDataSize;
    } else {
        // Use default data
        p_pKeepAlive->t_ulDataSize = 0;
    }
    
    // Reset counters
    p_pKeepAlive->t_iCurrentRetries = 0;
    p_pKeepAlive->t_iState = KEEPALIVE_STATE_IDLE;
    p_pKeepAlive->t_lLastSent = _getCurrentTimestamp();
    p_pKeepAlive->t_lLastReceived = p_pKeepAlive->t_lLastSent;
    
    // Set running flag
    p_pKeepAlive->t_bRunning = true;
    
    // Create thread
    if (pthread_create(&p_pKeepAlive->t_tThread, NULL, _keepAliveThreadFunc, p_pKeepAlive) != 0) {
        p_pKeepAlive->t_bRunning = false;
        _unlockKeepAlive(p_pKeepAlive);
        return NETWORK_ERROR;
    }
    
    _unlockKeepAlive(p_pKeepAlive);
    return NETWORK_OK;
}

// Stop keep-alive mechanism
int networkKeepAliveStop(NetworkKeepAlive* p_pKeepAlive) {
    if (!p_pKeepAlive)
        return NETWORK_INVALID_PARAM;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Check if not running
    if (!p_pKeepAlive->t_bRunning) {
        _unlockKeepAlive(p_pKeepAlive);
        return NETWORK_OK; // Already stopped
    }
    
    // Stop thread
    p_pKeepAlive->t_bRunning = false;
    bool l_bThreadActive = p_pKeepAlive->t_bThreadActive;
    pthread_t l_tThread = p_pKeepAlive->t_tThread;
    
    _unlockKeepAlive(p_pKeepAlive);
    
    // Wait for thread to exit if it's active
    if (l_bThreadActive) {
        pthread_join(l_tThread, NULL);
    }
    
    // Update state
    _lockKeepAlive(p_pKeepAlive);
    p_pKeepAlive->t_iState = KEEPALIVE_STATE_DISABLED;
    _unlockKeepAlive(p_pKeepAlive);
    
    return NETWORK_OK;
}

// Cleanup keep-alive resources
int networkKeepAliveCleanup(NetworkKeepAlive* p_pKeepAlive) {
    if (!p_pKeepAlive)
        return NETWORK_INVALID_PARAM;
    
    // Stop if running
    networkKeepAliveStop(p_pKeepAlive);
    
    // Destroy mutex
    pthread_mutex_destroy(&p_pKeepAlive->t_tMutex);
    
    // Free structure
    free(p_pKeepAlive);
    
    return NETWORK_OK;
}

// Set keep-alive callback
int networkKeepAliveSetCallback(NetworkKeepAlive* p_pKeepAlive, 
                             KeepAliveCallback p_pfCallback, 
                             void* p_pUserData) {
    if (!p_pKeepAlive)
        return NETWORK_INVALID_PARAM;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Set callback
    p_pKeepAlive->t_pfCallback = p_pfCallback;
    p_pKeepAlive->t_pUserData = p_pUserData;
    
    _unlockKeepAlive(p_pKeepAlive);
    return NETWORK_OK;
}

// Check keep-alive status
int networkKeepAliveGetState(NetworkKeepAlive* p_pKeepAlive) {
    if (!p_pKeepAlive)
        return KEEPALIVE_STATE_DISABLED;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return KEEPALIVE_STATE_DISABLED;
    
    int l_iState = p_pKeepAlive->t_iState;
    
    _unlockKeepAlive(p_pKeepAlive);
    return l_iState;
}

// Process a keep-alive response
int networkKeepAliveProcessResponse(NetworkKeepAlive* p_pKeepAlive, 
                                 const void* p_pData, 
                                 unsigned long p_ulDataSize) {
    if (!p_pKeepAlive || !p_pData)
        return NETWORK_INVALID_PARAM;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Update timestamps and state
    p_pKeepAlive->t_lLastReceived = _getCurrentTimestamp();
    
    // Reset retry counter if we were in ACTIVE state
    if (p_pKeepAlive->t_iState == KEEPALIVE_STATE_ACTIVE) 
    {
        p_pKeepAlive->t_iCurrentRetries = 0;
        p_pKeepAlive->t_iState = KEEPALIVE_STATE_IDLE;
    } 
    else if (p_pKeepAlive->t_iState == KEEPALIVE_STATE_FAILED) 
    {
        // Recovery
        p_pKeepAlive->t_iCurrentRetries = 0;
        p_pKeepAlive->t_iState = KEEPALIVE_STATE_IDLE;
        
        // Call recovery callback if registered
        if (p_pKeepAlive->t_pfCallback) 
        {
            KeepAliveCallback l_pfCallback = p_pKeepAlive->t_pfCallback;
            void* l_pUserData = p_pKeepAlive->t_pUserData;
            
            _unlockKeepAlive(p_pKeepAlive);
            l_pfCallback(p_pKeepAlive, KEEPALIVE_EVENT_RECOVERED, l_pUserData);
            return NETWORK_OK;
        }
    }
    
    // Call callback if registered
    if (p_pKeepAlive->t_pfCallback) 
    {
        KeepAliveCallback l_pfCallback = p_pKeepAlive->t_pfCallback;
        void* l_pUserData = p_pKeepAlive->t_pUserData;
        
        _unlockKeepAlive(p_pKeepAlive);
        l_pfCallback(p_pKeepAlive, KEEPALIVE_EVENT_RECEIVED, l_pUserData);
        return NETWORK_OK;
    }
    
    _unlockKeepAlive(p_pKeepAlive);
    return NETWORK_OK;
}

// Get last keep-alive timestamp
long networkKeepAliveGetLastTimestamp(NetworkKeepAlive* p_pKeepAlive) 
{
    if (!p_pKeepAlive)
        return 0;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return 0;
    
    long l_lTimestamp = p_pKeepAlive->t_lLastReceived;
    
    _unlockKeepAlive(p_pKeepAlive);
    return l_lTimestamp;
}

// Update keep-alive interval
int networkKeepAliveSetInterval(NetworkKeepAlive* p_pKeepAlive, int p_iInterval) 
{
    if (!p_pKeepAlive || p_iInterval <= 0)
        return NETWORK_INVALID_PARAM;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Update interval
    p_pKeepAlive->t_iInterval = p_iInterval;
    
    _unlockKeepAlive(p_pKeepAlive);
    return NETWORK_OK;
}

// Force immediate keep-alive probe
int networkKeepAliveTrigger(NetworkKeepAlive* p_pKeepAlive) 
{
    if (!p_pKeepAlive)
        return NETWORK_INVALID_PARAM;
    
    int l_iResult = _lockKeepAlive(p_pKeepAlive);
    if (l_iResult != NETWORK_OK)
        return l_iResult;
    
    // Check if running
    if (!p_pKeepAlive->t_bRunning) {
        _unlockKeepAlive(p_pKeepAlive);
        return NETWORK_ERROR; // Not running
    }
    
    // Update state to force a probe
    p_pKeepAlive->t_lLastSent = _getCurrentTimestamp() - p_pKeepAlive->t_iInterval - 1;
    p_pKeepAlive->t_iState = KEEPALIVE_STATE_IDLE;
    
    _unlockKeepAlive(p_pKeepAlive);
    
    // The thread will pick up the change and send a probe
    return NETWORK_OK;
} 