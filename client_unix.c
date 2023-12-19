#include "client_unix.h" // file header contenente strutture dati, costanti e fime delle fuznioni

//variabili glocbali per lo scambio di messaggi criptati con il server
unsigned char client_pk[crypto_kx_PUBLICKEYBYTES], client_sk[crypto_kx_SECRETKEYBYTES];
unsigned char client_rx[crypto_kx_SESSIONKEYBYTES], client_tx[crypto_kx_SESSIONKEYBYTES];
unsigned char server_pk[crypto_kx_PUBLICKEYBYTES];
// variabile globale per il descrittore della socket
int sock;

int initialization_socket(){
    // Inizializzazione del descrittore della socket
    int server_socket = socket(AF_INET,SOCK_STREAM,0);
    if( server_socket <= 0){
        perror("Impossibile creare socket");
        exit(-1);
    }
    // Binding sock
    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));  
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(PORTA_SERVER);
    // Conversione indirizzo IP da formato testuale a binario
    if (inet_pton(AF_INET, IP_SERVER, &(addr_server.sin_addr)) <= 0) {
        perror("Errore nel convertire l'indirizzo IP");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    // Connessione al server
    if (connect(server_socket, (struct sockaddr*)&addr_server, sizeof(addr_server)) < 0) {
        perror("Errore nella connessione al server");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("connessione al server stabilita \n");
    return server_socket;
}

// funzione che inizializza la libreria libsodium per la criptazione dei dati sulla sock
int initialization_cryptolibrary(){
    //inizializzazione della libreria sodium
    if (sodium_init() < 0) {
        perror("Errore inizializzazione libreria sodium");
        return 1;
    }
    // Generazione delle chiavi lato client
    crypto_kx_keypair(client_pk, client_sk);
    // Ricezione della chiave pubblica del server
    if(receive_data(server_pk,crypto_kx_PUBLICKEYBYTES,sock)!=0){
        printf("Errore: problema nella ricezione del server_pk \n");
        return 1;
    }
    //Invio della chiave pubblica al server
    if(send_data(client_pk,crypto_kx_PUBLICKEYBYTES,sock)!=0){
        printf("Errore nell'invio del client_pk \n");
        return 1;
    }
    // creazione della sessione di criptazione
    if (crypto_kx_client_session_keys(client_rx, client_tx,client_pk, client_sk, server_pk) != 0) {
        printf("Errore chiave pubblica del server, riavviare l'applicazione \n");
        return 1;
    }
    printf("la connessione ora è sicura\n");
    return 0;
}

int authentication(char* user){ // ricordati che nel server il mutex deve essere preso prima del controllo nella liosta degli utenti perchè così evito che nello stesso momento si registrano con lo stesso nome
    //richiesta di un nome utente
    char* username; // buffer temporaneo per username inserito dall'utente
    int16_t validate = -1; // "instruzione lato server" operatore che comunica lo stato del server al client
    do{
        //Allocazione temporanea di memoria per l'username
        if((username = (char*)malloc(MAX_ID * sizeof(char))) == NULL){
            perror("error to malloc for username");
            return 1;
        }
        //richiesta dell'username
        if(stringrequire(username,MAX_ID,"username")!=0){
            printf("Errore nella richiesta dell'username \n");
            return 1;
        }
        //invio dell'username al server
        if(send_encrypted_data(sock,(const unsigned char *)username,MAX_ID,client_tx)!=0){
            printf("Errore nell'invio dell'username al server \n");
            return 1;
        }
        // attesa della risposta da parte del server
        if(receive_encrypted_int(&validate,sock,client_rx) !=0){
            printf("Errore nella ricezione della rispodta del client \n");
            return 1;
        }
        if(validate == 0){ //il server  conferma la ricezione dell'username
            strcpy(user,username); // copiamo l'username nel buffer definitivo
            printf("username accettato! \n");
        } else if(validate == 1){ // errore in caso di registrazione 
            printf("username già in uso! per favore inseriscine uno diverso\n");
        } else if(validate == 2){ // errore in caso di accesso o eliminazione dell'account
            printf("username inesistente! per favore inseriscine uno corretto\n");
        }
        //liberiamo memoria allocata dinamicamente
        free(username);
    }while(validate!=0);

    //richiesta password utente
    char *password; // buffer temporaneo per la password in chiaro
    validate = -1; // reset al valore di default della risposta lato server
    do{
        // allocazione di memoria per la password in chiaro
        if((password = (char*)malloc(MAX_PSWD * sizeof(char))) == NULL){
            perror("error to malloc for password");
            return 1;
        }

        //richiesta password
        if(stringrequire(password,MAX_PSWD,"password") != 0){
            printf("errore nella richiesta della password \n");
            return 1;
        }

        // invio della password in chiaro
        if(send_encrypted_data(sock,(const unsigned char*)password,MAX_PSWD,client_tx)!=0){
            printf("errore nell'invio della password verso il server \n");
            return 1;
        }

        // attesa della risposta da parte del server
        if(receive_encrypted_int(&validate,sock,client_rx) !=0){
            printf("errore nella ricezione della rispodta del client \n");
        }
        if(validate == 1){ // password errata in caso di accesso o eliminazione dall'account
            printf("password errata, per favore inserisci la password corretta \n");
        }
        free(password); // deallochiamo memoria per password in chiaro
    }while(validate != 0);
    return 0;
}

int sendMessage(){
    char* dest;
    char* object;
    char* text;
    int16_t validate = -1;
    do{
        // allocazione di memoria per il destinatario
        if((dest = (char*)malloc(MAX_ID * sizeof(char))) == NULL){
            perror("Errore allocazione memoria per destinatario");
            return 1;
        }
        //richiesta dell'username
        if(stringrequire(dest,MAX_ID,"destinatario")!=0){
            printf("Errore nella richiesta del destinatario \n");
            return 1;
        }
        //invio dell'username al server
        if(send_encrypted_data(sock,(const unsigned char *)dest,MAX_ID,client_tx)!=0){
            printf("Errore nell'invio dell'username al server \n");
            return 1;
        }
        // attesa della risposta da parte del server
        if(receive_encrypted_int(&validate,sock,client_rx) !=0){
            printf("errore nella ricezione della rispodta del client \n");
            return 1;
        }
        if(validate == 0){ //il server  conferma la ricezione dell'username
            printf("Destinatario trovato \n");
        } else if(validate == 1){ // errore in caso di registrazione 
            printf("non puoi inviare messaggi a te stesso!\n");
        } else if(validate == 2){ // errore in caso di accesso o eliminazione dell'account
            printf("dstinatario inesistente!\n");
        }
        free(dest);
    }while(validate!=0);
    do{
        // allocazione di memoria per l'ogetto
        if((object = (char*)malloc(MAX_OBJECT * sizeof(char))) == NULL){
            perror("error to malloc for destinatario");
            return 1;
        }
        //richiesta dell'ogetto
        if(stringrequire(object,MAX_OBJECT,"ogetto")!=0){
            printf("errore nella richiesta del'username \n");
            return 1;
        }
        //invio ogetto al server
        if(send_encrypted_data(sock,(const unsigned char *)object,MAX_OBJECT,client_tx)!=0){
            printf("errore nell'invio dell'ogetto al server \n");
            return 1;
        }
        // attesa della risposta da parte del server
        if(receive_encrypted_int(&validate,sock,client_rx) !=0){
            printf("errore nella ricezione della rispodta del client \n");
            return 1;
        }
        if(validate != 0){ //il server  conferma la ricezione dell'ogetto
            printf("Errore imprevisto \n");
        }
        free(object);
    }while(validate!=0);
    do{
        // allocazione di memoria per il testo
        if((text = (char*)malloc(MAX_TEXT * sizeof(char))) == NULL){
            perror("error to malloc for destinatario");
            return 1;
        }
        //richiesta del testo
        if(stringrequire(text,MAX_TEXT,"testo")!=0){
            printf("errore nella richiesta del testo \n");
            return 1;
        }
        //invio del testo al server
        if(send_encrypted_data(sock,(const unsigned char *)text,MAX_TEXT,client_tx)!=0){
            printf("errore nell'invio del testo al server \n");
            return 1;
        }
        // attesa della risposta da parte del server
        if(receive_encrypted_int(&validate,sock,client_rx) !=0){
            printf("errore nella ricezione della rispodta del client \n");
            return 1;
        }
        if(validate != 0){ //il server  conferma l'invio del messaggio
            printf("Errore imprevisto \n");
        }
    }while(validate!=0);
    return 0;
}

int showallMessage(int numMess){
    char mittente[MAX_ID];
    char ogetto[MAX_OBJECT];
    char testo[MAX_TEXT];
    for(int i=0;i<numMess;i++){
        // ricezione del mittente
        if(receive_encrypted_data(sock,mittente,MAX_ID,client_rx)!=0){
            printf("errore nella ricezione del mittente del messaggio \n");
            return 1;
        }
        //invio conferma ricezione
        if(send_encrypted_int(sock,(int16_t)0,client_tx)!=0){
            printf("errore nell'invio della conferma ricezione \n");
            return 1;
        }
        //ricezione dell'oggetto
        if(receive_encrypted_data(sock,ogetto,MAX_OBJECT,client_rx)!=0){
            printf("errore nella ricezione dell'ogetto del messaggio \n");
            return 1;
        }
        //invio conferma ricezione
        if(send_encrypted_int(sock,(int16_t)0,client_tx)!=0){
            printf("errore nell'invio della conferma ricezione \n");
            return 1;
        }
        //ricezione testo
        if(receive_encrypted_data(sock,testo,MAX_TEXT,client_rx)!=0){
            printf("errore nella ricezione del testo del messaggio \n");
            return 1;
        }
        //stampa del messaggio con annesso il numero identificativo
        printf("[%d]\t%s\n\t%s\n\t%s\n",i,mittente,ogetto,testo);
        //invio conferma ricezione
        if(send_encrypted_int(sock,(int16_t)0,client_tx)!=0){
            printf("errore nell'invio della conferma ricezione \n");
            return 1;
        }
    }
    return 0;
}

int deleteMessage(int16_t numMess){
    if(numMess == 0){
        printf("Non hai messaggi in bacheca \n");
        return 0;
    }
    int16_t num;
    if(intrequire(&num,numMess-1,"numero del messaggio")!=0){
        printf("Errore: impossibile inserire un numero di messaggio \n");
        return 1;
    }
    if(send_encrypted_int(sock,num,client_tx)!=0){
        printf("Errore: impossibile inviare al server il numero del messaggio da eliminare \n");
        return 1;
    }
    if(receive_encrypted_int(&num,sock,client_rx)!=0){
        printf("Errore: impossibile ricevere conferma eliminazione dal client\n");
        return 1;
    }
    printf("messaggio eliminato... \n");
}

//funzione terminatore del processo
void handle_terminator(){
    //chiudiamo la connessione con il server
    close(sock);
    printf("\nconnessione con il client chiusa correttamente \nArrivederci\n");
    exit(EXIT_SUCCESS);
}

// Funzione che ignora esplicitamente tutti i segnali tranne sigterm,sigint,sighup che vengono gestiti dall'handle terminatore
int initializationsignal(){
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
    // Rimuove SIGTERM, SIGHUP, e SIGINT dalla mask
    sigdelset(&mask, SIGTERM);
    sigdelset(&mask, SIGHUP);
    sigdelset(&mask, SIGINT);
    // Applica la maschera al processo
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1) {
        perror("Errore durante l'applicazione della maschera");
        return 1;
    }
    //impostazione esplicita dell'handle per sigint,sighup e sigterm
    // Configura sigaction per SIGTERM
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Errore durante la configurazione di SIGTERM");
        return 1;
    }
    // Configura sigaction per SIGHUP
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("Errore: configurazione di SIGHUP");
        return 1;
    }
    // Configura sigaction per SIGINT
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Errore durante la configurazione di SIGINT");
        return 1;
    }
    return 0;
}

