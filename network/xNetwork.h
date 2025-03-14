////////////////////////////////////////////////////////////
//  network core header file for embedded systems
//  defines the basic network types and constants
//  IPv4-focused, thread-safe with TLS extension points
//  Memory-optimized for embedded environments
//
// general disclosure: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#ifndef NETWORK_CORE_H_
#define NETWORK_CORE_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

// Configuration constants - adjust as needed for target system
#define NETWORK_MAX_SOCKETS 16   // Maximum number of simultaneous sockets
#define NETWORK_BUFFER_SIZE 512  // Default buffer size for operations
#define NETWORK_MAX_PENDING 5    // Default pending connections queue

// network error codes
#define NETWORK_OK 0
#define NETWORK_ERROR -1
#define NETWORK_TIMEOUT -2
#define NETWORK_DISCONNECTED -3
#define NETWORK_INVALID_SOCKET -4
#define NETWORK_INVALID_ADDRESS -5
#define NETWORK_INVALID_PORT -6
#define NETWORK_INVALID_BUFFER -7
#define NETWORK_INVALID_SIZE -8
#define NETWORK_WOULD_BLOCK -9
#define NETWORK_MUTEX_ERROR -10
#define NETWORK_NO_RESOURCES -11
#define NETWORK_INVALID_PARAM -12

// Byte order conversion macros
#define HOST_TO_NET_LONG(p_uiValue) htonl(p_uiValue)
#define HOST_TO_NET_SHORT(p_usValue) htons(p_usValue)
#define NET_TO_HOST_LONG(p_uiValue) ntohl(p_uiValue)
#define NET_TO_HOST_SHORT(p_usValue) ntohs(p_usValue)

// Forward declarations for opaque types
typedef struct NetworkContext NetworkContext;
typedef struct NetworkSocket NetworkSocket;

// network address structure (IPv4 focused)
//////////////////////////////////
/// t_cAddress : address string
/// t_usPort : port number
//////////////////////////////////
typedef struct {
    char t_cAddress[INET_ADDRSTRLEN];
    unsigned short t_usPort;
} NetworkAddress;

// TLS configuration structure (extension point)
//////////////////////////////////
/// t_bEnabled : TLS enabled flag
/// t_pContext : TLS context (implementation specific)
/// t_bVerifyPeer : verify peer certificate
/// t_cCertPath : certificate path
/// t_cKeyPath : private key path
//////////////////////////////////
typedef struct {
    bool t_bEnabled;
    void* t_pContext;
    bool t_bVerifyPeer;
    char t_cCertPath[128];
    char t_cKeyPath[128];
} NetworkTlsConfig;

// available socket types
#define NETWORK_SOCK_TCP SOCK_STREAM
#define NETWORK_SOCK_UDP SOCK_DGRAM


// available socket options
#define NETWORK_SOCK_BLOCKING 0
#define NETWORK_SOCK_NONBLOCKING 1

// Event types for polling
#define NETWORK_EVENT_READ  0x01
#define NETWORK_EVENT_WRITE 0x02
#define NETWORK_EVENT_ERROR 0x04

//////////////////////////////////
/// Core API functions
//////////////////////////////////

//////////////////////////////////
/// @brief Initialize network library
/// @return NetworkContext* Context handle or NULL on error
//////////////////////////////////
NetworkContext* networkInit(void);

//////////////////////////////////
/// @brief Cleanup network resources
/// @param p_pContext Network context
/// @return int Error code
//////////////////////////////////
int networkCleanup(NetworkContext* p_pContext);

//////////////////////////////////
/// @brief Create a socket
/// @param p_pContext Network context
/// @param p_iType Socket type (NETWORK_SOCK_TCP or NETWORK_SOCK_UDP)
/// @param p_iBlocking Blocking mode (NETWORK_SOCK_BLOCKING or NETWORK_SOCK_NONBLOCKING)
/// @return NetworkSocket* Socket handle or NULL on error
//////////////////////////////////
NetworkSocket* networkCreateSocket(NetworkContext* p_pContext, int p_iType, int p_iBlocking);

//////////////////////////////////
/// @brief Create a network address
/// @param p_pcAddress IP address string (NULL or empty for INADDR_ANY)
/// @param p_usPort Port number
/// @return NetworkAddress Address structure
//////////////////////////////////
NetworkAddress networkMakeAddress(const char* p_pcAddress, unsigned short p_usPort);

//////////////////////////////////
/// @brief Bind socket to address
/// @param p_pSocket Socket handle
/// @param p_pAddress Address structure
/// @return int Error code
//////////////////////////////////
int networkBind(NetworkSocket* p_pSocket, const NetworkAddress* p_pAddress);

//////////////////////////////////
/// @brief Listen for incoming connections
/// @param p_pSocket Socket handle
/// @param p_iBacklog Maximum pending connections
/// @return int Error code
//////////////////////////////////
int networkListen(NetworkSocket* p_pSocket, int p_iBacklog);

