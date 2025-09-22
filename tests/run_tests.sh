#!/bin/bash

# Script de testes automatizados para o servidor de calculadora

SERVER_PORT=5051
SERVER_PID=""
TEST_RESULTS=""
PASSED=0
FAILED=0

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Função para iniciar o servidor
start_server() {
    echo -e "${YELLOW}Iniciando servidor na porta $SERVER_PORT...${NC}"
    ../server $SERVER_PORT &
    SERVER_PID=$!
    sleep 2
    
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${RED}Erro: Servidor não conseguiu iniciar${NC}"
        exit 1
    fi
    echo -e "${GREEN}Servidor iniciado (PID: $SERVER_PID)${NC}"
}

# Função para parar o servidor
stop_server() {
    if [ ! -z "$SERVER_PID" ]; then
        echo -e "${YELLOW}Parando servidor...${NC}"
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
        echo -e "${GREEN}Servidor parado${NC}"
    fi
}

# Função para executar um teste
run_test() {
    local test_name="$1"
    local input="$2"
    local expected="$3"
    
    echo -e "${BLUE}Teste: $test_name${NC}"
    
    # Executa o teste com timeout
    result=$(echo -e "$input\nQUIT" | timeout 10 ../client 127.0.0.1 $SERVER_PORT 2>/dev/null | grep -E "^(Resultado|Erro):" | head -n1)
    
    if [ "$result" = "$expected" ]; then
        echo -e "${GREEN}✓ PASSOU${NC}"
        TEST_RESULTS="$TEST_RESULTS\n${GREEN}✓${NC} $test_name"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}✗ FALHOU${NC}"
        echo "  Esperado: $expected"
        echo "  Obtido:   $result"
        TEST_RESULTS="$TEST_RESULTS\n${RED}✗${NC} $test_name"
        FAILED=$((FAILED + 1))
    fi
    echo
}

# Função para testar comandos especiais
run_special_test() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"
    
    echo -e "${BLUE}Teste: $test_name${NC}"
    
    result=$(echo -e "$input\nQUIT" | timeout 10 ../client 127.0.0.1 $SERVER_PORT 2>/dev/null | grep -v "Conectado\|Cliente encerrado\|===" | head -n1)
    
    if echo "$result" | grep -q "$expected_pattern"; then
        echo -e "${GREEN}✓ PASSOU${NC}"
        TEST_RESULTS="$TEST_RESULTS\n${GREEN}✓${NC} $test_name"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}✗ FALHOU${NC}"
        echo "  Esperado que contenha: $expected_pattern"
        echo "  Obtido: $result"
        TEST_RESULTS="$TEST_RESULTS\n${RED}✗${NC} $test_name"
        FAILED=$((FAILED + 1))
    fi
    echo
}

# Trap para cleanup
trap 'stop_server; exit' INT TERM EXIT

echo -e "${YELLOW}=== TESTES AUTOMATIZADOS - CALCULADORA SOCKET ===${NC}"
echo

# Verificar se estamos no diretório correto
if [ ! -f "../server" ] || [ ! -f "../client" ]; then
    echo -e "${RED}Erro: Executáveis não encontrados!${NC}"
    echo "Certifique-se de que está no diretório tests/ e que executou 'make' primeiro"
    exit 1
fi

# Iniciar servidor
start_server

echo -e "${YELLOW}Executando testes...${NC}"
echo

# Testes básicos
echo -e "${YELLOW}--- TESTES DE OPERAÇÕES BÁSICAS ---${NC}"
run_test "Adição simples" "ADD 10 5" "Resultado: 15.000000"
run_test "Subtração" "SUB 10 3" "Resultado: 7.000000"
run_test "Multiplicação" "MUL 4 2.5" "Resultado: 10.000000"
run_test "Divisão" "DIV 15 3" "Resultado: 5.000000"
run_test "Números negativos" "ADD -5 3" "Resultado: -2.000000"
run_test "Números decimais" "MUL 2.5 4" "Resultado: 10.000000"

# Testes de erro
echo -e "${YELLOW}--- TESTES DE TRATAMENTO DE ERROS ---${NC}"
run_test "Divisão por zero" "DIV 10 0" "Erro: EZDV divisao_por_zero"
run_test "Operação inválida" "XYZ 1 2" "Erro: EINV operacao_invalida"
run_test "Formato inválido" "ADD 1" "Erro: EINV formato_invalido"
run_test "Números inválidos" "ADD abc def" "Erro: EINV numeros_invalidos"

# Testes formato infix (bônus)
echo -e "${YELLOW}--- TESTES FORMATO INFIXA (BÔNUS) ---${NC}"
run_test "Infix adição" "10 + 5" "Resultado: 15.000000"
run_test "Infix subtração" "10 - 3" "Resultado: 7.000000"
run_test "Infix multiplicação" "4 * 2.5" "Resultado: 10.000000"
run_test "Infix divisão" "15 / 3" "Resultado: 5.000000"
run_test "Infix divisão por zero" "10 / 0" "Erro: EZDV divisao_por_zero"

# Testes de comandos especiais
echo -e "${YELLOW}--- TESTES COMANDOS ESPECIAIS ---${NC}"
run_special_test "Comando HELP" "HELP" "Comandos:"
run_special_test "Comando VERSION" "VERSION" "Server v"

# Parar servidor
stop_server

# Resumo dos resultados
echo -e "${YELLOW}=== RESUMO DOS TESTES ===${NC}"
echo -e "$TEST_RESULTS"
echo
echo -e "${BLUE}Total de testes:${NC} $((PASSED + FAILED))"
echo -e "${GREEN}Passaram:${NC} $PASSED"
echo -e "${RED}Falharam:${NC} $FAILED"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 TODOS OS TESTES PASSARAM! 🎉${NC}"
    exit 0
else
    echo -e "${RED}❌ ALGUNS TESTES FALHARAM ❌${NC}"
    exit 1
fi
