////////////////////////////////////////////////////////////
//  Network Keep-Alive header file for embedded systems
//  Provides keep-alive functionality for network connections
//  Compatible with xNetwork API, memory-optimized
//
// general disclosure: copy or share the file is forbidden
// Written : 14/11/2024
// Intellectual property of Christophe Benedetti
////////////////////////////////////////////////////////////

#ifndef NETWORK_KEEPALIVE_H_
#define NETWORK_KEEPALIVE_H_

#include "xNetwork.h"

// Configuration constants
#define KEEPALIVE_DEFAULT_INTERVAL 30    // Default interval in seconds
#define KEEPALIVE_DEFAULT_TIMEOUT  5     // Default timeout waiting for response
#define KEEPALIVE_DEFAULT_RETRIES  3     // Default retries before failure
#define KEEPALIVE_MAX_DATA_SIZE    32    // Maximum size of keep-alive data

// Keep-alive states
#define KEEPALIVE_STATE_DISABLED   0     // Keep-alive is disabled
#define KEEPALIVE_STATE_IDLE       1     // Keep-alive is idle, waiting for next interval
#define KEEPALIVE_STATE_ACTIVE     2     // Keep-alive probe sent, waiting for response
#define KEEPALIVE_STATE_FAILED     3     // Keep-alive failed after max retries

// Forward declarations
typedef struct NetworkKeepAlive NetworkKeepAlive;

// Keep-alive callback function type
typedef void (*KeepAliveCallback)(NetworkKeepAlive* p_pKeepAlive, int p_iEvent, void* p_pUserData);

// Keep-alive events for callbacks
#define KEEPALIVE_EVENT_SENT       1     // Keep-alive probe sent
#define KEEPALIVE_EVENT_RECEIVED   2     // Keep-alive response received
#define KEEPALIVE_EVENT_TIMEOUT    3     // Keep-alive timed out, retry
#define KEEPALIVE_EVENT_FAILED     4     // Keep-alive failed after max retries
#define KEEPALIVE_EVENT_RECOVERED  5     // Keep-alive recovered after failure

//////////////////////////////////
/// @brief Initialize keep-alive for a socket
/// @param p_pSocket Socket to attach keep-alive to
/// @param p_iInterval Interval in seconds (0 for default)
/// @param p_iTimeout Timeout in seconds waiting for response (0 for default)
/// @param p_iMaxRetries Max retries before failure (0 for default)
/// @return NetworkKeepAlive* Keep-alive handle or NULL on error
//////////////////////////////////
NetworkKeepAlive* networkKeepAliveInit(NetworkSocket* p_pSocket, 
                                    int p_iInterval, 
                                    int p_iTimeout, 
                                    int p_iMaxRetries);

//////////////////////////////////
/// @brief Start keep-alive mechanism
/// @param p_pKeepAlive Keep-alive handle
/// @param p_pData Custom data to send (NULL to use default)
/// @param p_ulDataSize Size of custom data (0 for default)
/// @return int Error code
//////////////////////////////////
int networkKeepAliveStart(NetworkKeepAlive* p_pKeepAlive, 
                        const void* p_pData, 
                        unsigned long p_ulDataSize);

//////////////////////////////////
/// @brief Stop keep-alive mechanism
/// @param p_pKeepAlive Keep-alive handle
/// @return int Error code
//////////////////////////////////
int networkKeepAliveStop(NetworkKeepAlive* p_pKeepAlive);

//////////////////////////////////
/// @brief Cleanup keep-alive resources
/// @param p_pKeepAlive Keep-alive handle
/// @return int Error code
//////////////////////////////////
int networkKeepAliveCleanup(NetworkKeepAlive* p_pKeepAlive);

//////////////////////////////////
/// @brief Set keep-alive callback
/// @param p_pKeepAlive Keep-alive handle
/// @param p_pfCallback Callback function
/// @param p_pUserData User data to pass to callback
/// @return int Error code
//////////////////////////////////
int networkKeepAliveSetCallback(NetworkKeepAlive* p_pKeepAlive, 
                             KeepAliveCallback p_pfCallback, 
                             void* p_pUserData);

//////////////////////////////////
/// @brief Check keep-alive status
/// @param p_pKeepAlive Keep-alive handle
/// @return int Keep-alive state code
//////////////////////////////////
int networkKeepAliveGetState(NetworkKeepAlive* p_pKeepAlive);

//////////////////////////////////
/// @brief Process a keep-alive response
/// @param p_pKeepAlive Keep-alive handle
/// @param p_pData Response data
/// @param p_ulDataSize Response data size
/// @return int Error code
//////////////////////////////////
int networkKeepAliveProcessResponse(NetworkKeepAlive* p_pKeepAlive, 
                                 const void* p_pData, 
                                 unsigned long p_ulDataSize);

//////////////////////////////////
/// @brief Get last keep-alive timestamp
/// @param p_pKeepAlive Keep-alive handle
/// @return long Last timestamp (seconds since epoch) or 0 on error
//////////////////////////////////
long networkKeepAliveGetLastTimestamp(NetworkKeepAlive* p_pKeepAlive);

//////////////////////////////////
/// @brief Update keep-alive interval
/// @param p_pKeepAlive Keep-alive handle
/// @param p_iInterval New interval in seconds
/// @return int Error code
//////////////////////////////////
int networkKeepAliveSetInterval(NetworkKeepAlive* p_pKeepAlive, int p_iInterval);

//////////////////////////////////
/// @brief Force immediate keep-alive probe
/// @param p_pKeepAlive Keep-alive handle
/// @return int Error code
//////////////////////////////////
int networkKeepAliveTrigger(NetworkKeepAlive* p_pKeepAlive);

#endif // NETWORK_KEEPALIVE_H_ 