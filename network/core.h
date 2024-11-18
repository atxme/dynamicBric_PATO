////////////////////////////////////////////////////////////
//  network core header file
//  defines the basic network types and constants
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////
#pragma once

#ifndef NETWORK_CORE_H_
#define NETWORK_CORE_H_

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NETWORK_SUCCESS 0  // Correction de la typo "SUCCES"
#define NETWORK_ERROR -1


#define HOST_TO_NET_LONG(p_uiValue) htonl(p_uiValue)
#define HOST_TO_NET_SHORT(p_usValue) htons(p_usValue)

#define NET_TO_HOST_LONG(p_uiValue) ntohl(p_uiValue)
#define NET_TO_HOST_SHORT(p_usValue) ntohs(p_usValue)


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


// network address
//////////////////////////////////
/// t_cAddress : address string
/// t_usPort : port number
//////////////////////////////////
typedef struct
{
	char t_cAddress[16];  // 16 bytes to store both IPv4 and IPv6 addresses
	unsigned short t_usPort;
} NetworkAddress;

// network socket
//////////////////////////////////
/// t_iSocket : socket handle
/// t_iType : socket type
//////////////////////////////////
typedef struct
{
	int t_iSocket;
	int t_iType;
} NetworkSocket;


// available domain types
#define NETWORK_DOMAIN_IPV4 AF_INET
#define NETWORK_DOMAIN_IPV6 AF_INET6

// available socket types
#define NETWORK_SOCK_TCP SOCK_STREAM
#define NETWORK_SOCK_UDP SOCK_DGRAM

// available socket options
#define NETWORK_SOCK_BLOCKING 0
#define NETWORK_SOCK_NONBLOCKING 1

// available protocol type 
#define NETWORK_PROTOCOL_TCP 0
#define NETWORK_PROTOCOL_UDP 1

// socket creation structure 
//////////////////////////////////
/// t_iDomain : domain type (NETWORK_DOMAIN_IPV4 or NETWORK_DOMAIN_IPV6)
/// t_iType : socket type (NETWORK_SOCK_TCP or NETWORK_SOCK_UDP)
/// t_iProtocol : protocol type (0 for default)
//////////////////////////////////
typedef struct
{
	int t_iDomain;
	int t_iType;
	int t_iProtocol;
} NetworkSocketCreation;

// core functions 

//////////////////////////////////
/// @brief create socket stream 
/// @param p_iSocketParameters : structure containing socket parameters
/// @return : socket handle or error code
//////////////////////////////////
int createSocket(NetworkSocketCreation* p_tSocketParameters);



//////////////////////////////////
/// @brief bind socket to address
/// @param p_iSocket : socket handle
/// @param p_tAddress : address structure
/// @param p_pttSockaddr : socket address sructure
/// @return : error code
//////////////////////////////////
int bindSocket(int p_iSocket, NetworkAddress* p_pttAddress, struct sockaddr* p_pttSockaddr);



//////////////////////////////////
/// @brief listen to incoming connections
/// @param p_iSocket : socket handle
/// @param p_iBacklog : maximum number of pending connections
/// @return : error code
//////////////////////////////////
int listenSocket(int p_iSocket, int p_iBacklog);

//////////////////////////////////
/// @brief accept incoming connection
/// @param p_iSocket : socket handle
/// @param p_tAddress : address structure
/// @return : socket handle or error code
//////////////////////////////////
int acceptConnection(int p_iSocket, NetworkAddress* p_tAddress);

//////////////////////////////////
/// @brief connect to remote address
/// @param p_iSocket : socket handle
/// @param p_tAddress : address structure
/// @return : success or error code
//////////////////////////////////
int connectSocket(int p_iSocket, NetworkAddress* p_tAddress);

//////////////////////////////////
/// @brief send data to remote address
/// @param p_iSocket : socket handle
/// @param p_ptBuffer : data buffer
/// @param p_iSize : data size
/// @return : success or error code
//////////////////////////////////
int sendData(int p_iSocket, void* p_tpBuffer, int p_iSize);

//////////////////////////////////
/// @brief receive data from remote address
/// @param p_iSocket : socket handle
/// @param p_pBuffer : data buffer
/// @param p_iSize : data size
/// @return : error code
//////////////////////////////////
int receiveData(int p_iSocket, void* p_ptBuffer, int p_iSize);

//////////////////////////////////
/// @brief close socket
/// @param p_iSocket : socket handle
/// @return : error code
//////////////////////////////////
int closeSocket(int p_iSocket);


//////////////////////////////////
/// @brief set socket option
/// @param p_iSocket : socket handle
/// @param p_iOption : option type
/// @param p_iValue : option value
/// @return : error code
///////////////////////////////////
int setSocketOption(int p_iSocket, int p_iOption, int p_iValue);

//////////////////////////////////
/// @brief get socket option
/// @param p_iSocket : socket handle
/// @param p_iOption : option type
/// @param p_iValue : option value
/// @return : error code
////////////////////////////////// 
int getSocketOption(int p_iSocket, int p_iOption, int* p_iValue);



#endif // NETWORK_CORE_H_
