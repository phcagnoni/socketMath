#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <sys/select.h>

#define DEFAULT_PORT 5050
#define BUFFER_SIZE 1024
#define MAX_TOKENS 10
#define MAX_CLIENTS FD_SETSIZE

// Códigos de erro
#define ERR_EINV "EINV"
#define ERR_EZDV "EZDV"
#define ERR_ESRV "ESRV"

// Versão do protocolo
#define PROTOCOL_VERSION "1.0"

// Operações suportadas
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_QUIT,
    OP_HELP,
    OP_VERSION,
    OP_INVALID
} operation_t;

// Estrutura para resultado de operação
typedef struct {
    int success;
    double result;
    char error_code[10];
    char error_msg[256];
} math_result_t;

// Funções utilitárias
operation_t parse_operation(const char* op_str);
operation_t parse_infix_operation(const char* op_str);
int parse_double(const char* str, double* value);
math_result_t calculate(operation_t op, double a, double b);
void log_message(const char* format, ...);
int parse_infix_expression(const char* expr, operation_t* op, double* a, double* b);

#endif