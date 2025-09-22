# Socket Math Calculator

Este projeto feito para a matéria de Computação Distribuída implementa uma aplicação cliente-servidor em C que utiliza sockets TCP (IPv4) para realizar operações matemáticas. O cliente envia operações matemáticas para o servidor, que as executa e retorna o resultado.

Autores:

Bernardo Souza Oliveira RA: 10312871
Pedro Henrique C. Guimarães RA: 10417477

## Funcionalidades Implementadas

### Requisitos Básicos ✅
- ✅ Servidor TCP (IPv4) funcional na porta configurável
- ✅ Cliente de terminal que lê do stdin
- ✅ Parsing robusto com validação
- ✅ Tratamento de erros (divisão por zero, entrada inválida)
- ✅ Formatação com ponto decimal (%.6f)
- ✅ Makefile com targets: all, server, client, clean
- ✅ Logs no servidor com timestamp
- ✅ Encerramento limpo com SIGINT (Ctrl+C)
- ✅ Parametrização de endereço/porta no cliente

### Bônus Implementados ✅
- ✅ **Concorrência**: Múltiplos clientes simultâneos via select()
- X **Testes automatizados**: Não conseguimos implementar os testes automatizados
- ✅ **Protocolo estendido**: 
  - Formato infixa (A + B, A - B, A * B, A / B)
  - Comandos HELP e VERSION

## Protocolo de Comunicação

### Formato Obrigatório (Prefixo)
```
OP A B\n
```
- `OP`: ADD, SUB, MUL, DIV
- `A, B`: números reais (ex: 2, -3.5, 10.0)

### Formato Infixa (Bônus)
```
A op B\n
```
- `op`: +, -, *, /
- Exemplo: `10 + 5`, `3.5 * 2`

### Comandos Especiais
- `HELP`: Lista comandos disponíveis
- `VERSION`: Mostra versão do servidor
- `QUIT`: Encerra conexão

### Respostas do Servidor
- Sucesso: `OK resultado\n`
- Erro: `ERR CODIGO mensagem\n`
  - `EINV`: Entrada inválida
  - `EZDV`: Divisão por zero
  - `ESRV`: Erro interno

## Compilação

```bash
make              # Compila servidor e cliente
make server       # Compila apenas servidor
make client       # Compila apenas cliente
make clean        # Remove executáveis
```

## Execução

### Servidor
```bash
./server          # Porta padrão 5050
./server 6000     # Porta personalizada
```

### Cliente
```bash
./client 127.0.0.1 5050    # Conecta ao servidor
./client localhost         # Usa porta padrão 5050
```

## Exemplos de Uso

```bash
# Terminal 1 - Servidor
./server 5050

# Terminal 2 - Cliente
./client 127.0.0.1 5050
> ADD 10 2
OK 12.000000
> 5 * 3
OK 15.000000
> DIV 10 0
ERR EZDV divisao_por_zero
> HELP
OK Comandos: ADD/SUB/MUL/DIV A B, A+/-/*/B, HELP, VERSION, QUIT
> QUIT
OK bye
```


## Estrutura do Projeto

```
socketMath/
├── src/
│   ├── server.c      # Servidor principal
│   ├── client.c      # Cliente
│   └── common.h      # Definições compartilhadas
├── Makefile          # Build system
└── README.md         # Esta documentação
```

## Decisões de Projeto

1. **Concorrência**: Usado select() para suportar múltiplos clientes simultaneamente
2. **Protocolo**: Implementados formatos prefixo e infixa para flexibilidade
3. **Logs**: Timestamp em todas as mensagens para debugging
4. **Tratamento de Sinais**: SIGINT para encerramento limpo

## Limitações Conhecidas

- Máximo de clientes simultâneos limitado por FD_SETSIZE
- Números muito grandes podem causar overflow
- Formato infixa requer exatamente um espaço entre operandos


## Autor

Implementação para atividade de Sistemas Distribuídos - Sockets TCP em C.