//////////////////////////////////
/// @brief Accept incoming connection
/// @param p_pSocket Listening socket
/// @param p_pClientAddress Address to store client info (can be NULL)
/// @return NetworkSocket* New socket handle or NULL on error
//////////////////////////////////
NetworkSocket* networkAccept(NetworkSocket* p_pSocket, NetworkAddress* p_pClientAddress);

//////////////////////////////////
/// @brief Connect to remote server
/// @param p_pSocket Socket handle
/// @param p_pAddress Remote address
/// @return int Error code
//////////////////////////////////
int networkConnect(NetworkSocket* p_pSocket, const NetworkAddress* p_pAddress);

//////////////////////////////////
/// @brief Send data
/// @param p_pSocket Socket handle
/// @param p_pBuffer Data buffer
/// @param p_ulSize Data size
/// @return int Bytes sent or error code
//////////////////////////////////
int networkSend(NetworkSocket* p_pSocket, const void* p_pBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief Receive data
/// @param p_pSocket Socket handle
/// @param p_pBuffer Data buffer
/// @param p_ulSize Buffer size
/// @return int Bytes received or error code
//////////////////////////////////
int networkReceive(NetworkSocket* p_pSocket, void* p_pBuffer, unsigned long p_ulSize);

//////////////////////////////////
/// @brief Send data to specific address (UDP)
/// @param p_pSocket Socket handle
/// @param p_pBuffer Data buffer
/// @param p_ulSize Data size
/// @param p_pAddress Target address
/// @return int Bytes sent or error code
//////////////////////////////////
int networkSendTo(NetworkSocket* p_pSocket, const void* p_pBuffer, 
                  unsigned long p_ulSize, const NetworkAddress* p_pAddress);

//////////////////////////////////
/// @brief Receive data with sender info (UDP)
/// @param p_pSocket Socket handle
/// @param p_pBuffer Data buffer
/// @param p_ulSize Buffer size
/// @param p_pAddress Sender address
/// @return int Bytes received or error code
//////////////////////////////////
int networkReceiveFrom(NetworkSocket* p_pSocket, void* p_pBuffer, 
                       unsigned long p_ulSize, NetworkAddress* p_pAddress);

//////////////////////////////////
/// @brief Close socket
/// @param p_pSocket Socket handle
/// @return int Error code
//////////////////////////////////
int networkCloseSocket(NetworkSocket* p_pSocket);

//////////////////////////////////
/// @brief Set socket option
/// @param p_pSocket Socket handle
/// @param p_iOption Option type
/// @param p_iValue Option value
/// @return int Error code
//////////////////////////////////
int networkSetOption(NetworkSocket* p_pSocket, int p_iOption, int p_iValue);

//////////////////////////////////
/// @brief Set socket blocking mode
/// @param p_pSocket Socket handle
/// @param p_iBlocking NETWORK_SOCK_BLOCKING or NETWORK_SOCK_NONBLOCKING
/// @return int Error code
//////////////////////////////////
int networkSetBlocking(NetworkSocket* p_pSocket, int p_iBlocking);

//////////////////////////////////
/// @brief Set socket timeout
/// @param p_pSocket Socket handle
/// @param p_iTimeoutMs Timeout in milliseconds
/// @param p_bSendTimeout True for send timeout, false for receive timeout
/// @return int Error code
//////////////////////////////////
int networkSetTimeout(NetworkSocket* p_pSocket, int p_iTimeoutMs, bool p_bSendTimeout);

//////////////////////////////////
/// @brief Configure TLS for socket (extension point)
/// @param p_pSocket Socket handle
/// @param p_pTlsConfig TLS configuration
/// @return int Error code
//////////////////////////////////
int networkSetTls(NetworkSocket* p_pSocket, const NetworkTlsConfig* p_pTlsConfig);

//////////////////////////////////
/// @brief Poll socket for events
/// @param p_ppSockets Array of socket handles
/// @param p_piEvents Array of event flags (in/out)
/// @param p_iCount Number of sockets
/// @param p_iTimeoutMs Timeout in milliseconds (-1 for infinite)
/// @return int Number of ready sockets or error code
//////////////////////////////////
int networkPoll(NetworkSocket** p_ppSockets, int* p_piEvents, 
                int p_iCount, int p_iTimeoutMs);

//////////////////////////////////
/// @brief Get last error description
/// @param p_iError Error code
/// @return const char* Error description
//////////////////////////////////
const char* networkGetErrorString(int p_iError);

//////////////////////////////////
/// @brief Check if socket is connected
/// @param p_pSocket Socket handle
/// @return bool Connected status
//////////////////////////////////
bool networkIsConnected(NetworkSocket* p_pSocket);

//////////////////////////////////
/// @brief Get socket index from handle (for embedded debugging)
/// @param p_pSocket Socket handle
/// @return int Socket index or -1 if invalid
//////////////////////////////////
int networkGetSocketIndex(NetworkSocket* p_pSocket);

#endif // NETWORK_CORE_H_
