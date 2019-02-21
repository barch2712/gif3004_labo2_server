#include "actions.h"

int verifierNouvelleConnexion(struct requete reqList[], int maxlen, int socket){
    // Dans cette fonction, vous devez d'abord vérifier si le serveur peut traiter
    // une nouvelle connexions (autrement dit, si le nombre de connexions en cours
    // ne dépasse pas MAX_CONNEXIONS). Utilisez nouvelleRequete() pour cela.
    //
    // Si une nouvelle connexion peut être traitée, alors vous devez utiliser accept()
    // pour vérifier si un nouveau client s'est connecté. Si c'est le cas, vous devez modifier
    // la nouvelle entrée de reqList pour y sauvegarder le descripteur de fichier correspondant
    // à cette nouvelle connexion, et changer son statut à REQ_STATUS_LISTEN
    // Voyez man accept(2) pour plus de détails sur cette fonction
    //
    // Cette fonction doit retourner 0 si elle n'a pas acceptée de nouvelle connexion, ou 1 dans le cas contraire.

    // TODO
    int req = nouvelleRequete(reqList, maxlen);
    if (req >= 0) {
        int desc = accept(socket, NULL, NULL);
        if (desc >= 0) {
            reqList[req].fdSocket = desc;
            reqList[req].status = REQ_STATUS_LISTEN;
            return 1;
        }
    }
    return 0;
}

int traiterConnexions(struct requete reqList[], int maxlen){
    // Cette fonction est partiellement implémentée pour vous
    // Elle utilise select() pour déterminer si une connexion cliente vient d'envoyer
    // une requête (et s'il faut donc la lire).
    // Si c'est le cas, elle lit la requête et la stocke dans un buffer.
    //
    // Par la suite, VOUS devez implémenter le code créant un nouveau processus et
    // un nouveau pipe de communication, partagé avec ce processus enfant.
    // Finalement, vous devez mettre à jour la structure de données de la requête touchée.
    // Cette fonction doit retourner 0 si elle n'a lu aucune donnée supplémentaire, ou un nombre > 0 si c'est le cas.

    int octetsTraites;

    // On parcourt la liste des connexions en cours
    // On utilise select() pour determiner si des descripteurs de fichier sont disponibles
    fd_set setSockets;
    struct timeval tvS;
    tvS.tv_sec = 0; tvS.tv_usec = SLEEP_TIME;
    int nfdsSockets = 0;
    FD_ZERO(&setSockets);

    for(int i = 0; i < maxlen; i++){
        if(reqList[i].status == REQ_STATUS_LISTEN){
            FD_SET(reqList[i].fdSocket, &setSockets);
            nfdsSockets = (nfdsSockets <= reqList[i].fdSocket) ? reqList[i].fdSocket+1 : nfdsSockets;
        }
    }

    if(nfdsSockets > 0){
        // Au moins un socket est en attente d'une requête
        int s = select(nfdsSockets, &setSockets, NULL, NULL, &tvS);
        if(s > 0){
            // Au moins un socket est prêt à être lu
            for(int i = 0; i < maxlen; i++){
                if(reqList[i].status == REQ_STATUS_LISTEN && FD_ISSET(reqList[i].fdSocket, &setSockets)){
                    struct msgReq req;
                    char* buffer = malloc(sizeof(req));

                    // On lit les donnees sur le socket
                    if(VERBOSE)
                        printf("Lecture de la requete sur le socket %i\n", reqList[i].fdSocket);
                    octetsTraites = read(reqList[i].fdSocket, buffer, sizeof(req));
                    if(octetsTraites == -1){
                        perror("Erreur en effectuant un read() sur un socket pret");
                        exit(1);
                    }

                    memcpy(&req, buffer, sizeof(req));
                    buffer = realloc(buffer, sizeof(req) + req.sizePayload);
                    octetsTraites = read(reqList[i].fdSocket, buffer + sizeof(req), req.sizePayload);
                    if(VERBOSE){
                        printf("\t%i octets lus au total\n", req.sizePayload + sizeof(req));
                        printf("\tContenu de la requete : %s\n", buffer + sizeof(req));
                    }

                    // Ici, vous devez tout d'abord initialiser un nouveau pipe à l'aide de la fonction pipe()
                    // Voyez man pipe pour plus d'informations sur son fonctionnement
                    // TODO

                    malloc(sizeof(req.sizePayload));
                    memcpy(reqList[i].filename, buffer + sizeof(req), req.sizePayload);

                    int fd[2];
                    pipe(fd);

                    // Une fois le pipe initialisé, vous devez effectuer un fork, à l'aide de la fonction du même nom
                    // Cela divisera votre processus en deux nouveaux processus, un parent et un enfant.
                    // - Dans le processus enfant, vous devez appeler la fonction executeRequete() en lui donnant
                    //      l'extrémité d'écriture du pipe et le buffer contenant la requête. Lorsque cette fonction
                    //      retourne, vous pouvez assumer que le téléchargement est terminé et quitter le processus.
                    // - Dans le processus parent, vous devez enregistrer le PID (id du processus) de l'enfant ainsi que
                    //      le descripteur de fichier de l'extrémité de lecture du pip dans la structure de la requête.
                    //      Vous devez également passer son statut à REQ_STATUS_INPROGRESS.
                    //
                    // Pour plus d'informations sur la fonction fork() et sur la manière de détecter si vous êtes dans
                    // le parent ou dans l'enfant, voyez man fork(2).
                    // TODO
                    pid_t pid = fork();
                    if (pid > 0) {
                        close(fd[1]);
                        reqList[i].fdPipe = fd[0];
                        reqList[i].pid = pid;
                        reqList[i].status = REQ_STATUS_INPROGRESS;
                    } else {
                        close(fd[0]);
                        executeRequete(fd[1], buffer);
                        close(fd[1]);
                        exit(EXIT_SUCCESS);
                    }

                }
            }
        }
    }

    return nfdsSockets;
}