// Funzione principale del client
int main(){
    char user[MAX_ID]; //variabile locale che individua l'utente loggato per una determinata istanza di client in running
    //inizializzazione gestione dei segnali
    if(initializationsignal()!=0){
        printf("Errore: impossibile gestire i segnali \n");
        exit(EXIT_FAILURE);
    }
    //inizializzazione sock del client
    if((sock = initialization_socket()) <0){
        printf("errore nell'inizializazione della sock con il server \n");
        exit(EXIT_FAILURE);
    }
    //inizializzazione della libreria libsodium e connessione sicura con il server
    if(initialization_cryptolibrary() != 0){
        printf("errore nell'inizializazione della libreria di criptazione \n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    // richiesta operazione da eseguire: accesso registrazione o eliminazione account
    char *option[] = {"accesso", "registrazione", "eliminazione account"};
    int16_t op;
    if( operationrequire(&op,option,3) != 0){ //funzione che chiede all'utente di scegliere un'opzione fra quelle elencate
        printf("errore nella richiesta di un'operazione pre autenticazione \n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    //invio operazione da eseguire al server
    if( send_encrypted_int(sock,op,client_tx) != 0){
        printf("errore invio operazioen pre autenticazione \n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    //invio delle credenziali al server
    if(authentication(user) !=0){
        printf("errore durante la registrazione \n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    if(op==0){
        printf("Accesso avvenuto con successo! \nBentornato %s! \n", user);
    } else if( op == 1){
        printf("Registrazione avvenuta con successo! \nBenvenuto %s! \n",user);
    } else{
        printf("Eliminazione dell'account avvenuta correttamente! \nArrivederci %s! \n",user);
        handle_terminator();
    }

    //main client
    option[0] = "leggi messaggi";
    option[1] = "scrivi messaggi";
    option[2] = "elimina messaggio";
    op = -1;
    int16_t numMess;
    while(1){
        //richiesta operazione
        if( operationrequire(&op,option,3) != 0){
            printf("errore nella richiesta di un'operazione pre autenticazione \n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        //invio operazione
        if( send_encrypted_int(sock,op,client_tx) != 0){
            printf("errore invio operazioen pre autenticazione \n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        if( op == 1){
            sendMessage(sock);
        } else {
            // ricezione del numero dei messaggi in arrivo
            if((receive_encrypted_int(&numMess,sock,client_rx)) != 0){
                printf("errore nella ricezione del numero dei messaggi \n");
                close(sock);
                return 1;
            }
            //invio conferma ricezione
            if(send_encrypted_int(sock,0,client_tx)!=0){
                printf("errore nell'invio della conferma ricezione \n");
                close(sock);
                return 1;
            }
            showallMessage(numMess);
            if( op == 2){
                deleteMessage(numMess);
            }
        }
    }
    close(sock);
    return 0;
}