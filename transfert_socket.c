#include "transfert_socket.h"
//invio dato da socket in chiaro
int send_data(void *data, size_t size, int socket) {
    int sent_total = 0;
    int sent_partial;
    while(sent_total < (int)size){
        if ((sent_partial = send(socket, data + sent_total, size - sent_total, 0)) == -1) {
            perror("Errore nell'invio dei dati");
            return 1;
        }
        sent_total += sent_partial;
    }
    return 0;
}

//ricezione dato da socket in chiaro
int receive_data(void *data, size_t size, int socket) {
    int recv_total = 0;
    int recv_partial;
    while(recv_total<(int)size){
        if ((recv_partial = recv(socket, data + recv_total, size - recv_total, 0)) == -1) {
        perror("Errore nella ricezione dei dati");
        return 1;
        }
        recv_total += recv_partial;
    }
    return 0;
}

// invio intero in formato network
int send_int(int16_t value, int socket) {
    value = htonl(value);
    if(send_data(&value, sizeof(value), socket)!= 0){
        printf("errore nell'invio dell'intero");
        return 1;
    }
    return 0;
}

// ricezione di un intero in formato network e trasformazione in hardware
int receive_int(int16_t* num,int socket) {
    int value; 
    if(receive_data(&value, sizeof(value), socket) != 0){
        printf("errore nella ricezione dell'intero \n");
        return 1;
    }
    *num = ntohl(value);
    return 0;
}


int send_encrypted_data(int socket, const unsigned char *data, size_t data_length, const unsigned char *tx_key) {
    // Allocazione del buffer per i dati cifrati
    unsigned char encrypted_data[data_length + crypto_secretbox_MACBYTES];
    // Generazione di un nonce univoco (NONCE = Number Used Once)
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes(nonce, sizeof(nonce));
    // Cifratura dei dati
    if (crypto_secretbox_easy(encrypted_data, data, data_length, nonce, tx_key) != 0) {
        perror("Encryption failed");
        exit(EXIT_FAILURE);
    }
    if(send_data(nonce,sizeof(nonce),socket) != 0){
        printf("errore nell'invio del nonce \n");
        return 1;
    }
    if(send_data(encrypted_data,sizeof(encrypted_data),socket) != 0){
        printf("errore nell'invio dei dati criptati \n");
        return 1;
    }
    return 0;
}


int receive_encrypted_data(int socket, unsigned char *received_data, size_t max_data_length, const unsigned char *rx_key) {
    // Ricezione del nonce
    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    if(receive_data(nonce,sizeof(nonce),socket) != 0){
        printf("errore nella ricezione del nonce \n");
        return 1;
    }
    // Ricezione dei dati cifrati
    unsigned char encrypted_data[max_data_length + crypto_secretbox_MACBYTES];
    if(receive_data(encrypted_data,sizeof(encrypted_data),socket)!= 0){
        printf("errore nella ricezione dei dati criptati \n");
        return 1;
    }
    // Decifratura dei dati
    if (crypto_secretbox_open_easy(received_data, encrypted_data, sizeof(encrypted_data), nonce, rx_key) != 0) {
        perror("Decryption failed");
        return 1;
    }
    return 0;
}

int receive_encrypted_int(int16_t* num,int socket, const unsigned char *rx_key) {
    // Ricevi l'intero cifrato
    uint32_t received_value_network_order;
    if(receive_encrypted_data(socket, (unsigned char *)&received_value_network_order, sizeof(received_value_network_order), rx_key) != 0){
        printf("errore nella ricezione dell'intero criptato \n");
        return 1;
    }
    // Converti l'intero in host order
    *num = ntohl(received_value_network_order);
    return 0;
}

int send_encrypted_int(int socket, int16_t value, const unsigned char *tx_key) {
    // Converti l'intero in network order
    uint32_t value_network_order = htonl(value);
    // Invia l'intero cifrato
    if(send_encrypted_data(socket, (const unsigned char *)&value_network_order, sizeof(value_network_order), tx_key) != 0){
        printf("errore nell'invio dell'intero criptato \n");
        return 1;
    }
    return 0;
}