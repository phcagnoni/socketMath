CC = gcc
CFLAGS = -Wall -Wextra -O2
SRCDIR = src
TESTDIR = tests
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

# Executar testes automatizados
test: all $(TESTDIR)
	@echo "Executando testes automatizados..."
	@if [ -f $(TESTDIR)/run_tests.sh ]; then \
		chmod +x $(TESTDIR)/run_tests.sh; \
		cd $(TESTDIR) && ./run_tests.sh; \
	else \
		echo "Arquivo $(TESTDIR)/run_tests.sh não encontrado!"; \
	fi

# Criar diretório de testes
$(TESTDIR):
	mkdir -p $(TESTDIR)

.PHONY: all server client clean run-server run-client test