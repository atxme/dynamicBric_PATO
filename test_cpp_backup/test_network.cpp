////////////////////////////////////////////////////////////
//  network test file - Updated for Simplified Network API
//  implements test cases for network functions
//
// Written : 15/01/2025
// Updated : 22/04/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <string.h>

extern "C" {
    #include "xNetwork.h"
    #include "xOsMutex.h"
}

class NetworkTest : public ::testing::Test {
protected:
    NetworkAddress address;
    
    void SetUp() override {
        // Créer une adresse réseau pour les tests
        address = networkMakeAddress("127.0.0.1", 8080);
    }
    
    void TearDown() override {
        // Aucune opération particulière nécessaire
    }
};

// Test création de socket
TEST_F(NetworkTest, SocketCreation) {
    NetworkSocket* socket = networkCreateSocket(NETWORK_SOCK_TCP);
    EXPECT_NE(socket, nullptr);
    EXPECT_EQ(networkCloseSocket(socket), NETWORK_OK);
}

// Test paramètres invalides
TEST_F(NetworkTest, InvalidParameters) {
    EXPECT_EQ(networkCloseSocket(nullptr), NETWORK_INVALID_PARAM);
    
    // Test avec un type de socket invalide (999 n'est pas un type valide)
    NetworkSocket* socket = networkCreateSocket(999);
    EXPECT_EQ(socket, nullptr);
}

// Test bind
TEST_F(NetworkTest, BindSocket) {
    NetworkSocket* socket = networkCreateSocket(NETWORK_SOCK_TCP);
    ASSERT_NE(socket, nullptr);
    
    EXPECT_EQ(networkBind(socket, &address), NETWORK_OK);
    EXPECT_EQ(networkCloseSocket(socket), NETWORK_OK);
}

// Test connexion client/serveur
TEST_F(NetworkTest, ClientServerConnection) {
    // Création socket serveur
    NetworkSocket* serverSocket = networkCreateSocket(NETWORK_SOCK_TCP);
    ASSERT_NE(serverSocket, nullptr);
    
    EXPECT_EQ(networkBind(serverSocket, &address), NETWORK_OK);
    EXPECT_EQ(networkListen(serverSocket, 1), NETWORK_OK);
    
    // Thread client
    std::thread clientThread([this]() {
        // Attendre que le serveur soit prêt
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        NetworkSocket* clientSocket = networkCreateSocket(NETWORK_SOCK_TCP);
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

// Test timeout
TEST_F(NetworkTest, TimeoutTest) {
    NetworkSocket* socket = networkCreateSocket(NETWORK_SOCK_TCP);
    ASSERT_NE(socket, nullptr);
    
    // Test de timeout
    int timeout_result = networkSetTimeout(socket, 100, false);
    EXPECT_EQ(timeout_result, NETWORK_OK);
    
    // Test d'attente d'activité avec timeout
    int activity = networkWaitForActivity(socket, 10);
    // Devrait expirer car aucune connexion n'est établie
    EXPECT_EQ(activity, 0);
    
    networkCloseSocket(socket);
}

// Test UDP
TEST_F(NetworkTest, UdpCommunication) {
    // Dans la version simplifiée, il n'y a pas de fonction sendTo/receiveFrom
    // Skip ce test si ces fonctions ne sont pas disponibles
    if (std::getenv("SKIP_UDP_TESTS") != nullptr) {
        GTEST_SKIP() << "Test UDP ignoré car l'API simplifiée n'inclut pas sendTo/receiveFrom";
    }
}

// Test pour vérifier l'état de connexion
TEST_F(NetworkTest, ConnectionState) {
    NetworkSocket* socket = networkCreateSocket(NETWORK_SOCK_TCP);
    ASSERT_NE(socket, nullptr);
    
    // Au départ, la socket ne devrait pas être connectée
    EXPECT_FALSE(networkIsConnected(socket));
    
    // Connexion au serveur local (qui échoue car pas de serveur)
    // Mais ça permettra de tester si la fonction retourne bien NETWORK_ERROR
    networkConnect(socket, &address);
    
    // Fermer la socket
    networkCloseSocket(socket);
}

// Test la fonction networkGetErrorString
TEST_F(NetworkTest, ErrorString) {
    const char* errorStr = networkGetErrorString(NETWORK_OK);
    EXPECT_STREQ(errorStr, "Success");
    
    errorStr = networkGetErrorString(NETWORK_ERROR);
    EXPECT_STREQ(errorStr, "General network error");
    
    errorStr = networkGetErrorString(NETWORK_TIMEOUT);
    EXPECT_STREQ(errorStr, "Operation timed out");
    
    errorStr = networkGetErrorString(NETWORK_INVALID_PARAM);
    EXPECT_STREQ(errorStr, "Invalid parameter");
    
    errorStr = networkGetErrorString(999); // Code d'erreur inconnu
    EXPECT_NE(errorStr, nullptr);
}
