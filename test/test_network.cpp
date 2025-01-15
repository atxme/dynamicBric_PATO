////////////////////////////////////////////////////////////
//  network test file
//  implements test cases for network functions
//
// Written : 15/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

extern "C" {
    #include "network/xNetwork.h"
}

class NetworkTest : public ::testing::Test {
protected:
    NetworkSocketCreation socketParams;
    NetworkAddress address;
    struct sockaddr_in sockAddr;
    
    void SetUp() override {
        memset(&socketParams, 0, sizeof(NetworkSocketCreation));
        memset(&address, 0, sizeof(NetworkAddress));
        memset(&sockAddr, 0, sizeof(struct sockaddr_in));
        
        // Configuration par défaut
        socketParams.t_iDomain = NETWORK_DOMAIN_IPV4;
        socketParams.t_iType = NETWORK_SOCK_TCP;
        socketParams.t_iProtocol = NETWORK_PROTOCOL_TCP;
        
        strcpy(address.t_cAddress, "127.0.0.1");
        address.t_usPort = 8080;
    }
};

// Test création de socket
TEST_F(NetworkTest, SocketCreation) {
    int socket = createSocket(&socketParams);
    EXPECT_GT(socket, 0);
    EXPECT_EQ(closeSocket(socket), NETWORK_OK);
}

// Test paramètres invalides
TEST_F(NetworkTest, InvalidParameters) {
    EXPECT_EQ(createSocket(nullptr), NETWORK_INVALID_SOCKET);
    
    socketParams.t_iDomain = -1;
    EXPECT_EQ(createSocket(&socketParams), NETWORK_INVALID_ADDRESS);
    
    socketParams.t_iDomain = NETWORK_DOMAIN_IPV4;
    socketParams.t_iType = -1;
    EXPECT_EQ(createSocket(&socketParams), NETWORK_INVALID_SOCKET);
}

// Test bind
TEST_F(NetworkTest, BindSocket) {
    int socket = createSocket(&socketParams);
    ASSERT_GT(socket, 0);
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = HOST_TO_NET_SHORT(address.t_usPort);
    
    EXPECT_EQ(bindSocket(socket, &address, (struct sockaddr*)&sockAddr), NETWORK_OK);
    EXPECT_EQ(closeSocket(socket), NETWORK_OK);
}

// Test connexion client/serveur
TEST_F(NetworkTest, ClientServerConnection) {
    // Création socket serveur
    int serverSocket = createSocket(&socketParams);
    ASSERT_GT(serverSocket, 0);
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = HOST_TO_NET_SHORT(address.t_usPort);
    
    EXPECT_EQ(bindSocket(serverSocket, &address, (struct sockaddr*)&sockAddr), NETWORK_OK);
    EXPECT_EQ(listenSocket(serverSocket, 1), NETWORK_OK);
    
    // Thread client
    std::thread clientThread([this]() {
        NetworkSocketCreation clientParams = socketParams;
        int clientSocket = createSocket(&clientParams);
        EXPECT_GT(clientSocket, 0);
        
        EXPECT_EQ(connectSocket(clientSocket, &address), NETWORK_OK);
        
        // Test envoi/réception
        const char* testMsg = "Test Message";
        EXPECT_GT(sendData(clientSocket, testMsg, strlen(testMsg)), 0);
        
        closeSocket(clientSocket);
    });
    
    // Acceptation connexion serveur
    NetworkAddress clientAddr;
    int clientSocket = acceptConnection(serverSocket, &clientAddr);
    EXPECT_GT(clientSocket, 0);
    
    // Test réception
    char buffer[256] = {0};
    EXPECT_GT(receiveData(clientSocket, buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "Test Message");
    
    clientThread.join();
    closeSocket(clientSocket);
    closeSocket(serverSocket);
}

// Test options socket
TEST_F(NetworkTest, SocketOptions) {
    int socket = createSocket(&socketParams);
    ASSERT_GT(socket, 0);
    
    int value = 1;
    EXPECT_EQ(setSocketOption(socket, SO_REUSEADDR, value), NETWORK_OK);
    
    int getValue = 0;
    EXPECT_EQ(getSocketOption(socket, SO_REUSEADDR, &getValue), NETWORK_OK);
    EXPECT_EQ(getValue, value);
    
    closeSocket(socket);
}

// Test timeout et non-bloquant
TEST_F(NetworkTest, NonBlockingAndTimeout) {
    int socket = createSocket(&socketParams);
    ASSERT_GT(socket, 0);
    
    EXPECT_EQ(setSocketOption(socket, NETWORK_SOCK_NONBLOCKING, 1), NETWORK_OK);
    
    // Test réception non bloquante
    char buffer[256];
    EXPECT_EQ(receiveData(socket, buffer, sizeof(buffer)), NETWORK_ERROR);
    
    closeSocket(socket);
}
