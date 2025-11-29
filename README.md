# Trabalho de Banco de Dados

## Compilação

Executar `make` na raíz do projeto.

## Testes

Executar `./test_all.sh` na raíz do projeto. O script executa testes de todas as funcionalidades do programa (exceto o closure explicitamente, pois esse é usado internamente para as demais funcionalidades), utilizando arquivos na pasta `testes/`.

## Formato de Entrada (.fds)
Arquivo texto com espaços ignorados e DF's separadas por vírgula ou ponto e vírgula.

Exemplo:
```
U = {A,B,C,D,E}
F = { A ->BC, C->D, BD->E }
```

U é o universo de atributos e F é o conjunto de dependências funcionais, permitindo múltiplos atributos em LHS e RHS.

## Interface de Linha de Comando

```bash
# Fecho X+
fdtool closure --fds <arquivo.fds> --X <ATRIBUTOS>

# Cobertura minima (saida unitaria: L->A por linha)
fdtool mincover --fds <arquivo.fds>

# Chaves candidatas (todas, uma por linha)
fdtool keys --fds <arquivo.fds>

# Verificacao de formas normais (BCNF e 3FN)
fdtool normalform --fds <arquivo.fds>
```

### Saídas
closure: imprime X+ ordenado como string, p.ex., ABCDE.

minicover: uma DF por linha no formato L->A.

keys: cada chave (ordenada) em uma linha, p.ex., 
``` bash
AC 
BD
```

normalform: primeiras linhas com o status e, em caso de violações, detalhe por linha:
``` bash
BCNF: VIOLATION
3NF: VIOLATION
VIOLATION BCNF: B->C (B not superkey)
VIOLATION 3NF: B->C (C not prime, B not superkey)
```
## Estrutura do Projeto
- `src/`: código fonte em C
- `includes/`: arquivos de cabeçalho
- `testes/`: arquivos de teste .fds
- `Makefile`: script de compilação
- `test_all.sh`: script para executar todos os testes
- `README.md`: este arquivo de documentação =/