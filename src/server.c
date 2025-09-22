// Servidor para operações matemáticas TCP com suporte a múltiplos clientes
// Compilar: make server
// Executar: ./server [porta]

#include "common.h"

static volatile int server_running = 1;
static int listen_fd = -1;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}


static void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("\n[SERVIDOR] Recebido SIGINT (Ctrl+C), encerrando...\n");
        server_running = 0;
        if (listen_fd != -1) {
            close(listen_fd);
        }
        exit(0); // Força saída imediata
    }
}

// Log com timestamp
void log_message(const char* format, ...) {
    time_t now;
    char timestamp[64];
    va_list args;
    
    time(&now);
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", localtime(&now));
    
    printf("[%s] ", timestamp);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

// Parse da operação (formato prefixo)
operation_t parse_operation(const char* op_str) {
    if (strcmp(op_str, "ADD") == 0) return OP_ADD;
    if (strcmp(op_str, "SUB") == 0) return OP_SUB;
    if (strcmp(op_str, "MUL") == 0) return OP_MUL;
    if (strcmp(op_str, "DIV") == 0) return OP_DIV;
    if (strcmp(op_str, "QUIT") == 0) return OP_QUIT;
    if (strcmp(op_str, "HELP") == 0) return OP_HELP;
    if (strcmp(op_str, "VERSION") == 0) return OP_VERSION;
    return OP_INVALID;
}

// Parse da operação (formato infix)
operation_t parse_infix_operation(const char* op_str) {
    if (strcmp(op_str, "+") == 0) return OP_ADD;
    if (strcmp(op_str, "-") == 0) return OP_SUB;
    if (strcmp(op_str, "*") == 0) return OP_MUL;
    if (strcmp(op_str, "/") == 0) return OP_DIV;
    return OP_INVALID;
}

// Parse de número decimal
int parse_double(const char* str, double* value) {
    char* endptr;
    *value = strtod(str, &endptr);
    return (*endptr == '\0' && endptr != str);
}

// Parse expressão infixa (A op B)
int parse_infix_expression(const char* expr, operation_t* op, double* a, double* b) {
    char* tokens[3];
    char* token;
    int token_count = 0;
    char expr_copy[BUFFER_SIZE];
    
    strcpy(expr_copy, expr);
    token = strtok(expr_copy, " ");
    while (token != NULL && token_count < 3) {
        tokens[token_count++] = token;
        token = strtok(NULL, " ");
    }
    
    if (token_count != 3) return 0;
    
    if (!parse_double(tokens[0], a)) return 0;
    *op = parse_infix_operation(tokens[1]);
    if (*op == OP_INVALID) return 0;
    if (!parse_double(tokens[2], b)) return 0;
    
    return 1;
}

// Executa cálculo matemático
math_result_t calculate(operation_t op, double a, double b) {
    math_result_t result = {0, 0.0, "", ""};
    
    switch (op) {
        case OP_ADD:
            result.success = 1;
            result.result = a + b;
            break;
        case OP_SUB:
            result.success = 1;
            result.result = a - b;
            break;
        case OP_MUL:
            result.success = 1;
            result.result = a * b;
            break;
        case OP_DIV:
            if (b == 0.0) {
                result.success = 0;
                strcpy(result.error_code, ERR_EZDV);
                strcpy(result.error_msg, "divisao_por_zero");
            } else {
                result.success = 1;
                result.result = a / b;
            }
            break;
        default:
            result.success = 0;
            strcpy(result.error_code, ERR_EINV);
            strcpy(result.error_msg, "operacao_invalida");
    }
    
    return result;
}

// Processa requisição do cliente
static int process_request(int client_fd, char* request, char* client_ip) {
    char response[BUFFER_SIZE];
    char* tokens[MAX_TOKENS];
    char* token;
    int token_count = 0;
    char request_copy[BUFFER_SIZE];
    
    // Remove quebra de linha
    request[strcspn(request, "\n\r")] = '\0';
    
    if (strlen(request) == 0) return 1;
    
    log_message("Requisição de %s (fd=%d): %s", client_ip, client_fd, request);
    
    // Tokenizar requisição
    strcpy(request_copy, request);
    token = strtok(request_copy, " ");
    while (token != NULL && token_count < MAX_TOKENS) {
        tokens[token_count++] = token;
        token = strtok(NULL, " ");
    }
    
    if (token_count < 1) {
        snprintf(response, BUFFER_SIZE, "ERR %s entrada_invalida\n", ERR_EINV);
    } else {
        operation_t op = parse_operation(tokens[0]);
        
        // Comandos especiais
        if (op == OP_QUIT) {
            log_message("Cliente %s (fd=%d) solicitou desconexão", client_ip, client_fd);
            snprintf(response, BUFFER_SIZE, "OK bye\n");
            send(client_fd, response, strlen(response), 0);
            return 0; // Sinaliza desconexão
        } else if (op == OP_HELP) {
            snprintf(response, BUFFER_SIZE, 
                "OK Comandos: ADD/SUB/MUL/DIV A B, A+/-/*/B, HELP, VERSION, QUIT\n");
        } else if (op == OP_VERSION) {
            snprintf(response, BUFFER_SIZE, "OK Server v%s\n", PROTOCOL_VERSION);
        } else if (op == OP_INVALID) {
            // Tentar formato infixa
            operation_t infix_op;
            double a, b;
            if (parse_infix_expression(request, &infix_op, &a, &b)) {
                math_result_t result = calculate(infix_op, a, b);
                if (result.success) {
                    snprintf(response, BUFFER_SIZE, "OK %.6f\n", result.result);
                } else {
                    snprintf(response, BUFFER_SIZE, "ERR %s %s\n", result.error_code, result.error_msg);
                }
            } else {
                snprintf(response, BUFFER_SIZE, "ERR %s operacao_invalida\n", ERR_EINV);
            }
        } else {
            // Formato prefixo padrão
            if (token_count != 3) {
                snprintf(response, BUFFER_SIZE, "ERR %s formato_invalido\n", ERR_EINV);
            } else {
                double a, b;
                if (!parse_double(tokens[1], &a) || !parse_double(tokens[2], &b)) {
                    snprintf(response, BUFFER_SIZE, "ERR %s numeros_invalidos\n", ERR_EINV);
                } else {
                    math_result_t result = calculate(op, a, b);
                    if (result.success) {
                        snprintf(response, BUFFER_SIZE, "OK %.6f\n", result.result);
                    } else {
                        snprintf(response, BUFFER_SIZE, "ERR %s %s\n", result.error_code, result.error_msg);
                    }
                }
            }
        }
    }
    
    log_message("Resposta para %s (fd=%d): %s", client_ip, client_fd, response);
    
    if (send(client_fd, response, strlen(response), 0) < 0) {
        log_message("Erro ao enviar resposta para %s (fd=%d)", client_ip, client_fd);
        return 0;
    }
    
    return 1;
}

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Porta inválida: %d\n", port);
            return EXIT_FAILURE;
        }
    }

    // CONFIGURAR HANDLER DE SINAL
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // Criar socket TCP IPv4
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) die("socket");

    // Permitir reutilização de endereço
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
    }

    // Configurar endereço do servidor
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind e Listen
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) die("bind");
    if (listen(listen_fd, 8) < 0) die("listen");

    log_message("Servidor iniciado na porta %d", port);
    log_message("Pressione Ctrl+C para encerrar");
    
    // Gerenciamento de múltiplos clientes
    int clients[MAX_CLIENTS];
    char client_ips[MAX_CLIENTS][INET_ADDRSTRLEN];
    
    for(int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
        memset(client_ips[i], 0, INET_ADDRSTRLEN);
    }

    fd_set allset, rset;
    FD_ZERO(&allset);
    FD_SET(listen_fd, &allset);
    int maxfd = listen_fd;
    int max_i = -1;
    char buf[BUFFER_SIZE];

    while (server_running) {
        rset = allset;
        
        // TIMEOUT PARA PERMITIR VERIFICAÇÃO DE server_running
        struct timeval timeout;
        timeout.tv_sec = 1;  // 1 segundo
        timeout.tv_usec = 0;
        
        int nready = select(maxfd + 1, &rset, NULL, NULL, &timeout);
        if (nready < 0) {
            if (errno == EINTR) continue; // Interrompido por sinal
            if (server_running) die("select");
            break;
        }
        
        if (nready == 0) continue; // Timeout - verifica server_running

        // Nova conexão
        if (FD_ISSET(listen_fd, &rset)) {
            struct sockaddr_in cliaddr;
            socklen_t clilen = sizeof(cliaddr);
            
            int connfd = accept(listen_fd, (struct sockaddr*)&cliaddr, &clilen);
            if (connfd < 0) {
                if (server_running) perror("accept");
            } else {
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cliaddr.sin_addr, ip, sizeof(ip));
                log_message("Novo cliente %s:%d (fd=%d)", ip, ntohs(cliaddr.sin_port), connfd);

                int i;
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] < 0) {
                        clients[i] = connfd;
                        strcpy(client_ips[i], ip);
                        break;
                    }
                }
                if (i == MAX_CLIENTS) {
                    log_message("Máximo de clientes atingido, recusando conexão");
                    close(connfd);
                } else {
                    FD_SET(connfd, &allset);
                    if (connfd > maxfd) maxfd = connfd;
                    if (i > max_i) max_i = i;
                }
            }
            if (--nready <= 0) continue;
        }

        // Processar dados dos clientes
        for (int i = 0; i <= max_i; i++) {
            int fd = clients[i];
            if (fd < 0) continue;
            
            if (FD_ISSET(fd, &rset)) {
                ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
                
                if (n <= 0) {
                    if (n < 0) perror("recv");
                    log_message("Cliente %s (fd=%d) desconectou", client_ips[i], fd);
                    close(fd);
                    FD_CLR(fd, &allset);
                    clients[i] = -1;
                    memset(client_ips[i], 0, INET_ADDRSTRLEN);
                } else {
                    buf[n] = '\0';
                    if (!process_request(fd, buf, client_ips[i])) {
                        // Cliente enviou QUIT
                        close(fd);
                        FD_CLR(fd, &allset);
                        clients[i] = -1;
                        memset(client_ips[i], 0, INET_ADDRSTRLEN);
                    }
                }
                if (--nready <= 0) break;
            }
        }
    }

    // Cleanup
    for (int i = 0; i <= max_i; i++) {
        if (clients[i] >= 0) {
            close(clients[i]);
        }
    }
    
    close(listen_fd);
    log_message("Servidor encerrado");
    return 0;
}