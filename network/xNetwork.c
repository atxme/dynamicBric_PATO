////////////////////////////////////////////////////////////
//  network core source file
//  implements the basic network functions
//
// general discloser: copy or share the file is forbidden
// Written : 14/11/2024
////////////////////////////////////////////////////////////

#include "xNetwork.h"
#include "xAssert.h"

///////////////////////////////////////////////////////
/// createSocket
///////////////////////////////////////////////////////
int createSocket(NetworkSocketCreation* p_tSocketParameters)
{
    if (!p_tSocketParameters) {
        return NETWORK_INVALID_SOCKET;
    }

    if (p_tSocketParameters->t_iDomain != NETWORK_DOMAIN_IPV4 &&
        p_tSocketParameters->t_iDomain != NETWORK_DOMAIN_IPV6) {
        return NETWORK_INVALID_ADDRESS;
    }

    if (p_tSocketParameters->t_iType != NETWORK_SOCK_TCP &&
        p_tSocketParameters->t_iType != NETWORK_SOCK_UDP) {
        return NETWORK_INVALID_SOCKET;
    }

    int l_iSocket = socket(p_tSocketParameters->t_iDomain,
        p_tSocketParameters->t_iType,
        p_tSocketParameters->t_iProtocol);

    return (l_iSocket < 0) ? NETWORK_ERROR : l_iSocket;
}

///////////////////////////////////////////////////////
/// bindSocket
///////////////////////////////////////////////////////
int bindSocket(int p_iSocket, NetworkAddress* p_pttAddress, struct sockaddr* p_pttSockaddr)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    if (!p_pttAddress || !p_pttSockaddr) {
        return NETWORK_INVALID_ADDRESS;
    }

    if (p_pttAddress->t_usPort <= 0 || p_pttAddress->t_usPort >= 65535) {
        return NETWORK_INVALID_PORT;
    }

    int l_iResult = bind(p_iSocket, p_pttSockaddr, sizeof(struct sockaddr));
    return (l_iResult < 0) ? NETWORK_ERROR : NETWORK_OK;
}

///////////////////////////////////////////////////////
/// listenSocket
///////////////////////////////////////////////////////
int listenSocket(int p_iSocket, int p_iBacklog)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    if (p_iBacklog <= 0) {
        return NETWORK_ERROR;
    }

    int l_iResult = listen(p_iSocket, p_iBacklog);
    return (l_iResult < 0) ? NETWORK_ERROR : NETWORK_OK;
}

///////////////////////////////////////////////////////
/// acceptConnection
///////////////////////////////////////////////////////
int acceptConnection(int p_iSocket, NetworkAddress* p_ptAddress)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    if (!p_ptAddress) {
        return NETWORK_INVALID_ADDRESS;
    }

    struct sockaddr_storage l_tClientAddr;
    socklen_t l_iAddrSize = sizeof(l_tClientAddr);

    int l_iNewSocket = accept(p_iSocket, (struct sockaddr*)&l_tClientAddr, &l_iAddrSize);
    return (l_iNewSocket < 0) ? NETWORK_ERROR : l_iNewSocket;
}

///////////////////////////////////////////////////////
/// connectSocket
///////////////////////////////////////////////////////
int connectSocket(int p_iSocket, NetworkAddress* p_ptAddress)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    if (!p_ptAddress) {
        return NETWORK_INVALID_ADDRESS;
    }

    if (p_ptAddress->t_usPort <= 0 || p_ptAddress->t_usPort >= 65535) {
        return NETWORK_INVALID_PORT;
    }

    struct sockaddr_in l_tSockaddr = { 0 };
    l_tSockaddr.sin_family = AF_INET;
    l_tSockaddr.sin_port = HOST_TO_NET_SHORT(p_ptAddress->t_usPort);

    if (inet_pton(AF_INET, p_ptAddress->t_cAddress, &l_tSockaddr.sin_addr) != 1) {
        return NETWORK_INVALID_ADDRESS;
    }

    int l_iResult = connect(p_iSocket, (struct sockaddr*)&l_tSockaddr, sizeof(l_tSockaddr));
    return (l_iResult < 0) ? NETWORK_ERROR : NETWORK_OK;
}

///////////////////////////////////////////////////////
/// sendData
///////////////////////////////////////////////////////
int sendData(int p_iSocket, const void* p_ptpBuffer, unsigned long p_ulSize)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    if (!p_ptpBuffer) {
        return NETWORK_INVALID_BUFFER;
    }

    if (p_ulSize == 0) {
        return NETWORK_INVALID_SIZE;
    }

    ssize_t l_iBytesSent = send(p_iSocket, p_ptpBuffer, p_ulSize, 0);
    return (l_iBytesSent < 0) ? NETWORK_ERROR : (int)l_iBytesSent;
}

///////////////////////////////////////////////////////
/// receiveData
///////////////////////////////////////////////////////
int receiveData(int p_iSocket, void* p_ptBuffer, int p_iSize)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    if (!p_ptBuffer) {
        return NETWORK_INVALID_BUFFER;
    }

    if (p_iSize <= 0) {
        return NETWORK_INVALID_SIZE;
    }

    ssize_t l_iBytesReceived = recv(p_iSocket, p_ptBuffer, p_iSize, 0);
    return (l_iBytesReceived < 0) ? NETWORK_ERROR : (int)l_iBytesReceived;
}

///////////////////////////////////////////////////////
/// closeSocket
///////////////////////////////////////////////////////
int closeSocket(int p_iSocket)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    int l_iResult = close(p_iSocket);
    return (l_iResult < 0) ? NETWORK_ERROR : NETWORK_OK;
}

///////////////////////////////////////////////////////
/// setSocketOption
///////////////////////////////////////////////////////
int setSocketOption(int p_iSocket, int p_iOption, int p_iValue)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    int l_iResult = setsockopt(p_iSocket, SOL_SOCKET, p_iOption,
        (const char*)&p_iValue, sizeof(p_iValue));
    return (l_iResult < 0) ? NETWORK_ERROR : NETWORK_OK;
}

///////////////////////////////////////////////////////
/// getSocketOption
///////////////////////////////////////////////////////
int getSocketOption(int p_iSocket, int p_iOption, int* p_ptiValue)
{
    if (p_iSocket <= 0) {
        return NETWORK_INVALID_SOCKET;
    }

    if (!p_ptiValue) {
        return NETWORK_INVALID_BUFFER;
    }

    socklen_t l_iOptLen = sizeof(int);
    int l_iResult = getsockopt(p_iSocket, SOL_SOCKET, p_iOption, p_ptiValue, &l_iOptLen);
    return (l_iResult < 0) ? NETWORK_ERROR : NETWORK_OK;
}
