#include "server.h"

// funzione che inizializza la socket
int initialization_socket(){
    int server_socket; // descrittore della socket
    // Creazione di una nuova socket
    server_socket = socket(AF_INET,SOCK_STREAM,0);
    if( server_socket <= 0){
        perror("error creation socket");
        return -1;
    }
    // Bind Socket con indirizzo inserito nella costante globale
    struct sockaddr_in addr_sock;
    memset(&addr_sock, 0, sizeof(addr_sock));
    addr_sock.sin_family = AF_INET;
    addr_sock.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_sock.sin_port = htons(PORTA_SERVER);
    if (bind(server_socket, (struct sockaddr*)&addr_sock, sizeof(addr_sock)) < 0) { 
        perror("Errore nel bind");
        close(server_socket);
        return -1;
    }
    // Spostamento della socket nello stato di ascolto per nuove connesioni
    if (listen(server_socket, MAX_ONLINE) < 0) { // entro nella modalità listen con una coda di massimo 5 client gli altri verranno scartati
        perror("Errore nell'ascolto");
        close(server_socket);
        return -1;
    }
    return server_socket;
}

//funzione che inizializza la libreria di criptazione
int initialization_cryptolibrary(){
    if (sodium_init() < 0) {
        perror("problemi inizializzazione libreria libsodium");
        return 1;
    }
    return 0;
}

// funzione che prova ad aprire il file contenente i messaggi dell'utente e se riesci li inserisce all'interno della struttura dati
int initializationMessaggi(Utente* utente){
    //apriamo il file in modalità lettura
    FILE* file;
    if((file = fopen(utente->nome_file,"r"))!=NULL){
        //il file esiste e quindi va letto per inserire i messaggi
        Messaggio newMessaggio;
        while(fscanf(file,"%[^\n]\n%[^\n]\n%[^\n]\n",newMessaggio.mittente,newMessaggio.ogetto,newMessaggio.text)>0){
            if(insertMessaggio(utente,newMessaggio) != 0){
                printf("errore nell'inserire il messaggo\n");
                return 1;
            }
        }
    }
    return 0;
}

// inserisce un nuovo messaggio all'interno della lista di un determinato utente
int insertMessaggio(Utente* utente, Messaggio messaggio){
    //creazione di un nuovo messaggio
    Messaggio* NewMessaggio = (Messaggio*)malloc(sizeof(Messaggio));
    if(NewMessaggio == NULL){
        perror("Errore: impossibile allocare memoria per il nuovo messaggio");
        return 1;
    }
    //inizializziamo il nuovo nodo
    strcpy(NewMessaggio->mittente,messaggio.mittente);
    strcpy(NewMessaggio->ogetto,messaggio.ogetto);
    strcpy(NewMessaggio->text,messaggio.text);
    NewMessaggio->next = NULL;
    //blocca mutex per la lista messggi
    if(pthread_mutex_lock(&(utente->mutexMess)) != 0){
        printf("errore nel bloccare il mutex per i messaggi");
        return 1;
    }
    // Collega il nuovo messaggio alla lista
    if(utente->startMessaggi == NULL){
        utente->startMessaggi = NewMessaggio;
        utente->endMessaggi = NewMessaggio;
    }else{
        utente->endMessaggi->next = NewMessaggio;
        utente->endMessaggi = NewMessaggio;
    }
    utente->numMessaggi ++;
    //sblocca mutex per messaggi
    if(pthread_mutex_unlock(&(utente->mutexMess)) != 0){
        printf("errore nel sbloccare il mutex per i messaggi");
        return 1;
    }
    return 0;
}

