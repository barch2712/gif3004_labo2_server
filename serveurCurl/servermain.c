
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Pour recuperer les descriptions d'erreur
#include <errno.h>

// Multiprocessing
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

// Sockets UNIX
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

// Signaux UNIX
#include <signal.h>


// Structures et fonctions de communication
#include "communications.h"
// Fonction de téléchargement utilisant cURL
#include "telechargeur.h"
// Structures et fonctions stockant et traitant les requêtes en cours
#include "requete.h"
// Fonctions de la boucle principale
#include "actions.h"

// Chaînes de caractères représentant chaque statut (utile pour l'affichage)
const char* statusDesc[] = {"Inoccupe", "Connexion client etablie", "En cours de telechargement", "Pret a envoyer"};

// Nombre maximal de connexions simultanés
#define MAX_CONNEXIONS 10
// Contient les requetes en cours de traitement
struct requete reqList[MAX_CONNEXIONS];


void gereSignal(int signo) {
    // Fonction affichant des statistiques sur les tâches en cours
    // lorsque SIGUSR2 (et _seulement_ SIGUSR2) est reçu
    // TODO
    if (signo == SIGUSR2) {
        int nbconnected = 0;
		for(int i = 0; i < MAX_CONNEXIONS; i++)
		{
			if(reqList[i].status != REQ_STATUS_INACTIVE)
			{
				nbconnected++;
			}
		}
		printf("Le nombre de connexions actives: %u\n\n", nbconnected);

		printf("Les fichiers en cours de téléchargement:\n");
		for(int i = 0; i < MAX_CONNEXIONS; i++)
		{
			if(reqList[i].status == REQ_STATUS_INPROGRESS)
			{
				printf("Nom du fichier: %s\n", reqList[i].filename);
				printf("Identifiant du processus de telechargement: %u\n\n", reqList[i].pid);
			}
}
    }
}



int main(int argc, char* argv[]){
    // Chemin du socket UNIX
    // Linux ne supporte pas un chemin de plus de 108 octets (voir man 7 unix)
    char path[108] = "/tmp/unixsocket";
    if(argc > 1)        // On peut également le passer en paramètre
        strncpy(path, argv[1], sizeof(path));
    unlink(path);       // Au cas ou le fichier liant le socket UNIX existerait deja

    // On initialise la liste des requêtes
    memset(&reqList, 0, sizeof(reqList));

    // TODO
    // Implémentez ici le code permettant d'attacher la fonction "gereSignal" au signal SIGUSR2
    struct sigaction action;
    memset(&action, '\0', sizeof(action));
    action.sa_handler = &gereSignal;
    if (sigaction(SIGUSR2, &action, NULL) < 0) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    // TODO
    // Création et initialisation du socket (il y a 5 étapes)
    // 1) Créez une struct de type sockaddr_un et initialisez-la à 0.
    //      Puis, désignez le socket comme étant de type AF_UNIX
    //      Finalement, copiez le chemin vers le socket UNIX dans le bon attribut de la structure
    //      Voyez man unix(7) pour plus de détails sur cette structure
    struct sockaddr_un sock_addr;
    sock_addr.sun_family = AF_UNIX;
    strncpy(sock_addr.sun_path, path, sizeof(path)-1);


    // TODO
    // 2) Créez le socket en utilisant la fonction socket().
    //      Vérifiez si sa création a été effectuée avec succès, sinon quittez le processus en affichant l'erreur
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("connection socket");
        exit(EXIT_FAILURE);
    }

    // TODO
    // 3) Utilisez fcntl() pour mettre le socket en mode non-bloquant
    //      Vérifiez si l'opération a été effectuée avec succès, sinon quittez le processus en affichant l'erreur
    //      Voyez man fcntl pour plus de détails sur le champ à modifier
    int flag = fcntl(sock, F_GETFL);
    if (flag == -1) {
        perror("Erreur sur get socket flags");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sock, F_SETFL, flag | O_NONBLOCK) == -1) {
        perror("Erreur sur mise en mode non-bloquant du socket");
        exit(EXIT_FAILURE);
    }

    // TODO
    // 4) Faites un bind sur le socket
    //      Vérifiez si l'opération a été effectuée avec succès, sinon quittez le processus en affichant l'erreur
    //      Voyez man bind(2) pour plus de détails sur cette opération
    if (bind(sock, (const struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
        perror("Erreur sur bind du socket");
        exit(EXIT_FAILURE);
    }

    // TODO
    // 5) Mettez le socket en mode écoute (listen), en acceptant un maximum de MAX_CONNEXIONS en attente
    //      Vérifiez si l'opération a été effectuée avec succès, sinon quittez le processus en affichant l'erreur
    //      Voyez man listen pour plus de détails sur cette opération
    if (listen(sock, MAX_CONNEXIONS) == -1) {
        perror("Erreur sur la mise en mode ecoute du socket");
        exit(EXIT_FAILURE);
    }


    // Initialisation du socket UNIX terminée!

    // Boucle principale du programme
    int tacheRealisee;
    while(1){
        // On vérifie si de nouveaux clients attendent pour se connecter
        tacheRealisee = verifierNouvelleConnexion(reqList, MAX_CONNEXIONS, sock);

        // On teste si un client vient de nous envoyer une requête
        // Si oui, on la traite
        tacheRealisee += traiterConnexions(reqList, MAX_CONNEXIONS);

        // On teste si un de nos processus enfants a terminé son téléchargement
        // Dans ce cas, on traite le résultat
        tacheRealisee += traiterTelechargements(reqList, MAX_CONNEXIONS);

        // Si on a des données à envoyer au client, on le fait
        tacheRealisee += envoyerReponses(reqList, MAX_CONNEXIONS);

        // Si on n'a pas attendu dans un select ou effectué une tâche, on ajoute
        // un petit delai ici pour éviter d'utiliser 100% du CPU inutilement
        if(tacheRealisee == 0)
            usleep(SLEEP_TIME);
    }

    return 0;
}
