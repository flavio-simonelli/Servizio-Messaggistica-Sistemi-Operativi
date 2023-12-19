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

int send_data(void *data, size_t size, int socket);

int receive_data(void *data, size_t size, int socket);

int send_int(int16_t value, int socket);

int receive_int(int16_t* num,int socket);

int send_encrypted_data(int socket, const unsigned char *data, size_t data_length, const unsigned char *tx_key);

int receive_encrypted_data(int socket, unsigned char *received_data, size_t max_data_length, const unsigned char *rx_key);

int receive_encrypted_int(int16_t* num,int socket, const unsigned char *rx_key);

int send_encrypted_int(int socket, int16_t value, const unsigned char *tx_key);