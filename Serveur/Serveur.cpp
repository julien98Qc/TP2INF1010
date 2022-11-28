#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <vector>
#include <string>
#include <list>
#include <iostream>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

//Structure du paquet, si changement. Devrait être changé dans le client
struct Paquet {
    std::string emetteur;
    std::string commande;
    std::string destinataire;       //Doit être changer en liste plus tard
    std::string message;
};

//Structure du client qui sera contenu dans notre liste de clients connectés (nom et socket)
struct Client {
    std::string nom;
    SOCKET ClientSocket;
};

//Transforme le paquet reçu en un paquet utilisable
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

//Fonction qui gère l'envoie de la liste des clients connecté lors de la déconnexion d'un client'
void disconnectList(std::list<Client>* connectedClients) {
    Paquet responsePaquet;
    int iSendResult;
    std::string str;
    responsePaquet.message = "";


    std::list<Client>::iterator itri = connectedClients->begin();
    while (itri != connectedClients->end()) {
        responsePaquet.message += itri->nom + ", ";
        itri++;
    }

    //Enleve la virgule a la fin
    responsePaquet.message = responsePaquet.message.substr(0, responsePaquet.message.size() - 2);

    responsePaquet.emetteur = "serveur";
    responsePaquet.commande = "list";

    //Envoie à tous les clients
    itri = connectedClients->begin();
    while (itri != connectedClients->end()) {

        responsePaquet.destinataire = itri->nom;
        //on serialize les informations en un paquet envoyable
        std::string serialPaquet = serialize(responsePaquet);
        iSendResult = send(itri->ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);
        itri++;
    }

    printf("La liste des clients a ete envoyee a tous les clients \n");
}

void connectCommand(Paquet paquet, struct Client client, std::list<Client>* connectedClients) {
    int iSendResult;
    const char* recvbuf = "";
    Paquet responsePaquet;
    std::string str;
    //Le client envoie son nom dans le paquet, on tient à jour notre liste de clients.
    client.nom = paquet.emetteur;
    //Parcours la liste des clients connectés
    std::list<Client>::iterator itri = connectedClients->begin();
    while (itri != connectedClients->end()) {
        if (itri->ClientSocket == client.ClientSocket) {
            itri->nom = paquet.emetteur;        //Mise à jour du nom du client dans notre liste de clients
            break;
        }
        else {
            ++itri;
        }
    }

    //Parcours la liste des clients connectés pour les ajouter à notre string
    itri = connectedClients->begin();
    while (itri != connectedClients->end()) {
        responsePaquet.message += itri->nom + ", ";
        itri++;
    }

    //Enleve la virgule a la fin
    responsePaquet.message = responsePaquet.message.substr(0, responsePaquet.message.size() - 2);

    responsePaquet.emetteur = "serveur";
    responsePaquet.commande = "list";

    //Envoie à tous les clients
    itri = connectedClients->begin();
    while (itri != connectedClients->end()) {

        responsePaquet.destinataire = itri->nom;
        //on serialize les informations en un paquet envoyable
        std::string serialPaquet = serialize(responsePaquet);
        iSendResult = send(itri->ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);

        //Vérification erreur et regarde si le client est toujours connecter
        if (iSendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(client.ClientSocket);
            //On l'enlève de l'ensemble des clients connectés
            itri = connectedClients->begin();
            while (itri != connectedClients->end()) {
                if (itri->nom == client.nom) {
                    connectedClients->erase(itri++);
                    disconnectList(connectedClients);
                }

                else {
                    ++itri;
                }
            }
            WSACleanup();
        }
        itri++;
    }
    printf("La liste des clients a ete envoyee a tous les clients \n");
}

void listCommand(Paquet paquet, struct Client client, std::list<Client>* connectedClients) {
    int iSendResult;
    const char* recvbuf = "";
    Paquet responsePaquet;
    std::string str;

    responsePaquet.message = "";

    //Parcours la liste des clients connectés pour les ajouter à notre string
    std::list<Client>::iterator itri = connectedClients->begin();
    while (itri != connectedClients->end()) {
        responsePaquet.message += itri->nom + ", ";
        itri++;
    }

    //Enleve la virgule a la fin
    responsePaquet.message = responsePaquet.message.substr(0, responsePaquet.message.size() - 2);
    responsePaquet.emetteur = "serveur";
    responsePaquet.commande = "list";
    responsePaquet.destinataire = paquet.emetteur;

    //on serialize les informations en un paquet envoyable
    std::string serialPaquet = serialize(responsePaquet);
    iSendResult = send(client.ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);
    //Vérification erreur et regarde si le client est toujours connecter
    if (iSendResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(client.ClientSocket);
        //On l'enlève de l'ensemble des clients connectés
        itri = connectedClients->begin();
        while (itri != connectedClients->end()) {
            if (itri->nom == client.nom) {
                connectedClients->erase(itri++);
                disconnectList(connectedClients);
            }

            else {
                ++itri;
            }
        }
        WSACleanup();
    }
    str = "La liste des clients connectes a ete envoyee a " + responsePaquet.destinataire + "\n";
    printf(str.c_str());
}

