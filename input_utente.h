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
#include <sodium.h>

#define fflush(stdin) while(getchar() != '\n')
#define MIN_CHAR 4


int credentialrequirment(char* string, size_t size, char* ogetto);

int operationrequire(int16_t* num,char** options, int numopt);

int intrequire(int16_t *num, int max, char *oggetto);