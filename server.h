#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sodium.h>
#include <fcntl.h>
#include <unistd.h>

#define fflush(stdin) while(getchar() != '\n')

#define PORTA_SERVER 8080
#define MAX_ONLINE 5
#define MAX_ID 13
#define MAX_PSWD 16
#define ENC_PSWD crypto_pwhash_STRBYTES
#define SIZE_HT 10
#define MAX_OBJECT 20
#define MAX_TEXT 200
#define SEPARATOR " " // separatore tra i campi delle strutture
#define FILE_CREDENTIAL "credentials.txt"

typedef struct Messaggio{
    char mittente[MAX_ID];
    char ogetto[MAX_OBJECT];
    char text[MAX_TEXT];
    struct Messaggio* next;
} Messaggio;

typedef struct Utente{
    char username[MAX_ID];
    char password[ENC_PSWD];
    char nome_file[MAX_ID+4];
    Messaggio* startMessaggi;
    Messaggio* endMessaggi;
    pthread_mutex_t mutexMess;
    int16_t numMessaggi;
    struct Utente* next;
}Utente;

typedef struct Thread{
    pthread_t tid;
    int socket;
    struct Thread* next;
}Thread;

Utente* listaUtenti;
Thread* listaThreads;
pthread_mutex_t mutexListaThreads;
pthread_mutex_t mutexListaUtenti;
int sock; // socket che si mette in listen

int initialization_socket();

int initialization_cryptolibrary();

int insertMessaggio(Utente* utente, Messaggio messaggio);

int send_data(void *data, size_t size, int socket);

int receive_data(void *data, size_t size, int socket);

int send_int(int16_t value, int socket);

int receive_int(int16_t* num,int socket);

int send_encrypted_data(int socket, const unsigned char *data, size_t data_length, const unsigned char *tx_key);

int receive_encrypted_data(int socket, unsigned char *received_data, size_t max_data_length, const unsigned char *rx_key);

int receive_encrypted_int(int16_t* num,int socket, const unsigned char *rx_key);

int send_encrypted_int(int socket, int16_t value, const unsigned char *tx_key);
