////////////////////////////////////////////////////////////
//  assert scource file
//  
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xAssert.h"
#include "core.h"

///////////////////////////////////////////////////////
/// createSocket
///////////////////////////////////////////////////////
int createSocket(NetworkSocketCreation* p_tSocketParameters)
{
	
	X_ASSERT(p_tSocketParameters != NULL);
	X_ASSERT(p_tSocketParameters->t_iDomain == AF_INET || p_tSocketParameters->t_iDomain == AF_INET6);
	X_ASSERT(p_tSocketParameters->t_iType == SOCK_STREAM || p_tSocketParameters->t_iType == SOCK_DGRAM);
	X_ASSERT(p_tSocketParameters->t_iProtocol == 0);

	int l_iSocket = socket(p_tSocketParameters->t_iDomain, p_tSocketParameters->t_iType, p_tSocketParameters->t_iProtocol);

	X_ASSERT(l_iSocket == NETWORK_OK);

	return l_iSocket;
}

///////////////////////////////////////////////////////
/// bindSocket
///////////////////////////////////////////////////////
int bindSocket(int p_iSocket, NetworkAddress* p_pttAddress, struct sockaddr* p_pttSockaddr)
{
	X_ASSERT(p_iSocket > 0);
	X_ASSERT(p_pttAddress != NULL);
	X_ASSERT(p_pttSockaddr != NULL);

	// convert to local structure
	NetworkAddress l_ttAddress = *p_pttAddress;

	X_ASSERT(l_ttAddress.t_usPort > 0);
	X_ASSERT(l_ttAddress.t_usPort < 65535);

	// bind socket
	int l_iResult = bind(p_iSocket, p_pttSockaddr, sizeof(struct sockaddr));

	return l_iResult;
}

///////////////////////////////////////////////////////
/// listen socket 
///////////////////////////////////////////////////////
int listenSocket(int p_iSocket, int p_iBacklog)
{
	X_ASSERT(p_iSocket > 0);
	X_ASSERT(p_iBacklog > 0);

	int l_iResult = listen(p_iSocket, p_iBacklog);

	return l_iResult;
}

///////////////////////////////////////////////////////
/// accept connection
///////////////////////////////////////////////////////
int acceptConnection(int p_iSocket, NetworkAddress* p_tAddress)
{
	X_ASSERT(p_iSocket > 0);
	X_ASSERT(p_tAddress != NULL);

	// convert to local structure
	NetworkAddress l_ttAddress = *p_tAddress;

	X_ASSERT(l_ttAddress.t_usPort > 0);
	X_ASSERT(l_ttAddress.t_usPort < 65535);

	// accept connection
	int l_iResult = accept(p_iSocket, NULL, NULL);

	return l_iResult;
}

///////////////////////////////////////////////////////
/// connect socket
///////////////////////////////////////////////////////
int connectSocket(int p_iSocket, NetworkAddress* p_tAddress)
{
	X_ASSERT(p_iSocket > 0);
	X_ASSERT(p_tAddress != NULL);

	// convert to local structure
	NetworkAddress l_ttAddress = *p_tAddress;

	X_ASSERT(l_ttAddress.t_usPort > 0);
	X_ASSERT(l_ttAddress.t_usPort < 65535);

	struct sockaddr_in l_ttSockaddr;
	memset(&l_ttSockaddr, 0, sizeof(struct sockaddr_in));
	l_ttSockaddr.sin_family = AF_INET;
	l_ttSockaddr.sin_port = htons(l_ttAddress.t_usPort);

	// convert ip address
	int l_iResult = inet_pton(AF_INET, l_ttAddress.t_cAddress, &l_ttSockaddr.sin_addr);

	X_ASSERT(l_iResult == 1);

	// connect socket
	l_iResult = connect(p_iSocket, (struct sockaddr*)&l_ttSockaddr, sizeof(struct sockaddr_in));

	return l_iResult;	
}

///////////////////////////////////////////////////////
/// send data
///////////////////////////////////////////////////////
int sendData(int p_iSocket, void* p_tpBuffer, unsigned long p_ulSize)
{
	X_ASSERT(p_iSocket > 0);
	X_ASSERT(p_tpBuffer != NULL);
	X_ASSERT(p_ulSize > 0);

	// send data
	int l_iResult = send(p_iSocket, p_tpBuffer, p_ulSize, 0);

	return l_iResult;
}

///////////////////////////////////////////////////////
/// receive data
///////////////////////////////////////////////////////
int receiveData(int p_iSocket, void* p_ptBuffer, int p_iSize)
{
	X_ASSERT(p_iSocket > 0);
	X_ASSERT(p_ptBuffer != NULL);
	X_ASSERT(p_iSize > 0);

	// receive data
	int l_iResult = recv(p_iSocket, p_ptBuffer, p_iSize, 0);

	return l_iResult;
}

///////////////////////////////////////////////////////
/// close socket
///////////////////////////////////////////////////////
int closeSocket(int p_iSocket)
{
	X_ASSERT(p_iSocket > 0);

	// close socket
	int l_iResult = close(p_iSocket);

	return l_iResult;
}

///////////////////////////////////////////////////////
/// set socket option
///////////////////////////////////////////////////////
int setSocketOption(int p_iSocket, int p_iOption, int p_iValue)
{
	X_ASSERT(p_iSocket > 0);
	X_ASSERT(p_iOption > 0);

	// set socket option
	int l_iResult = setsockopt(p_iSocket, SOL_SOCKET, p_iOption, &p_iValue, sizeof(int));

	return l_iResult;
}

#ifndef _WIN32
///////////////////////////////////////////////////////
/// get socket option
///////////////////////////////////////////////////////
int getSocketOption(int p_iSocket, int p_iOption, int* p_iValue)
{
    X_ASSERT(p_iSocket > 0);
    X_ASSERT(p_iOption > 0);
    X_ASSERT(p_iValue != NULL);

	// Déclarer la taille comme int
	int optlen = sizeof(int);


    // Déclarer la taille comme socklen_t
    socklen_t optlen = sizeof(int);

    // Appeler getsockopt avec un pointeur vers optlen
    int l_iResult = getsockopt(p_iSocket, SOL_SOCKET, p_iOption, p_iValue, &optlen);

    return l_iResult;
}

#endif // _WIN32