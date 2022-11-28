#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <thread>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"


//Même structure que le serveur, pour qu'ils se comprennent.
 struct Paquet {
    std::string emetteur;
    std::string commande;
    std::string destinataire;   //Changer en liste plus tard
    std::string message;
};

 Paquet deserialize(char recvbuf[]) {
     std::string serialString;
     std::string delimiter = "//";
     serialString = recvbuf;

     Paquet paquet;

     //Je prends le premier bout du paquet sérialiser et je les sépares au //
     paquet.emetteur = serialString.substr(0, serialString.find(delimiter));
     //J'enlève le segment que j'ai trouvé
     serialString.erase(0, serialString.find(delimiter) + delimiter.length());
     //Je fais la même chose avec le reste
     paquet.commande = serialString.substr(0, serialString.find(delimiter));
     serialString.erase(0, serialString.find(delimiter) + delimiter.length());
     paquet.destinataire = serialString.substr(0, serialString.find(delimiter));
     serialString.erase(0, serialString.find(delimiter) + delimiter.length());
     paquet.message = serialString.substr(0, serialString.find(delimiter));
     serialString.erase(0, serialString.find(delimiter) + delimiter.length());

     return paquet;
 }

 //Fonction pour sérialiser un paquet
 std::string serialize(Paquet paquet) {
     std::string serialPaquet;

     serialPaquet = paquet.emetteur + "//" + paquet.commande + "//" + paquet.destinataire + "//" + paquet.message;

     return serialPaquet;
 }

 //Fonction où le client interprète les paquets reçus.
 void receive(Paquet paquet) {
     std::string str;

     //En ce moment, il y a aucune gestion d'erreur

     //Un client nous a envoyé un message
     if (paquet.commande == "sendTo") {
         str = paquet.emetteur + " : " + paquet.message + "\n";
         printf(str.c_str());
     }

     //On a reçu une liste de client
     else if (paquet.commande == "list") {
         str = "Clients connectes au serveur : " + paquet.message + "\n";
         printf(str.c_str());
     }

     //on a recu une erreur
     else {
         str = paquet.emetteur + " : " + paquet.message + "\n";
         printf(str.c_str());
     }
     
 }

 void handle_connection(SOCKET ConnectSocket, char recvbuf[], int recvbuflen) {
     int iResult;
     Paquet paquet;
     do {

         iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
         if (iResult > 0) {
             recvbuf[iResult] = 0;
             paquet = deserialize(recvbuf);
             receive(paquet);
             //printf("Bytes received: %d\n", iResult);
         }
         else if (iResult == 0) {
             printf("Connection closed\n");
             printf("appuyer sur enter pour tenter de vous reconnecter\n");
         }
         else {
             printf("recv failed with error: %d\n", WSAGetLastError());
             printf("appuyer sur enter pour tenter de vous reconnecter\n");
         }


     } while (iResult > 0);

 }

 SOCKET connexion(struct addrinfo* ptr, struct addrinfo* result, SOCKET ConnectSocket,int iResult) {
     int i = 0;

     while (i == 0) {

         for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

             // Create a SOCKET for connecting to server
             ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                 ptr->ai_protocol);
             if (ConnectSocket == INVALID_SOCKET) {
                 printf("socket failed with error: %ld\n", WSAGetLastError());
                 WSACleanup();
             }

             // Connect to server.
             iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
             if (iResult == SOCKET_ERROR) {
                 closesocket(ConnectSocket);
                 ConnectSocket = INVALID_SOCKET;
                 continue;
             }
             else {
                 i = 1;
             }
         }
         printf("Tentative de connexion !\n");

     }


     freeaddrinfo(result);

     //if (ConnectSocket == INVALID_SOCKET) {
         //printf("Unable to connect to server!\n");
       //  WSACleanup();
     //}

     printf("Vous etes connecte au serveur!\n");
     return ConnectSocket;
 }

 struct addrinfo* resolveServer(char** argv, struct addrinfo hints, struct addrinfo* result) {
     int iResult;
     iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
     if (iResult != 0) {
         printf("getaddrinfo failed with error: %d\n", iResult);
         WSACleanup();
     }
     return result;
 }

 void envoieConnexion(std::string nomClient,int iResult, SOCKET ConnectSocket, char** argv,struct addrinfo hints, struct addrinfo* ptr, struct addrinfo* result) {
     //On envoie un connect au serveur pour information sur mon nom automatiquement une fois connecté au serveur
     std::string listCommand = nomClient + "//connect////";
     const char* sendbuf = listCommand.c_str();
     iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
     if (iResult == SOCKET_ERROR) {
         printf("send failed with error: \n", WSAGetLastError());
         closesocket(ConnectSocket);
         WSACleanup();
         printf("Connexion perdu \n");
         result = resolveServer(argv, hints, result);
         ConnectSocket = connexion(ptr, result, ConnectSocket, iResult);
         printf("Connexion retablie \n");
         //return 1;
     }
     printf("Bytes Sent: %ld\n", iResult);
 }

 void afficherCommande() {
     printf("--------- Liste des commandes ---------  \n");
     printf("1 - list \n");
     printf("2 - sendTo//Benoit//Bonjour \n");
     printf("3 - sendTo//Benoit:Julien//Bonjour \n");
     printf("4 - sendTo//all//Bonjour \n");
     printf("5 - disconnect \n");
 }



int __cdecl main(int argc, char** argv)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    const char* sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    int disconnect = 0;
    std::string nomClient;
    std::string inputClient;

    //Demande au client de s'identifier
    do {
        printf("\nVeuillez entrer votre nom : ");
        std::getline(std::cin, nomClient);
    } while (nomClient == "");

    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        //return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = resolveServer(argv,hints,result);


    //Établie le socket avec le serveur
    ConnectSocket = connexion(ptr,result,ConnectSocket,iResult);

    //Envoie les informations du client au serveur
    envoieConnexion(nomClient, iResult, ConnectSocket, argv, hints, ptr, result);

    //Affiche les commandes
    afficherCommande();

    //Une fois officiellement connecté, thread pour gérer ce que le serveur lui envoie
    std::thread threadReception(handle_connection, ConnectSocket, recvbuf, recvbuflen);
    threadReception.detach();

    //Boucle simple pour écrire, ici on pourrait faire la vérification de la commande disconnect.
    while(disconnect == 0) {
        std::getline(std::cin, inputClient);
        if (inputClient != "disconnect") {
            inputClient = nomClient + "//" + inputClient;
            iResult = send(ConnectSocket, inputClient.c_str(), (int)strlen(inputClient.c_str()), 0);
            if (iResult == SOCKET_ERROR) {
                //printf("send failed with error: %d\n", WSAGetLastError());
                //closesocket(ConnectSocket);
                //WSACleanup();
                printf("Connexion perdu\n");
                result = resolveServer(argv, hints, result);
                ConnectSocket = connexion(ptr, result, ConnectSocket, iResult);
                envoieConnexion(nomClient, iResult, ConnectSocket, argv, hints, ptr, result);
                printf("Connexion retablie \n");

                std::thread threadReception(handle_connection, ConnectSocket, recvbuf, recvbuflen);
                threadReception.detach();
                //return 1;
        }


        } else {
                disconnect = 1;
        }
    }


}