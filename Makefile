CC = gcc
CFLAGS = -Wall -Wextra -O2
SRCDIR = src
PORT ?= 5050

# Arquivos fonte
SERVER_SRC = $(SRCDIR)/server.c
CLIENT_SRC = $(SRCDIR)/client.c

# Executáveis
SERVER = server
CLIENT = client

# Regra padrão
all: $(SERVER) $(CLIENT)

# Compilar servidor
$(SERVER): $(SERVER_SRC) $(SRCDIR)/common.h
	$(CC) $(CFLAGS) -I$(SRCDIR) -o $@ $(SERVER_SRC)

# Compilar cliente
$(CLIENT): $(CLIENT_SRC) $(SRCDIR)/common.h
	$(CC) $(CFLAGS) -I$(SRCDIR) -o $@ $(CLIENT_SRC)

# Targets individuais (requisito)
server: $(SERVER)
client: $(CLIENT)

# Limpeza
clean:
	rm -f $(SERVER) $(CLIENT)

# Executar servidor
run-server: $(SERVER)
	./$(SERVER) $(PORT)

# Executar cliente
run-client: $(CLIENT)
	./$(CLIENT) 127.0.0.1 $(PORT)

.PHONY: all server client clean run-server run-client
