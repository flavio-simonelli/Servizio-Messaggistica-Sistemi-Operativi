# Nome dei file sorgente
SERVER_SOURCE = server.c transfert_socket.c
CLIENT_SOURCE = client_unix.c input_utente.c transfert_socket.c

# Nome degli eseguibili
SERVER_EXEC = server
CLIENT_EXEC = client

# Opzioni di compilazione
CFLAGS = -lpthread -lsodium
LDFLAGS = -lpthread -lsodium

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_SOURCE)
	gcc $(SERVER_SOURCE) -o $(SERVER_EXEC) $(LDFLAGS)

$(CLIENT_EXEC): $(CLIENT_SOURCE)
	gcc $(CLIENT_SOURCE) -o $(CLIENT_EXEC) $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)

# Verifica e installazione della libreria libsodium
check_libsodium:
	@if [ ! -f /usr/lib/libsodium.a ]; then \
		echo "La libreria libsodium non è installata. Installazione in corso..."; \
		sudo apt-get update && sudo apt-get install -y libsodium-dev; \
	fi

# Regola target per controllare e installare libsodium prima di compilare
$(SERVER_EXEC): check_libsodium
$(CLIENT_EXEC): check_libsodium

# Regola target per compilare il server senza controllare libsodium
server_nocheck: $(SERVER_SOURCE)
	gcc $(SERVER_SOURCE) -o $(SERVER_EXEC) $(CFLAGS)

# Regola target per compilare il client senza controllare libsodium
client_nocheck: $(CLIENT_SOURCE)
	gcc $(CLIENT_SOURCE) -o $(CLIENT_EXEC) $(CFLAGS)
