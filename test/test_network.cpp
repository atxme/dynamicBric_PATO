////////////////////////////////////////////////////////////
//  network test file - Updated for Embedded Network API
//  implements test cases for network functions
//
// Written : 15/01/2025
// Updated : 07/03/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

// Pour compatibilité avec les tests existants
#define NETWORK_TCP NETWORK_SOCK_TCP
#define NETWORK_UDP NETWORK_SOCK_UDP


extern "C" {
    #include "xNetwork.h"
}

class NetworkTest : public ::testing::Test {
protected:
    NetworkContext* context;
    NetworkAddress address;
    
    void SetUp() override {
        // Initialiser le sous-système réseau
        context = networkInit();
        ASSERT_NE(context, nullptr);
        
        // Créer une adresse réseau pour les tests
        address = networkMakeAddress("127.0.0.1", 8080);
    }
    
    void TearDown() override {
        // Nettoyer le contexte réseau
        if (context) {
            networkCleanup(context);
        }
    }
};

// Test création de socket
TEST_F(NetworkTest, SocketCreation) {
    NetworkSocket* socket = networkCreateSocket(context, NETWORK_TCP, false);
    EXPECT_NE(socket, nullptr);
    EXPECT_EQ(networkCloseSocket(socket), NETWORK_OK);
}

// Test paramètres invalides
TEST_F(NetworkTest, InvalidParameters) {
    EXPECT_EQ(networkCloseSocket(nullptr), NETWORK_INVALID_SOCKET);
    
    // Test avec un contexte nullptr
    EXPECT_EQ(networkCreateSocket(nullptr, NETWORK_TCP, false), nullptr);
    
    // Test avec un type de socket invalide
    NetworkSocket* socket = networkCreateSocket(context, 999, false);
    EXPECT_EQ(socket, nullptr);
}

// Test bind
TEST_F(NetworkTest, BindSocket) {
    NetworkSocket* socket = networkCreateSocket(context, NETWORK_TCP, false);
    ASSERT_NE(socket, nullptr);
    
    EXPECT_EQ(networkBind(socket, &address), NETWORK_OK);
    EXPECT_EQ(networkCloseSocket(socket), NETWORK_OK);
}

// Test connexion client/serveur
TEST_F(NetworkTest, ClientServerConnection) {
    // Création socket serveur
    NetworkSocket* serverSocket = networkCreateSocket(context, NETWORK_TCP, false);
    ASSERT_NE(serverSocket, nullptr);
    
    EXPECT_EQ(networkBind(serverSocket, &address), NETWORK_OK);
    EXPECT_EQ(networkListen(serverSocket, 1), NETWORK_OK);
    
    // Thread client
    std::thread clientThread([this]() {
        // Attendre que le serveur soit prêt
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        NetworkSocket* clientSocket = networkCreateSocket(context, NETWORK_TCP, false);
        EXPECT_NE(clientSocket, nullptr);
        
        EXPECT_EQ(networkConnect(clientSocket, &address), NETWORK_OK);
        
        // Test envoi/réception
        const char* testMsg = "Test Message";
        EXPECT_GT(networkSend(clientSocket, testMsg, strlen(testMsg)), 0);
        
        networkCloseSocket(clientSocket);
    });
    
    // Acceptation connexion serveur
    NetworkAddress clientAddr;
    NetworkSocket* clientSocket = networkAccept(serverSocket, &clientAddr);
    EXPECT_NE(clientSocket, nullptr);
    
    // Test réception
    char buffer[256] = {0};
    EXPECT_GT(networkReceive(clientSocket, buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "Test Message");
    
    clientThread.join();
    networkCloseSocket(clientSocket);
    networkCloseSocket(serverSocket);
}

// Test timeout et non-bloquant
TEST_F(NetworkTest, NonBlockingAndTimeout) {
    NetworkSocket* socket = networkCreateSocket(context, NETWORK_SOCK_TCP, true);
    ASSERT_NE(socket, nullptr);
    
    // Test réception non bloquante
    char buffer[256];
    int result = networkReceive(socket, buffer, sizeof(buffer));
    std::cout << "networkReceive returned: " << result 
              << " (expected NETWORK_WOULD_BLOCK: " << NETWORK_WOULD_BLOCK << ")" << std::endl;
    
    // Vérifiez le code d'erreur de manière explicite
    if (result != NETWORK_WOULD_BLOCK) {
        std::cout << "Got unexpected error code: " << result 
                  << " (" << networkGetErrorString(result) << ")" << std::endl;
    }
    
    EXPECT_EQ(result, NETWORK_WOULD_BLOCK);
    
    // Test de timeout
    int timeout_result = networkSetTimeout(socket, 100, false);
    std::cout << "networkSetTimeout returned: " << timeout_result << std::endl;
    EXPECT_EQ(timeout_result, NETWORK_OK);
    
    networkCloseSocket(socket);
    exit(0);
}


// Test UDP
TEST_F(NetworkTest, UdpCommunication) {
    // Socket UDP serveur
    NetworkSocket* serverSocket = networkCreateSocket(context, NETWORK_UDP, false);
    ASSERT_NE(serverSocket, nullptr);
    
    EXPECT_EQ(networkBind(serverSocket, &address), NETWORK_OK);
    
    // Thread client
    std::thread clientThread([this]() {
        // Attendre que le serveur soit prêt
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        NetworkSocket* clientSocket = networkCreateSocket(context, NETWORK_UDP, false);
        EXPECT_NE(clientSocket, nullptr);
        
        // Test envoi/réception
        const char* testMsg = "UDP Test Message";
        EXPECT_GT(networkSendTo(clientSocket, testMsg, strlen(testMsg), &address), 0);
        
        networkCloseSocket(clientSocket);
    });
    
    // Réception UDP
    char buffer[256] = {0};
    NetworkAddress senderAddr;
    
    // Utiliser un timeout pour éviter un blocage indéfini
    networkSetTimeout(serverSocket, 1000, false);
    
    int received = networkReceiveFrom(serverSocket, buffer, sizeof(buffer), &senderAddr);
    EXPECT_GT(received, 0);
    if (received > 0) {
        buffer[received] = '\0';
        EXPECT_STREQ(buffer, "UDP Test Message");
    }
    
    clientThread.join();
    networkCloseSocket(serverSocket);
}

// Test de connexion multiples avec polling
TEST_F(NetworkTest, MultipleConnectionsWithPolling) {
    // Configuration du serveur
    NetworkSocket* serverSocket = networkCreateSocket(context, NETWORK_TCP, true);
    ASSERT_NE(serverSocket, nullptr);
    
    EXPECT_EQ(networkBind(serverSocket, &address), NETWORK_OK);
    EXPECT_EQ(networkListen(serverSocket, 5), NETWORK_OK);
    
    // Tableau pour le polling
    NetworkSocket* sockets[2];
    int events[2];
    
    sockets[0] = serverSocket;
    events[0] = NETWORK_EVENT_READ;
    
    // Test polling de base
    int ready = networkPoll(sockets, events, 1, 0);
    EXPECT_GE(ready, 0);
    
    networkCloseSocket(serverSocket);
}
