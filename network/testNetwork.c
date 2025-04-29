/**
 * network_test_main.c
 * Programme de test pour la bibliothèque réseau embarquée
 */

 #include <stdio.h>
 #include <string.h>
 #include <unistd.h>
 #include <signal.h>
 #include <stdlib.h>
 #include <stdbool.h>
 #include "xNetwork.h"
 
 #define TEST_PORT 8080
 #define BUFFER_SIZE 1024
 
 // Drapeau pour arrêter le serveur
 volatile bool running = true;
 
 // Gestionnaire de signal pour arrêter proprement
 void sig_handler(int signo) {
     if (signo == SIGINT) {
         printf("\nArrêt demandé...\n");
         running = false;
     }
 }
 
 // Fonction de serveur
 void run_server() {
     printf("Démarrage du serveur sur le port %d...\n", TEST_PORT);
     
     // Initialiser le contexte réseau
     NetworkContext* context = networkInit();
     if (!context) {
         printf("Erreur: Impossible d'initialiser le contexte réseau\n");
         return;
     }
     
     // Créer un socket d'écoute
     NetworkSocket* serverSocket = networkCreateSocket(context, NETWORK_SOCK_TCP, NETWORK_SOCK_NONBLOCKING);
     if (!serverSocket) {
         printf("Erreur: Impossible de créer le socket serveur\n");
         networkCleanup(context);
         return;
     }
     
     // Créer l'adresse d'écoute
     NetworkAddress address = networkMakeAddress("0.0.0.0", TEST_PORT);
     
     // Lier le socket à l'adresse
     if (networkBind(serverSocket, &address) != NETWORK_OK) {
         printf("Erreur: Impossible de lier le socket à l'adresse\n");
         networkCloseSocket(serverSocket);
         networkCleanup(context);
         return;
     }
     
     // Mettre le socket en écoute
     if (networkListen(serverSocket, 5) != NETWORK_OK) {
         printf("Erreur: Impossible de mettre le socket en écoute\n");
         networkCloseSocket(serverSocket);
         networkCleanup(context);
         return;
     }
     
     printf("Serveur en écoute sur le port %d\n", TEST_PORT);
     
     // Boucle principale du serveur
     while (running) {
         // Accepter une connexion
         NetworkAddress clientAddr;
         NetworkSocket* clientSocket = networkAccept(serverSocket, &clientAddr);
         
         if (clientSocket) {
             printf("Nouvelle connexion de %s:%d\n", clientAddr.t_cAddress, clientAddr.t_usPort);
             
             // Recevoir des données
             char buffer[BUFFER_SIZE];
             int received = networkReceive(clientSocket, buffer, sizeof(buffer));
             
             if (received > 0) {
                 buffer[received] = '\0';
                 printf("Message reçu: %s\n", buffer);
                 
                 // Répondre au client
                 char response[BUFFER_SIZE];
                 snprintf(response, sizeof(response), "Serveur a reçu: %s", buffer);
                 networkSend(clientSocket, response, strlen(response));
             }
             
             // Fermer la connexion client
             networkCloseSocket(clientSocket);
         }
         
         // Petite pause pour éviter de surcharger le CPU
         usleep(10000);  // 10ms
     }
     
     // Nettoyage
     printf("Arrêt du serveur...\n");
     networkCloseSocket(serverSocket);
     networkCleanup(context);
     printf("Serveur arrêté\n");
 }
 
 // Fonction de client
 void run_client(const char* message) {
     printf("Démarrage du client...\n");
     
     // Initialiser le contexte réseau
     NetworkContext* context = networkInit();
     if (!context) {
         printf("Erreur: Impossible d'initialiser le contexte réseau\n");
         return;
     }
     
     // Créer un socket client
     NetworkSocket* clientSocket = networkCreateSocket(context, NETWORK_SOCK_TCP, NETWORK_SOCK_BLOCKING);
     if (!clientSocket) {
         printf("Erreur: Impossible de créer le socket client\n");
         networkCleanup(context);
         return;
     }
     
     // Créer l'adresse du serveur
     NetworkAddress serverAddr = networkMakeAddress("127.0.0.1", TEST_PORT);
     
     // Se connecter au serveur
     printf("Connexion au serveur...\n");
     if (networkConnect(clientSocket, &serverAddr) != NETWORK_OK) {
         printf("Erreur: Impossible de se connecter au serveur\n");
         networkCloseSocket(clientSocket);
         networkCleanup(context);
         return;
     }
     
     printf("Connecté au serveur\n");
     
     // Envoyer un message
     printf("Envoi du message: %s\n", message);
     if (networkSend(clientSocket, message, strlen(message)) < 0) {
         printf("Erreur: Impossible d'envoyer le message\n");
         networkCloseSocket(clientSocket);
         networkCleanup(context);
         return;
     }
     
     // Recevoir la réponse
     char buffer[BUFFER_SIZE];
     int received = networkReceive(clientSocket, buffer, sizeof(buffer));
     
     if (received > 0) {
         buffer[received] = '\0';
         printf("Réponse du serveur: %s\n", buffer);
     } else {
         printf("Aucune réponse du serveur ou erreur\n");
     }
     
     // Nettoyage
     networkCloseSocket(clientSocket);
     networkCleanup(context);
     printf("Client terminé\n");
 }
 
 // Fonction principale
 int main(int argc, char* argv[]) {
     // Configurer le gestionnaire de signal
     signal(SIGINT, sig_handler);
     
     if (argc < 2) {
         printf("Usage: %s [server|client <message>]\n", argv[0]);
         return 1;
     }
     
     if (strcmp(argv[1], "server") == 0) {
         run_server();
     } else if (strcmp(argv[1], "client") == 0) {
         if (argc < 3) {
             printf("Erreur: Le client nécessite un message\n");
             printf("Usage: %s client <message>\n", argv[0]);
             return 1;
         }
         run_client(argv[2]);
     } else {
         printf("Mode inconnu: %s\n", argv[1]);
         printf("Usage: %s [server|client <message>]\n", argv[0]);
         return 1;
     }
     
     return 0;
 }
 