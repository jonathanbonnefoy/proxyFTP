#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>

#define SERVADDR "localhost"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, 
                                    // si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille du tampon de demande de connexion
#define MAXBUFFERLEN 1024
#define MAXHOSTLEN 64
#define MAXPORTLEN 6

int main(){
	int ecode;                       					// Code retour des fonctions
	char serverAddr[MAXHOSTLEN];     					// Adresse du serveur
	char serverPort[MAXPORTLEN];     					// Port du server
	int descSockRDV;                					// Descripteur de socket de rendez-vous
	int descSockCOM;                 					// Descripteur de socket de communication
	int clientData;					 					// Descripteur de socket de donnees du client
	int serveurData;				 					// Descripteur de socket de donnes du serveur
	struct addrinfo hints;           					// Contrôle la fonction getaddrinfo
	struct addrinfo hintsClientData; 					// Controle la fonction getaddrinfo pour le client
	struct addrinfo hintsServeurData;					// Controle la fonction getaddrinfo pour le serveur
	struct addrinfo *res, *resPtr;   					// Contient le résultat de la fonction getaddrinfo client
	struct addrinfo *resServeur, *resServeurPtr;	 	// Contient le résultat de la fonction getaddrinfo serveur
	struct addrinfo *resServeurData, *resServeurDataPtr;// Contient le res getaddrinfo serveur data
	struct addrinfo *resClientData, *resClientDataPtr;  // Contient le res getaddrinfo client data
	struct sockaddr_storage myinfo;  					// Informations sur la connexion de RDV
	struct sockaddr_storage from;    					// Informations sur le client connecté
	socklen_t len;                   					// Variable utilisée pour stocker les 
				                    					// longueurs des structures de socket
	char buffer[MAXBUFFERLEN];       					// Tampon de communication entre le client et le serveur
    
	// Publication de la socket au niveau du système
	// Assignation d'une adresse IP et un numéro de port
	// Mise à zéro de hints
	memset(&hints, 0, sizeof(hints));
	// Initailisation de hints
	hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_family = AF_INET;      // les adresses IPv4 et IPv6 seront présentées par 
				                      // la fonction getaddrinfo

	// Récupération des informations du serveur avec getaddrinfo()
	ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
	if (ecode) {
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	//Création de la socket IPv4/TCP avec socket()
	descSockRDV = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (descSockRDV == -1) {
		perror("Erreur creation socket");
		exit(4);
	}

	// Publication de la socket avec un nom via la fonction bind()
	ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
	if (ecode == -1) {
		perror("Erreur liaison de la socket de RDV");
		exit(3);
	}
	// Nous n'avons plus besoin de cette liste chainée addrinfo
	freeaddrinfo(res);

	// Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
	len=sizeof(struct sockaddr_storage);
	ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
	if (ecode == -1)
	{
		perror("SERVEUR: getsockname");
		exit(4);
	}
	ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, 
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
	if (ecode != 0) {
		fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
		exit(4);
	}
	printf("L'adresse d'ecoute est: %s\n", serverAddr);
	printf("Le port d'ecoute est: %s\n", serverPort);
	// Definition de la taille du tampon contenant les demandes de connexion
	ecode = listen(descSockRDV, LISTENLEN); // Attendre connexion sur un socket
	if (ecode == -1) {
		perror("Erreur initialisation buffer d'écoute");
		exit(5);
	}

	len = sizeof(struct sockaddr_storage);
	// Attente connexion du client
	// Lorsque demande de connexion, creation d'une socket de communication avec le client
	descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len); // accepter la connexion 
	if (descSockCOM == -1){
		perror("Erreur accept\n");
		exit(6);
	}
	// Echange de données avec le client connecté
	strcpy(buffer, "220 Le proxy FTP est pret !\n");

	write(descSockCOM, buffer, strlen(buffer));

	//Lire user de client
	ecode = read(descSockCOM, buffer, MAXBUFFERLEN);
	if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
	//buffer[ecode] = '\0';
	printf("MESSAGE RECU DU CLIENT: \"%s\".\n",buffer);

	// USER anonymous@ftp.fr.debian.org
	// USER anonymous
	// ftp.fr.debian.org
	char login[50+1];  			// USER anonymous
	char servFTP[50+1];			// ftp.fr.debian.org
	bool isConnected = false; 
	memset(login, 0, sizeof(login));		// initialisation de la zone memoire de login a 0
	memset(servFTP, 0, sizeof(servFTP));	// initialisation de la zone memoire de servFTP a 0

	sscanf(buffer, "%50[^@]@%50s", login, servFTP); // Lecture formatee sur le buffer et affectation a login/servFTP
	sscanf(buffer, "\n"); 

	printf("login %s\n", login);
	printf("ServeurFTP %s\n", servFTP);

	// Initailisation de hints pour le serveur
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_family = AF_UNSPEC;      // les adresses IPv4 et IPv6 seront présentées par 
				                      // la fonction getaddrinfo

	//Récupération des informations sur le serveur
	ecode = getaddrinfo(servFTP,"21",&hints,&res);
	if (ecode){
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	resPtr = res;
    int servSock;
    // Tant que le proxy n'est pas connecte a un serveur et que nous n'avons pas
    // d'information de serveurs alors on essaie de se connecter
    // Parcours de la linked list contenue dans resPtr 
	while(!isConnected && resPtr!=NULL){

		//Création de la socket IPv4/TCP
		servSock = socket(resPtr->ai_family, resPtr->ai_socktype, resPtr->ai_protocol);
		if (servSock == -1) {
			perror("Erreur creation socket");
			exit(2);
		}

  		//Connexion au serveur
		ecode = connect(servSock, resPtr->ai_addr, resPtr->ai_addrlen);
		if (ecode == -1) { // Connexion non reussie
			resPtr = resPtr->ai_next; // On regarde la    		
			close(servSock);	
		} else {           // Connexion reussie
			isConnected = true;
			printf("Connexion réussie\n");
		}
	}
	// Liberation de la liste chainee de res
	freeaddrinfo(res);
	if (!isConnected){
		perror("Connexion impossible");
		exit(2);
	}
    memset(&buffer, 0, MAXBUFFERLEN);

    // Lecture sur le descripteur de socket du serveur
    ecode = read(servSock, buffer, MAXBUFFERLEN);
    if (ecode == -1) {perror("Erreur d'ecriture sur le serveur\n"); exit(3);}
    buffer[ecode] = '\0';
    printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer); // 220 Welcome
    memset(&buffer, 0, MAXBUFFERLEN);

    // write ne fonctionne pas avec login direct -> on passe par buffer
    // le '\n' ne passe pas en parametre de strncat : on utilise retourChariot[3]
    char retourChariot[3] = "\n";
    strncat(buffer,login,strlen(login));
    strncat(buffer,retourChariot,strlen(retourChariot));
    // ecriture sur le descripteur de socket 
    ecode = write(servSock, buffer, strlen(buffer));
    if (ecode == -1) {
        perror("Erreur d'ecriture sur le serveur\n");
        exit(3);
    }
    printf("MESSAGE ECRIT DU SERVEUR: \"%s\".\n",buffer); // USER anonymous
    memset(&buffer, 0, MAXBUFFERLEN);

    ecode = read(servSock, buffer, MAXBUFFERLEN);
    if (ecode == -1) {
        perror("Erreur de lecture sur le serveur\n");
        exit(3);
    }
    printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer); // Code 331 Specify password

    // Ecriture de la phrase au client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if (ecode == -1) {
        perror("Erreur d'ecriture sur le client\n");
        exit(3);
    }
    memset(&buffer, 0, MAXBUFFERLEN);

 	// Lecture de la reponse contenant le mdp
    ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1) {
        perror("Erreur de lecture sur le client\n");
        exit(3);
    }

    buffer[ecode] = '\0';
    printf("MESSAGE RECU DU CLIENT : \"%s\".\n",buffer); // PASS $iutinfo
    
    // Ecriture de la reponse sur le serveur
    ecode = write(servSock, buffer, strlen(buffer));
    if (ecode == -1) {
        perror("Erreur d'ecriture sur le serveur \n");
        exit(3);
    }
    memset(&buffer, 0, MAXBUFFERLEN);

    // Lecture sur le serveur : 230 si login successful
    ecode = read(servSock, buffer, MAXBUFFERLEN); 
    if (ecode == -1) {
        perror("Erreur de lecture sur le serveur \n");
        exit(3);
    }
    printf("MESSAGE RECU DU SERVEUR: \"%s\".\n", buffer); // Code 230

    // Ecriture de la reponse sur le descripteur de socket client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if (ecode == -1) {
        perror("Erreur d'ecriture sur le client\n");
        exit(3);
    }
    memset(&buffer, 0, MAXBUFFERLEN);

    // Lecture de la requete SYST 
    ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1) {
        perror("Erreur de lecture sur le client\n");
        exit(3);
    }
    printf("MESSAGE RECU DU CLIENT : \"%s\".\n",buffer); // SYST

    // Ecriture de SYST Sur le descripteur de socket serveur
    ecode = write(servSock, buffer, strlen(buffer)); 
    if (ecode == -1) {
        perror("Erreur d'ecriture sur le serveur \n");
        exit(3);
    }
    memset(&buffer, 0, MAXBUFFERLEN);

    // Lecture de la reponse du serveur
    ecode = read(servSock, buffer, MAXBUFFERLEN); 
    if (ecode == -1) {
        perror("Erreur de lecture sur le serveur \n");
        exit(3);
    }
    printf("MESSAGE RECU DU SERVEUR: \"%s\".\n", buffer); // Code 215 UNIX Type L8

	// Ecriture de la reponse du serveur sur le client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if (ecode == -1) {
        perror("Erreur d'ecriture sur le client\n");
        exit(3);
    }
    memset(&buffer, 0, MAXBUFFERLEN);

    // Lecture de PORT sur le client 
    ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1) {
        perror("Erreur de lecture sur le client\n");
        exit(3);
    }
    printf("MESSAGE RECU DU CLIENT : \"%s\".\n",buffer); 

    // Traduction du PORT en adresse IP pour la socket clientData et calcule le numero de port clientData
    // recuperation des 6 octets
    int n1,n2,n3,n4,n5,n6;
    sscanf(buffer,"PORT %d,%d,%d,%d,%d,%d",&n1,&n2,&n3,&n4,&n5,&n6);
    memset(&buffer, 0, MAXBUFFERLEN);
    char dataAdresseClient[25];
    char dataPortClient[10];
    // on met les 4 premiers octets a l'adresse et les 2 autres au port
    sprintf(dataAdresseClient,"%d.%d.%d.%d",n1,n2,n3,n4);
    sprintf(dataPortClient,"%d",n5*256+n6);
    printf("AdresseClient:%s\nPortClient:%s\n",dataAdresseClient,dataPortClient);
    //Ouverture clientData
    memset(&hintsClientData ,0, sizeof(hintsClientData));
    hintsClientData.ai_socktype = SOCK_STREAM;
	hintsClientData.ai_family = AF_INET;

  	ecode = getaddrinfo(dataAdresseClient, dataPortClient, &hintsClientData, &resClientData);
	if (ecode==-1) {
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	resClientDataPtr = resClientData;
	isConnected = false;
	// Tant que le proxy n'est pas connecte a un serveur et que nous n'avons pas
    // d'information de client data alors on essaie de se connecter
	while(!isConnected && resClientDataPtr!=NULL){

		//Création de la socket IPv4/TCP
		clientData = socket(resClientDataPtr->ai_family, resClientDataPtr->ai_socktype, resClientDataPtr->ai_protocol);
		if (clientData == -1) {
			perror("Erreur creation socket clientData");
			exit(2);
		}

  		//Connexion au serveur
		ecode = connect(clientData, resClientDataPtr->ai_addr, resClientDataPtr->ai_addrlen);
		if (ecode == -1) {
			resClientDataPtr = resClientDataPtr->ai_next;    		
			close(clientData);	
		} else {         // On a pu se connecter
			isConnected = true;
			printf("Connexion reussie\n");
		}
	}
	freeaddrinfo(resClientDataPtr);
	if (!isConnected) {
		perror("Connexion impossible");
		exit(2);
	}

	char PASSV[6]= "PASV\n";
	strncat(buffer,PASSV,strlen(PASSV));

	// Ecriture De PASV 
    ecode = write(servSock, buffer, strlen(buffer)); 
    if (ecode == -1) {
        perror("Erreur d'ecriture sur le serveur \n");
        exit(3);
    }
    memset(&buffer, 0, MAXBUFFERLEN);

    ecode = read(servSock, buffer, MAXBUFFERLEN); 
    if (ecode == -1) {
        perror("Erreur de lecture sur le serveur \n");
        exit(3);
    }
    printf("MESSAGE RECU DU SERVEUR: \"%s\".\n", buffer); // Code 227 Entering Passive Mode

    // OUVERTURE CONNEXION serveurData

    // Traduction du PORT en adresse IP pour la socket serveurData et calcule le numero de port serveurData
    // recuperation des 6 octets
	n1,n2,n3,n4,n5,n6=0;
	// on ignore la ( et on ne la stocke pas
    sscanf(buffer,"%*[^(](%d,%d,%d,%d,%d,%d",&n1,&n2,&n3,&n4,&n5,&n6);
    memset(&buffer, 0, MAXBUFFERLEN);

    char dataAdresseServeur[25];
    char dataPortServeur[10];
    // on met les 4 premiers octets a l'adresse et les 2 autres au port
    sprintf(dataAdresseServeur,"%d.%d.%d.%d",n1,n2,n3,n4);
    sprintf(dataPortServeur,"%d",n5*256+n6);
    printf("AdresseServeur:%s\nPortServeur:%s\n",dataAdresseServeur,dataPortServeur);
    //Ouvrir serveurData
    memset(&hintsServeurData ,0, sizeof(hintsServeurData));
    hintsServeurData.ai_socktype = SOCK_STREAM;
	hintsServeurData.ai_family = AF_INET;

	ecode = getaddrinfo(dataAdresseServeur, dataPortServeur, &hintsServeurData, &resServeurData);
	if (ecode==-1) {
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	resServeurDataPtr = resServeurData;
	isConnected = false;
	// Tant que le proxy n'est pas connecte a un serveur et que nous n'avons pas
    // d'informations de serveur data alors on essaie de se connecter
	while(!isConnected && resServeurDataPtr!=NULL){

		//Création de la socket IPv4/TCP
		serveurData = socket(resServeurDataPtr->ai_family, resServeurDataPtr->ai_socktype, resServeurDataPtr->ai_protocol);
		if (serveurData == -1) {
			perror("Erreur creation socket serveurData");
			exit(2);
		}

  		//Connexion au serveur
		ecode = connect(serveurData, resServeurDataPtr->ai_addr, resServeurDataPtr->ai_addrlen);
		if (ecode == -1) {
			resServeurDataPtr = resServeurDataPtr->ai_next;    		
			close(serveurData);	
		} else {         // On a pu se connecter
			isConnected = true;
			printf("Connexion reussie\n");
		}
	}
	freeaddrinfo(resServeurDataPtr);
	if (!isConnected) {
		perror("Connexion impossible");
		exit(2);
	}

	char port200[30]="200 PORT command successful\n";
	strncat(buffer,port200,strlen(port200));

	// Ecriture du code 200 sur le client
	ecode = write(descSockCOM, buffer, strlen(buffer)); 
	if(ecode == -1){
		perror("probleme d'ecriture sur sur le client\n");
		exit(3);
	}
	printf("MESSAGE ECRIT DU CLIENT :\"%s\".\n",buffer); // 200 port command successful
	memset(&buffer, 0, MAXBUFFERLEN);

	// Lecture de la commande LIST
    ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1) {
        perror("Erreur de lecture sur le client\n");
        exit(3);
    }
	printf("MESSAGE RECU DU CLIENT : \"%s\".\n",buffer); // LIST

	// Ecriture de LIST au serveur
	ecode = write(servSock, buffer, strlen(buffer));
	if(ecode == -1){
		perror("probleme d'ecriture sur sur le serveur\n");
		exit(3);
	}
	memset(&buffer, 0, MAXBUFFERLEN);

	// Reception reponse serveur (Code 150)
	ecode = read(servSock, buffer, MAXBUFFERLEN-1); 
	if(ecode == -1){
		perror("probleme de lecture sur le serveur\n");
		exit(3); 									
	}
	printf("MESSAGE RECU DU SERVEUR : \"%s\".\n",buffer); // Code 150 Here comes the directory listing

	// Ecriture du code 150 sur le client
	ecode = write(descSockCOM, buffer, strlen(buffer)); 
	if(ecode == -1){
		perror("probleme d'ecriture sur sur le client\n");
		exit(3);
	}
	memset(&buffer, 0, MAXBUFFERLEN);

	// Lecture sur la data du serveur
	// Debut algorithme de lecture de donnees
	ecode = read(serveurData, buffer, MAXBUFFERLEN-1);
    if (ecode == -1) {
        perror("Erreur de lecture sur la data du serveur\n");
        exit(3);
    }
	printf("MESSAGE RECU DU SERVEUR DATA: \"%s\".\n",buffer); // Code 150 Here comes the directory listing

	// Tant qu'il n'y a pas d'erreur de fonction write c'est a dire
	// Tant qu'il y a des donnees a lire du serveur data au vrai client
	// 0 = aucune ecriture
	while(ecode != 0) {
		// Traitement
		ecode = write(clientData,buffer,strlen(buffer));
		memset(&buffer, 0, MAXBUFFERLEN);
		// Lecture de la prochaine donnee
		ecode = read(serveurData,buffer,MAXBUFFERLEN);
		if (ecode == -1) {
			perror("Erreur de lecture sur le data serveur");
			exit(3);
		}
	}	
	memset(&buffer, 0, MAXBUFFERLEN);

	// Fermeture socket de data client et serveur
	close(serveurData);
	close(clientData);

	// Lecture sur le serveur (fin de lecture sur le serveurData 226)
	ecode = read(servSock,buffer,MAXBUFFERLEN);
	if (ecode == -1) {
		perror("Erreur de lecture sur le serveur");
		exit(3);
	}
	printf("MESSAGE RECU DU SERVEUR : \"%s\".\n", buffer); // Code 226 Directory send OK

	// Ecriture du code 226 au client
	ecode = write(descSockCOM, buffer, strlen(buffer));
	if (ecode == -1) {
		perror("Erreur d'ecriture sur le client");
		exit(3);
	}
	memset(&buffer, 0, MAXBUFFERLEN);

    // Lecture de la commande QUIT par le client
    ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1) {
        perror("Erreur de lecture sur le client\n"); 
        exit(3);
    }
    printf("MESSAGE RECU DU CLIENT: \"%s\".\n", buffer); // QUIT

    // Ecriture de la commande 221 au serveur
    ecode = write(servSock, buffer, strlen(buffer)); 
    if (ecode == -1) {
        perror("Erreur d'ecriture sur le serveur \n");
        exit(3);
    }
    memset(&buffer, 0, MAXBUFFERLEN);

    // Lecture de la reponse du serveur (Code 221)
    ecode = read(servSock, buffer, MAXBUFFERLEN); 
    if (ecode == -1) {
        perror("Erreur de lecture sur le serveur \n");
        exit(3);
    }
    printf("MESSAGE RECU DU SERVEUR: \"%s\".\n", buffer); // Code 221 Goodbye


	//Fermeture de la connexion
	close(descSockCOM);
	close(descSockRDV);
	close(servSock);

	exit(0);
}