int traiterTelechargements(struct requete reqList[], int maxlen){
    // Cette fonction détermine si des processus enfants (s'il y en a) ont écrit quelque chose sur leur pipe.
    // Si c'est le cas, elle le lit et le stocke dans le buffer lié à la requête.
    //
    // Il vous est conseillé de vous inspirer de la fonction traiterConnexions(), puisque la procédure y est
    // très similaire. En détails, vous devez :
    // 1) Faire la liste des descripteurs qui correspondent à des pipes ouverts
    // 2) Utiliser select() pour déterminer si un de ceux-ci peut être lu
    // 3) Si c'est le cas, vous devez lire son contenu. Rappelez-vous (voir les commentaires dans telecharger.h) que
    //      le processus enfant écrit d'abord la taille du contenu téléchargé, puis le contenu téléchargé lui-même.
    //      Cela vous permet de savoir combien d'octets vous devez récupérer au total. Attention : plusieurs lectures
    //      successives peuvent être nécessaires pour récupérer tout le contenu du pipe!
    // 4) Une fois que vous avez récupéré son contenu, modifiez le champ len de la structure de la requête pour
    //      refléter la taille du fichier, ainsi que buffer pour y écrire un pointeur vers les données. Modifiez
    //      également le statut à REQ_STATUS_READYTOSEND.
    // 5) Finalement, terminer les opérations avec le processus enfant le rejoignant en attendant sa terminaison
    //      (vous aurez besoin de la fonction waitpid()), puis fermer le descripteur correspondant l'extrémité
    //      du pipe possédée par le parent.
    //
    // S'il n'y a aucun processus enfant lancé, ou qu'aucun processus n'a écrit de données, cette fonction
    // peut retourner sans aucun traitement.
    // Cette fonction doit retourner 0 si elle n'a lu aucune donnée supplémentaire, ou un nombre > 0 si c'est le cas.

    // TODO
    fd_set setPipes;
    int nfdsPipes = 0;
    struct timeval tvS;
    tvS.tv_sec = 0; tvS.tv_usec = SLEEP_TIME;
    FD_ZERO(&setPipes);

    for(int i = 0; i < maxlen; i++){
        if(reqList[i].status == REQ_STATUS_INPROGRESS){
            FD_SET(reqList[i].fdPipe, &setPipes);
            nfdsPipes = (nfdsPipes <= reqList[i].fdPipe) ? reqList[i].fdPipe+1 : nfdsPipes;
        }
    }

    if(nfdsPipes > 0){
        int s = select(nfdsPipes, &setPipes, NULL, NULL, &tvS);
        if(s > 0){
            for(int i = 0; i < maxlen; i++) {
                if(reqList[i].status == REQ_STATUS_INPROGRESS && FD_ISSET(reqList[i].fdPipe, &setPipes)){
                    size_t read_size;
                    read(reqList[i].fdPipe, &read_size, sizeof(size_t));
                    reqList[i].buf = malloc(read_size);

                    size_t processed_size = 0;
                    while(processed_size < read_size) {
                        size_t n = read(reqList[i].fdPipe, reqList[i].buf + processed_size, read_size - processed_size);
                        processed_size += n;
                    }

                    reqList[i].len = read_size;
                    reqList[i].status = REQ_STATUS_READYTOSEND;

                    waitpid(reqList[i].pid, NULL, 0);
                    close(reqList[i].fdPipe);
                }
            }
        }
    }
    return nfdsPipes;
}
