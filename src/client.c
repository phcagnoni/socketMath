// Cliente para operações matemáticas TCP
// Compilar: make client
// Executar: ./client [ip] [porta]

#include "common.h"

static volatile int client_running = 1;
static int sockfd = -1;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Handler para SIGINT (Ctrl+C) - CORRIGIDO
static void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("\n[CLIENTE] Recebido SIGINT (Ctrl+C), encerrando...\n");
        client_running = 0;
        if (sockfd != -1) {
            // Tentar enviar QUIT antes de fechar
            send(sockfd, "QUIT\n", 5, 0);
            close(sockfd);
        }
        exit(0); // Força saída imediata
    }
}

// Remove espaços no início e fim da string
static void trim_string(char *str) {
    char *start = str;
    while (*start && isspace(*start)) start++;
    
    if (*start == '\0') {
        str[0] = '\0';
        return;
    }
    
    char *end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) end--;
    *(end + 1) = '\0';
    
    // Move string trimada para o início
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

static void print_help() {
    printf("\n=== CALCULADORA CLIENTE-SERVIDOR ===\n");
    printf("Comandos suportados:\n");
    printf("  Formato Prefixo:\n");
    printf("    ADD A B    - Adição (A + B)\n");
    printf("    SUB A B    - Subtração (A - B)\n");
    printf("    MUL A B    - Multiplicação (A * B)\n");
    printf("    DIV A B    - Divisão (A / B)\n");
    printf("\n  Formato Infixa (Bônus):\n");
    printf("    A + B      - Adição\n");
    printf("    A - B      - Subtração\n");
    printf("    A * B      - Multiplicação\n");
    printf("    A / B      - Divisão\n");
    printf("\n  Comandos Especiais:\n");
    printf("    HELP       - Mostra ajuda do servidor\n");
    printf("    VERSION    - Versão do servidor\n");
    printf("    QUIT       - Encerra conexão\n");
    printf("\n  Exemplos:\n");
    printf("    ADD 10 5\n");
    printf("    10 + 5\n");
    printf("    MUL -3 2.5\n");
    printf("    15.5 / 2\n");
    printf("\nDigite Ctrl+C ou QUIT para sair\n\n");
}

int main(int argc, char **argv) {
    char* server_ip = "127.0.0.1";
    int port = DEFAULT_PORT;
    
    // Parse dos argumentos
    if (argc > 1) {
        server_ip = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Porta inválida: %d\n", port);
            return EXIT_FAILURE;
        }
    }
    if (argc > 3) {
        fprintf(stderr, "Uso: %s [ip-servidor] [porta]\n", argv[0]);
        fprintf(stderr, "Ex.: %s 127.0.0.1 5050\n", argv[0]);
        return EXIT_FAILURE;
    }

    // CONFIGURAR HANDLER DE SINAL - CORRIGIDO
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // Criar socket TCP IPv4
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) die("socket");

    // Configurar endereço do servidor
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((uint16_t)port);
    
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "IP inválido: %s\n", server_ip);
        close(sockfd);
        return EXIT_FAILURE;
    }

    // Conectar ao servidor
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        die("connect");
    }

    printf("Conectado ao servidor %s:%d\n", server_ip, port);
    print_help();

    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];

    while (client_running) {
        printf("> ");
        fflush(stdout);

        // Ler entrada do usuário
        if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) {
            // EOF (Ctrl+D): enviar QUIT antes de sair
            printf("\nEnviando QUIT e encerrando...\n");
            strcpy(sendbuf, "QUIT\n");
            send(sockfd, sendbuf, strlen(sendbuf), 0);
            break;
        }

        // Remove quebra de linha e espaços extras
        trim_string(sendbuf);
        
        // Ignora linhas vazias
        if (strlen(sendbuf) == 0) {
            continue;
        }

        // Comandos de ajuda local
        if (strcasecmp(sendbuf, "help") == 0 || strcasecmp(sendbuf, "?") == 0) {
            print_help();
            continue;
        }

        // Verificar comandos de saída
        if (strcasecmp(sendbuf, "quit") == 0 || 
            strcasecmp(sendbuf, "exit") == 0 ||
            strcasecmp(sendbuf, "bye") == 0) {
            strcpy(sendbuf, "QUIT");
        }

        // Adicionar quebra de linha para o protocolo
        strcat(sendbuf, "\n");

        // Enviar para o servidor
        ssize_t n = send(sockfd, sendbuf, strlen(sendbuf), 0);
        if (n < 0) {
            perror("send");
            break;
        }

        // Se foi QUIT, receber resposta e sair
        if (strncmp(sendbuf, "QUIT", 4) == 0) {
            ssize_t recv_bytes = recv(sockfd, recvbuf, sizeof(recvbuf) - 1, 0);
            if (recv_bytes > 0) {
                recvbuf[recv_bytes] = '\0';
                printf("%s", recvbuf);
            }
            break;
        }

        // Receber resposta do servidor
        ssize_t recv_bytes = recv(sockfd, recvbuf, sizeof(recvbuf) - 1, 0);
        if (recv_bytes <= 0) {
            if (recv_bytes < 0) {
                perror("recv");
            } else {
                printf("Servidor encerrou a conexão.\n");
            }
            break;
        }

        recvbuf[recv_bytes] = '\0';
        
        // Formatar resposta do servidor
        if (strncmp(recvbuf, "OK ", 3) == 0) {
            printf("Resultado: %s", recvbuf + 3);
        } else if (strncmp(recvbuf, "ERR ", 4) == 0) {
            printf("Erro: %s", recvbuf + 4);
        } else {
            printf("%s", recvbuf);
        }
    }

    close(sockfd);
    printf("Cliente encerrado.\n");
    return 0;
}