void sendToCommand(Paquet paquet, struct Client client, std::list<Client>* connectedClients) {
    int iSendResult;
    const char* recvbuf = "";
    Paquet responsePaquet = paquet;
    std::string str;

    //on serialize les informations en un paquet envoyable
    std::string serialPaquet = serialize(responsePaquet);

    std::list<Client>::iterator itri = connectedClients->begin();
    //sendTo à tous les clients
    if (paquet.destinataire == "all") {
        while (itri != connectedClients->end()) {
            if (itri->nom != paquet.emetteur) {
                iSendResult = send(itri->ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);
                //Vérification erreur et regarde si le client est toujours connecter
                if (iSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(client.ClientSocket);

                    //On l'enlève de l'ensemble des clients connectés
                    itri = connectedClients->begin();
                    while (itri != connectedClients->end()) {
                        if (itri->nom == client.nom) {
                            connectedClients->erase(itri);
                            disconnectList(connectedClients);
                        }

                        else {
                            ++itri;
                        }
                    }
                    WSACleanup();
                }
            }
            ++itri;
        }
        str = "Message de " + responsePaquet.emetteur + " a tous les utilisatuers : " + responsePaquet.message + "\n";
        printf(str.c_str());
    }

    //sendTo à un ou plusieurs clients
    else {
        size_t pos = 0;
        std::string nom;
        std::string separator = ":";
        std::string fullDesti = paquet.destinataire;
        bool destExist = false;

        //on vient chercher chaque destinataire séparé par des :, puis on leur envoie le message
        while ((pos = fullDesti.find(":")) != std::string::npos) {

            //on extrait un nom de la chaine
            nom = fullDesti.substr(0, pos);

            //on remet au debut
            itri = connectedClients->begin();


            while (itri != connectedClients->end()) {
                if (itri->nom == nom) {
                    iSendResult = send(itri->ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);
                    destExist = true;
                    //Vérification erreur et regarde si le client est toujours connecter
                    if (iSendResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", WSAGetLastError());
                        closesocket(client.ClientSocket);

                        //On l'enlève de l'ensemble des clients connectés
                        itri = connectedClients->begin();
                        while (itri != connectedClients->end()) {
                            if (itri->nom == client.nom) {
                                connectedClients->erase(itri);
                                disconnectList(connectedClients);
                                break;
                            }

                            else {
                                ++itri;
                            }
                        }
                        WSACleanup();
                    }
                    str = "Message de " + responsePaquet.emetteur + " a " + nom + " : " + responsePaquet.message + "\n";
                    printf(str.c_str());
                    break;
                }
                else {
                    ++itri;
                }
            }
            //retire le client de la liste des clients restant à recevoir le message
            fullDesti.erase(0, pos + separator.length());
            //Vérification destinataire existe
            if (!destExist) {
                responsePaquet = paquet;
                responsePaquet.message = "Le destinataire " + nom + " n'existe pas.\n";
                responsePaquet.destinataire = responsePaquet.emetteur;
                responsePaquet.emetteur = "Serveur";
                printf(responsePaquet.message.c_str());
                //on serialize les informations en un paquet envoyable
                std::string serialPaquet = serialize(responsePaquet);
                iSendResult = send(client.ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);
            }
        }

        //on remet au debut
        itri = connectedClients->begin();
        destExist = false;
        //on envoie le message au dernier client
        while (itri != connectedClients->end()) {
            if (itri->nom == fullDesti) {
                iSendResult = send(itri->ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);
                destExist = true;
                //Vérification erreur et regarde si le client est toujours connecter
                if (iSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(client.ClientSocket);

                    //On l'enlève de l'ensemble des clients connectés
                    itri = connectedClients->begin();
                    while (itri != connectedClients->end()) {
                        if (itri->nom == client.nom) {
                            connectedClients->erase(itri);
                            disconnectList(connectedClients);
                            break;
                        }

                        else {
                            ++itri;
                        }
                    }
                    WSACleanup();
                }
                str = "Message de " + responsePaquet.emetteur + " a " + fullDesti + " : " + responsePaquet.message + "\n";
                printf(str.c_str());
                break;
            }

            else {
                ++itri;
            }
        }
        //Vérification destinataire existe
        if ((!destExist) && fullDesti != "") {
            responsePaquet = paquet;
            responsePaquet.message = "Le destinataire " + fullDesti + " n'existe pas.\n";
            responsePaquet.destinataire = responsePaquet.emetteur;
            responsePaquet.emetteur = "Serveur";
            printf(responsePaquet.message.c_str());
            //on serialize les informations en un paquet envoyable
            std::string serialPaquet = serialize(responsePaquet);
            iSendResult = send(client.ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);
        }
    }
}