// la funzione inserisce un nuovo nodo utente in testa alla lista e controlla se ci sono messaggi archiviati
int insertUtente(Utente** testa,Utente utente){
    //blocchiamo la scrittura sulla lista utenti
    if(pthread_mutex_lock(&mutexListaUtenti)!=0){
        printf("impossibile sbloccare il mutex \n");
        exit(EXIT_FAILURE);
    }
    // Creazione del nuovo nodo Utente
    Utente* NewUtente = (Utente*)malloc(sizeof(Utente));
    if (NewUtente == NULL) {
        perror("Errore: impossibile allocare memoria per il nuovo utente");
        return 1;
    }
    //creiamo la stringa per il nome del file
    char* nome_file;
    if((nome_file = (char*)malloc(sizeof(char)*(MAX_ID+4))) == NULL){
    perror("errore nella allocazione di memoria per la stringa username");
    return 1;
    }
    strcpy(nome_file,utente.username);
    strcat(nome_file, ".txt");
    //inizializziamo il nuovo nodo
    strcpy(NewUtente->username,utente.username);
    strcpy(NewUtente->password,utente.password);
    strcpy(NewUtente->nome_file,nome_file);
    pthread_mutex_init(&(NewUtente->mutexMess), NULL);
    NewUtente->startMessaggi = NULL;
    NewUtente->endMessaggi = NULL;
    NewUtente->numMessaggi = 0;
    // inizializzazione dei messaggi
    initializationMessaggi(NewUtente);
    // Collega il nuovo utente alla testa della lista
    NewUtente->next = *testa;
    *testa = NewUtente;
    //sblocchiamo la scrittura sulla lista utenti
    if(pthread_mutex_unlock(&mutexListaUtenti)!=0){
        printf("impossibile sbloccare il mutex \n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

//funzione che cerca un determinato utente all'interno della lista e se lo trova lo restituisce
Utente* searchUtente(Utente* lista,char* nome){
    Utente* nodoCorrente = lista;

    while (nodoCorrente != NULL) {
        if(strcmp(nome,nodoCorrente->username)==0){
            return nodoCorrente;
        }
        nodoCorrente = nodoCorrente->next;
    }

    return NULL;
}

// Funzione per eliminare un nodo dalla lista
int deleteUtente(Utente** testa, char * nome) {
    // Puntatori temporanei per attraversare la lista
    Utente* temp = *testa;
    Utente* prev = NULL;
    // Cerca il nodo da eliminare
    while (temp != NULL && strcmp(temp->username,nome) != 0) {
        prev = temp;
        temp = temp->next;
    }
    // Se il nodo non è presente nella lista
    if (temp == NULL) {
        return 1;
    }
    //blocca la scrittura sulla lista utenti
    if(pthread_mutex_lock(&mutexListaUtenti)!=0){
        printf("impossibile bloccare il mutex \n");
        exit(EXIT_FAILURE);
    }
    // blocca la scrittura sulla lista messaggi
    if(pthread_mutex_lock(&(temp->mutexMess))!=0){
        printf("impossibile bloccare il mutex \n");
        exit(EXIT_FAILURE);
    }
    // Collega il nodo precedente al successivo, ignorando il nodo corrente
    if (prev == NULL) {
        // Se il nodo da eliminare è il primo nodo
        *testa = temp->next;
    } else {
        prev->next = temp->next;
    }
    // Libera la memoria del nodo eliminato
    Messaggio* mess= temp->startMessaggi;
    Messaggio* att;
    while(mess!= NULL){
        att = mess;
        mess= att->next;
        free(att);
    }
    //eliminiamo il file contenente i messaggi
    FILE* file;
    //proviamo ad aprire il file
    if((file = fopen(temp->nome_file,"r")) != NULL){
        //chiudiamo il file
        fclose(file);
        //eliminiamo il file
        if(remove(temp->nome_file)!=0){
            printf("Errore impossibile eliminare il file\n");
            return 1;
        }
    }
    free(temp);
    //sblocchiamo il mutex per la scrittura di utenti nella lista
    if(pthread_mutex_unlock(&mutexListaUtenti)!=0){
        printf("impossibile sbloccare il mutex \n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// la funzione serve per inizializzare la struttura dati contenenti utenti registarti e messaggi in arrivo associati
int initialization_database(){
    listaUtenti = NULL;// inizializiamo la lista utenti come vuota
    //inizializziamo il mutex che sblocca la scrittura nella lista utenti
    if(pthread_mutex_init(&mutexListaUtenti,NULL) != 0){
        perror("errore inizializzazione mutex lista utenti");
        return 1;
    }
    //se esiste apriamo il file con all'interno le credenziali degli utenti registrati
    FILE* fcred = fopen(FILE_CREDENTIAL,"r");
    if(fcred != NULL){
        // ciclo che per ogni utente letto nel file credenziali controlla se esiste il file relativo ai messaggi in arrivo
        Utente utente;
        while(fscanf(fcred,"%s %s\n",utente.username,utente.password) > 0){
            //funzione che inserisce l'utente nella lista degli utenti attualmente regiustrati nell'applicazione
            if(insertUtente(&listaUtenti,utente) != 0){
                printf("errore nell'inserimento dell'utente nel database \n");
                fclose(fcred);
                return 1;
            }
        }
        //chiudiamo il file credenziali
        fclose(fcred);
    }
    return 0;
}

//la funzione serve per scambiare le chiavi crittografiche fra il client e il thread associato
int key_exchange(unsigned char* server_pk, unsigned char* server_sk, unsigned char* server_rx, unsigned char* server_tx, unsigned char* client_pk, int socket){
    // Genera le chiavi del server privata e pubblica
    crypto_kx_keypair(server_pk, server_sk);
    // Invia la chiave pubblica del server al client
    if(send_data(server_pk,crypto_kx_PUBLICKEYBYTES,socket) != 0){
        printf("errore nell'invio del server_pk \n");
        return 1;
    }
    // Riceve la chiave pubblica del client al server
    if(receive_data(client_pk,crypto_kx_PUBLICKEYBYTES,socket) != 0){
        printf("errore nella ricezione del client_pk \n");
        return 1;
    }
    // Calcola una coppia di chiavi per criptazione e decriptazione dei dati
    if (crypto_kx_server_session_keys(server_rx, server_tx, server_pk, server_sk, client_pk) != 0) {
        printf("Errore nel creare la coppia di chiavi per ricezione e invio \n");
        return 1;
    }
    return 0;
}

// La funzione gestisce le autenticazioni dei client (registrandoli o controllando le credenziali)
int authentication_client(char* user, int16_t op, unsigned char* server_rx,unsigned char* server_tx,int socket){
    Utente* utente; //puntatore ad un nodo utente
    // ricezione dell'username
    char username[MAX_ID]; //puntatore al buffer contenente l'username inviato dall'utente
    int16_t instr_server = -1; // risposta del server al client (-1 stato default)
    while( instr_server != 0){
        // riceve la stringa username scritta dall'utente
        if(receive_encrypted_data(socket,(unsigned char*)username,MAX_ID,server_rx) != 0){
            printf("errore ricezione username dal client \n");
            return 1;
        }
        //cerchiamo all'interno della lista se esiste un utente con quell'username
        if((utente = searchUtente(listaUtenti,username)) != NULL){
            if(op != 1){
                //l'utente vuole loggarsi
                instr_server = 0; // confermiamo l'esistenza dell'utente
                strcpy(user,username); //inserisco l'username all'interno del buffer che identifica l'utente nel thread
            } else {
                //l'utente vuole registrarsi
                instr_server = 1; //username gia esistente
            }
        } else {
            if(op == 1){
                //l'utente vuole registrarsi
                instr_server = 0; // confermiamo username valido
                strcpy(user,username); //inserisco l'username all'interno del buffer che identifica l'utente nel thread
                //allochiamo memoria per un nuovo utente
                utente = (Utente*)malloc(sizeof(Utente));
                if(utente == NULL){
                    perror("errore nella malloc per la creazione di un nuovo utente");
                    return 1;
                }
                strcpy(utente->username,username);
            } else{
                //l'utente vuole loggarsi
                instr_server = 2; // username insesistente
            }
        }
        // invio risultato della ricerca al client
        send_encrypted_int(socket,instr_server,server_tx);
    }
    //ricezione della password
    char password[MAX_PSWD]; // stringa temporanea per la password inviata dal cliente
    char hashed_password[ENC_PSWD]; // stringa che contiene la password cifrata
    instr_server = -1; // riportiamo l' "instruzione lato server" allo stato di default
    // Blocca la memoria riservata alla password in chiaro
    if (sodium_mlock(password, sizeof(password)) != 0) {
        fprintf(stderr, "Impossibile bloccare la memoria riservata alla password\n");
        return 1;
    }
    while(instr_server != 0){
        // riceve la password in chiaro inviata dall'utente
        if(receive_encrypted_data(socket,(unsigned char*)password,MAX_PSWD,server_rx)!=0){
            printf("errore nella ricezione della password in chiaro dal server \n");\
            return 1;
        }
        // Cifra la password in chiaro
        if (crypto_pwhash_str(hashed_password, password, strlen(password), crypto_pwhash_OPSLIMIT_SENSITIVE, crypto_pwhash_MEMLIMIT_SENSITIVE) != 0) {
            perror("error to hash password ");
            return 1;
        }
        if(op == 1){
            //l'utente vuole registrarsi
            strcpy(utente->password,hashed_password);
            instr_server = 0;
            if(insertUtente(&listaUtenti,*utente) != 0){
                printf("errore nell'aggiunta del nuovo utente");
                return 1;
            }
        } else {
            //l'utente vuole loggarsi o eliminare l'account
            if ( crypto_pwhash_str_verify(utente->password, password, strlen(password)) != 0) {
                // Password sbagliata
                instr_server = 1; // messaggio al client "password errata" 
            } else {
                instr_server = 0; //messaggio al client "password corretta"
            } 
        }
        //sblocco la memoria riservata alla password in chiaro
        sodium_munlock(password, sizeof(password));
        // Invia risultato al client
        send_encrypted_int(socket,instr_server,server_tx);
    }
    return 0;
}

//la funzione serve per inserire un nuovo messaggio all'interno della lista di un'altro utente
int writeMessage(char* user, unsigned char* server_rx, unsigned char* server_tx, int socket){
    Utente* destinatario; //puntatore al nodo del destinatario
    Messaggio nuovoMessaggio; //nodo messaggio
    strcpy(nuovoMessaggio.mittente,user); //inseriamo il mittente (cioè l'user che ha chiamato la funzione)
    // ricezione dell'username
    char* username; //puntatore al buffer contenente l'username inviato dall'utente
    int16_t instr_server = -1; // risposta del server al client (-1 stato default)
    while( instr_server != 0){
        //allocazioen di memoria dinamica per l'username
        username = (char*)malloc(sizeof(char)*MAX_ID);
        if(username == NULL){
            perror("errore malloc per allocazioen username");
            return 1;
        }
        // riceve la stringa username scritta dall'utente
        if(receive_encrypted_data(socket,(unsigned char*)username,MAX_ID,server_rx) != 0){
            printf("errore ricezione username dal client \n");
            return 1;
        }
        if(strcmp(username,user)==0){
            instr_server = 1; // impossibile inviare un messaggioa se stessi
        } else {
            if((destinatario = searchUtente(listaUtenti,username)) != NULL){
                instr_server = 0; // confermiamo l'esistenza dell'utente
            } else {
                instr_server =2; // utente insesitente
            }
        }
        // invio risultato della ricerca al client
        if(send_encrypted_int(socket,instr_server,server_tx) != 0){
            printf("errore nell'invio dell'intero \n");
            return 1;
        }
        free(username);
    }
    //ricezione ogetto
    if(receive_encrypted_data(socket,(unsigned char*)nuovoMessaggio.ogetto,MAX_OBJECT,server_rx) != 0){
        printf("errore ricezione ogetto dal client \n");
        return 1;
    }
    // invio conferma ricezione al client
    send_encrypted_int(socket,instr_server,server_tx);
    // ricezione testo    
    if(receive_encrypted_data(socket,(unsigned char*)nuovoMessaggio.text,MAX_TEXT,server_rx) != 0){
        printf("errore ricezione testo dal client \n");
        return 1;
    }
    // invio messaggio al destinatario
    if(insertMessaggio(destinatario,nuovoMessaggio) != 0){
        printf("errore nell'invio del messaggio \n");
        return 1;
    }
    // invio conferma successo al client
    if(send_encrypted_int(socket,instr_server,server_tx) != 0){
        printf("errore nell'invio dell'intero \n");
        return 1;
    }
    return 0;
}

//la funzione legge tutta la lista dei messaggi dell'utente e la invia al client
int sendAllMessage(char* user, unsigned char* server_rx,unsigned char* server_tx,int socket){
    Utente* utente; // nodo utente all'interno della lista
    int16_t numeroMessaggi; // numero di messaggi da inviare al client
    int16_t validate; //risposta del client alla ricezione dei messaggi
    //cerchiamo l'utente all'interno della lista
    if((utente = searchUtente(listaUtenti,user)) == NULL){
        printf("utente non trovato nella lista \n");
        return 1;
    }
    //prendiamo il numero dei messaggi a disposizione
    numeroMessaggi = utente->numMessaggi;
    //inviamo il numero dei messaggi correntemente salvati
    if(send_encrypted_int(socket,numeroMessaggi,server_tx) != 0){
        printf("errore nell'invio del numero dei messaggi \n");
        return 1;
    }
    //aspettiamo ricezione conferma
    if(receive_encrypted_int(&validate,socket,server_rx)!=0){
        printf("errore nella ricezione della conferma \n");
        return 1;
    }
    //scorriamo interamente la lista dei messaggi per inviarli al client
    Messaggio* attuale = utente->startMessaggi;
    for(int i=0;i<numeroMessaggi;i++){
        //mandiamo il mittente
        if(send_encrypted_data(socket,attuale->mittente,MAX_ID,server_tx) != 0){
            printf("errore nell'invio del mittente \n");
            return 1;
        }
        //aspettiamo ricezione conferma
        if(receive_encrypted_int(&validate,socket,server_rx)!=0){
            printf("errore nella ricezione della conferma \n");
            return 1;
        }
        //mandiamo ogetto
        if(send_encrypted_data(socket,attuale->ogetto,MAX_OBJECT,server_tx) != 0){
            printf("errore nell'invio dell'ogetto \n");
            return 1;
        }
        //aspettiamo ricezione conferma
        if(receive_encrypted_int(&validate,socket,server_rx)!=0){
            printf("errore nella ricezione della conferma \n");
            return 1;
        }
        //mandiamo il testo
        if(send_encrypted_data(socket,attuale->text,MAX_TEXT,server_tx) != 0){
            printf("errore nell'invio del testo \n");
            return 1;
        }
        //aspettiamo ricezione conferma
        if(receive_encrypted_int(&validate,socket,server_rx)!=0){
            printf("errore nella ricezione della conferma \n");
            return 1;
        }
        //spostiamo il puntatore
        attuale = attuale->next;
    }
    return 0;
}

//La funzione cerca all'interno della lista dei messaggi dell'utente il messggio selezionato e lo elimina
int deleteMessage(char* user, unsigned char* server_rx,unsigned char* server_tx,int socket){
    Utente* utente;
    if((utente = searchUtente(listaUtenti,user)) == NULL){
        printf("impossibile trovare l''utente \n");
        return 1;
    }
    Messaggio* attuale = utente->startMessaggi;
    Messaggio* prev = NULL;
    if(utente->numMessaggi == 0){
        return 0;
    }
    int16_t nMess;
    if(receive_encrypted_int(&nMess,socket,server_rx)!=0){
        printf("errore nel riceve il numero del messaggio da eliminare \n");
        return 1;
    }
    for(int16_t i=0;i<nMess;i++){
        prev = attuale;
        attuale = prev->next;
    }
    //blocchiamo la scrittura di nuovi messaggi
    if(pthread_mutex_lock(&(utente->mutexMess))!=0){
        printf("errore nello bloccare il mutex\n");
        return 1;
    }

    if(attuale == utente->startMessaggi){
        utente->startMessaggi = attuale->next;
    }
    if(attuale == utente->endMessaggi){
        utente->endMessaggi = prev;
    }
    if(prev!=NULL){
        prev->next = attuale->next;
    }
    utente->numMessaggi --;
    //sblocchiamo la scrittura di nuovi messaggi
    if(pthread_mutex_unlock(&(utente->mutexMess))!=0){
        printf("errore nello sbloccare il mutex\n");
        return 1;
    }
    //liberiamo la memoria del nodo messaggio
    free(attuale);
    //inviamo conferma eliminazione del nodo
    if(send_encrypted_int(socket,0,server_tx)!=0){
        printf("errore nell'inviare conferma eliminazione \n");
        return 1;
    }
    return 0;
}

//Funzione che viene eseguita dai thread che gestiscono l'interazione con i client
void *thread_main(void *client_socket){
    int socket = *((int *)client_socket); //cast del descrittore della socket
    char user[MAX_ID]; //buffer che identifica il thread per un determinato user loggato lato client
    // stringhe per la criptazione della socket con il client
    unsigned char server_pk[crypto_kx_PUBLICKEYBYTES], server_sk[crypto_kx_SECRETKEYBYTES];
    unsigned char server_rx[crypto_kx_SESSIONKEYBYTES], server_tx[crypto_kx_SESSIONKEYBYTES];
    unsigned char client_pk[crypto_kx_PUBLICKEYBYTES];
    // scambio di chiavi per la criptazione dei dati
    if(key_exchange(server_pk, server_sk, server_rx, server_tx, client_pk, socket) != 0){
        printf("non è stato possibile stabilire una connessione sicura");
        close(socket);
        pthread_exit(NULL);
    }
    // fase preautenticazione
    //riceviamo l'operazione richiesta dall'utente
    int16_t op;
    if (receive_encrypted_int(&op,socket,server_rx)!=0){
        printf("errore nella ricezione dell'operazione pre autenticazione \n");
        close(socket);
        pthread_exit(NULL);
    }
    // autentichiamo l'utente connesso
    if(authentication_client(user,op,server_rx,server_tx,socket)!=0){
        printf("errore nell'autenticazione del client \n");
        close(socket);
        pthread_exit(NULL);
    }
    if(op == 2){
        //eliminazione dell'utente all'interno della struttura dati
        if(deleteUtente(&listaUtenti,user)!=0){
            printf("errore nell'eliminazione dell'utente \n");
        }
        close(socket);
        pthread_exit(NULL);
    }
    while(1){
        // ricezione operazione richiesta dall'utente
        if (receive_encrypted_int(&op,socket,server_rx)!=0){
            printf("errore nella ricezione dell'operazione pre autenticazione \n");
            close(socket);
            pthread_exit(NULL);
        }
        if(op == 1){
            //scrivi messaggio
            if(writeMessage(user,server_rx,server_tx,socket) != 0){
                printf("errore nell'invio del messaggio\n");
                close(socket);
                pthread_exit(NULL);
            }
        } else {
            if(sendAllMessage(user,server_rx,server_tx,socket) != 0){
                printf("errore nella stampa di tutti i messaggi\n");
                close(socket);
                pthread_exit(NULL);
            }
            if(op == 2){
                if(deleteMessage(user,server_rx,server_tx,socket)!=0){
                    printf("Errore: impossibile eliminare il messaggio \n");
                    close(socket);
                    pthread_exit(NULL);
                }
            }
        }
    }
}

void handle_terminator(){
    printf("\nchiusura del server \n");
    //apriamo/creiamo il backup sulle credenziali
    FILE* credential; 
    if((credential=fopen(FILE_CREDENTIAL,"w+"))==NULL){
        printf("Errore: impossibile aprire il file di backup per le credenziali \n");
        exit(EXIT_FAILURE);
    }
    //blocchiamo la possibilità di eliminare o aggiungere utenti
    if(pthread_mutex_lock(&mutexListaUtenti)!=0){
        printf("Errore: non è possibile bloccare la scrittura sulla lista utenti\n");
        exit(EXIT_FAILURE);
    }
    //scorriamo tutti gli utenti
    Utente* utente = listaUtenti;
    Utente* succ = listaUtenti;
    FILE* fmess;
    Messaggio* messaggio;
    Messaggio* temp;
    while(utente != NULL){
        //blocchiamo la scrittura dei suoi messaggi in arrivo
        if(pthread_mutex_lock(&(utente->mutexMess))!=0){
            printf("Errore: impossibile bloccare la scrittura sui messaggi degli utenti \n");
            exit(EXIT_FAILURE);
        }
        //scriviamo le credenziali nel file di backup
        if(fprintf(credential,"%s %s\n",utente->username,utente->password)<=0){
            printf("Errore: impossibile scrivere sul file credenziali \n");
            exit(EXIT_FAILURE);
        }
        //apriamo il file per i messaggi associato all'utente
        if((fmess=fopen(utente->nome_file,"w+"))==NULL){
            printf("Errore: impossibile aprire il file per il backup di messaggi\n");
            exit(EXIT_FAILURE);
        }
        //scriviamo nel file tutti i messaggi salvati
        messaggio = utente->startMessaggi;
        temp = messaggio;
        while(messaggio!=NULL){
            if(fprintf(fmess,"%s\n%s\n%s\n",messaggio->mittente,messaggio->ogetto,messaggio->text)<=0){
                printf("Errore: impossibile scrivere sul file di backup dei messaggi \n");
                exit(EXIT_FAILURE);
            }
            temp = messaggio->next;
            free(messaggio);
            messaggio = temp;
        }
        //chiudiamo il file
        fclose(fmess);
        //passiamo all'utente successivo
        succ = utente->next;
        // liberiamo la memoria associata a quel'utente
        free(utente);
        utente=succ;
    }
    //chiudiamo il file delle credenziali
    fclose(credential);
    close(sock);
    exit(EXIT_SUCCESS);
}

//funzione che inizializza i segnali da ignorare esplicitamente e quelli da catturare
int initialization_signal(){
    sigset_t mask; // maschera dei segnali
    //inizializzazione della struttura per sigaction
    struct sigaction sa;
    sa.sa_handler = handle_terminator;
    sa.sa_flags = SA_RESTART;
    // Inizializza la maschera per includere tutti i segnali
    if (sigfillset(&mask) == -1) {
        perror("Errore durante l'inizializzazione della maschera");
        return 1;
    }
    // Rimuove SIGTERM, e SIGINT dalla mask
    sigdelset(&mask, SIGTERM);
    sigdelset(&mask, SIGINT);
    // Applica la maschera al processo
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1) {
        perror("Errore durante l'applicazione della maschera");
        return 1;
    }
    //impostazione esplicita dell'handle per sigint e sigterm
    // Configura sigaction per SIGTERM
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Errore durante la configurazione di SIGTERM");
        return 1;
    }
    // Configura sigaction per SIGINT
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Errore durante la configurazione di SIGINT");
        return 1;
    }
    return 0;
}

int main() {
    // inizializzazione dei segnali
    if(initialization_signal()!=0){
        printf("Errore: non è stato possibile inizializzare i segnali \n");
        exit(EXIT_FAILURE);
    }
    // inizializzazione della socket
    sock = initialization_socket();
    if(sock < 0){
        printf("errore nell'inizializzazione della socket \n");
        exit(EXIT_FAILURE);
    }
    //inizializzazione della libreria libsodium
    if(initialization_cryptolibrary() != 0){
        printf("errore nell'inizializzazione della libreria di crittografia \n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    //inizializzazione delle strutture dati che contengono utenti (credenziali) e messaggi
    if(initialization_database() != 0){
        printf("errore nell'inizializzazione dei file database \n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    //accettazione di nuove connessioni con i client attraverso threads separati
    int client_socket; // descrittore della connessione con il client
    while(1){
        //mettiamo la socket in stato di accettazione
        struct sockaddr_in client_addr; // inizializziamo una struttura per l'indirizzo del client
        socklen_t client_addr_len = sizeof(client_addr);
        client_socket = accept(sock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Errore nell'accettare la connessione");
            close(sock);
            exit(EXIT_FAILURE);
        }
        //creiamo un thread per ogni nuova connessione in ingresso
        pthread_t thread;
        if(pthread_create(&thread,NULL,thread_main,(void *)&client_socket) != 0){
            close(client_socket);
            close(sock);
            perror("error to create a thread");
            exit(EXIT_FAILURE);
        }
    }
    handle_terminator();
    return 0;
}