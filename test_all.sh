#!/bin/bash

# Script para testar os comandos do fdtool com todos os arquivos de teste

FDTOOL="./fdtool"
TEST_DIR="exemplos"

# Cores para output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

if [ ! -f "$FDTOOL" ]; then
    echo -e "${RED}Erro: $FDTOOL não encontrado. Execute 'make' primeiro.${NC}"
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}Erro: Diretório $TEST_DIR não encontrado.${NC}"
    exit 1
fi

echo -e "${BLUE}=== Testando fdtool ===${NC}\n"

for test_file in "$TEST_DIR"/*.fds; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    test_name=$(basename "$test_file")
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║ Teste: $test_name$(printf '%*s' $((52 - ${#test_name})) '')║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
    
    # Teste mincover
    echo -e "\n${GREEN}→ mincover${NC}"
    "$FDTOOL" mincover --fds "$test_file"
    
    # Teste keys
    echo -e "\n${GREEN}→ keys${NC}"
    "$FDTOOL" keys --fds "$test_file"
    
    # Teste normalform
    echo -e "\n${GREEN}→ normalform${NC}"
    "$FDTOOL" normalform --fds "$test_file"
    
    echo -e "\n"
done

echo -e "${BLUE}=== Testes concluídos ===${NC}"