//Fonction où le serveur exécute la commande du client
void executeCommand(Paquet paquet, struct Client client, std::list<Client>* connectedClients) {
    int iSendResult;
    const char* recvbuf = "";
    Paquet responsePaquet;
    std::string str;

    //Lorsque nouvelle connexion, list à tous les utilisateurs
    if (paquet.commande == "connect") 
        connectCommand(paquet, client, connectedClients); 

    //La commande list renvoie à l'utilisateur l'ensemble des clients connectés au serveur
    else if (paquet.commande == "list") 
        listCommand(paquet, client, connectedClients);

    //la commande sendTo envoie un message à un utilisateur ou plusieurs utilisateurs
    else if (paquet.commande == "sendTo") 
        sendToCommand(paquet, client, connectedClients);
    
    //Erreur de commande
    else {
        responsePaquet = paquet;
        responsePaquet.message = "Commande non reconnue";
        responsePaquet.destinataire = responsePaquet.emetteur;
        responsePaquet.emetteur = "Serveur";

        //on serialize les informations en un paquet envoyable
        std::string serialPaquet = serialize(responsePaquet);

        std::list<Client>::iterator itri = connectedClients->begin();

        while (itri != connectedClients->end()) {
            if (itri->nom == responsePaquet.destinataire) {
                iSendResult = send(itri->ClientSocket, serialPaquet.c_str(), strlen(serialPaquet.c_str()), 0);
                //Vérification erreur et regarde si le client est toujours connecter
                if (iSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(client.ClientSocket);

                    //On l'enlève de l'ensemble des clients connectés
                    itri = connectedClients->begin();
                    while (itri != connectedClients->end()) {
                        if (itri->nom == client.nom) {
                            connectedClients->erase(itri);
                            disconnectList(connectedClients);
                            break;
                        }

                        else {
                            ++itri;
                        }
                    }
                    WSACleanup();
                }
                str = "Message de " + responsePaquet.emetteur + " a " + responsePaquet.destinataire + " : " + responsePaquet.message + "\n";
                printf(str.c_str());
                break;
            }

            else {
                ++itri;
            }
        }
    }

}

//Fonction qui prend en charge chacune des connexions au serveur faites par les clients
int handle_connection(struct Client client, std::list<Client>* connectedClients) {
    int iResult;
    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    //On reçoit tant et aussi longtemps que le client ne se déconnecte pas du serveur
    do {

        iResult = recv(client.ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            recvbuf[iResult] = 0;
            printf("Donnees recues: %s\n", recvbuf);
            //printf("Bytes received: %d\n", iResult);
            executeCommand(deserialize(recvbuf), client, connectedClients);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(client.ClientSocket);
            //On l'enlève de l'ensemble des clients connectés
            std::list<Client>::iterator itri = connectedClients->begin();
            while (itri != connectedClients->end()) {
                if (itri->ClientSocket == client.ClientSocket) {
                    std::cout << itri->nom << " a disconnect \n";
                    connectedClients->erase(itri);
                    disconnectList(connectedClients);
                    break;
                }

                else {
                    ++itri;
                }
            }
            //WSACleanup();
            //return 1;
        }

    } while (iResult > 0);
}

int __cdecl main(void)
{
    std::vector<std::thread> ThreadVector;
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    std::list<Client>* ClientListPointer;
    std::list<Client> ClientList;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // prépare les informations pour le serveur (adresse et port)
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    //Crée le socket qui permet au serveur d'écouter les clients qui se connecte.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Prépare le TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    //On commence l'écoute
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    printf("Le serveur est maintenant a l'ecoute\n");

    //Le serveur est toujours en écoute pour un nouveau client
    while (true) {      
        //J'accepte un nouveau client et l'insère dans ma struct Client (nom, socket)
        Client newClient = { "", accept(ListenSocket, NULL, NULL) };

        //Initialisation du pointer vers notre liste de clients qui sera partagée par tous les threads
        ClientListPointer = &ClientList;
        ClientListPointer->push_back(newClient);
        if (ClientListPointer->back().ClientSocket == INVALID_SOCKET) {
            closesocket(ListenSocket);
            WSACleanup();
            //return 1;
        }
        else {
        //Pour chaque nouveau client, un thread est créé et reçoit en paramètre le client et un pointeur vers la liste des clients du serveur.
        ThreadVector.emplace_back([&]() {handle_connection(newClient, ClientListPointer); }); 
        printf("Une nouvelle connexion au serveur.\n");
        }


    }
    
    // Ferme le socket d'écoute
    closesocket(ListenSocket);

    // cleanup
    WSACleanup();

    return 0;
}