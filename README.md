# Trabalho de Banco de Dados

## Compilação

Executar `make` no diretório raíz.

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

### Saída esperada
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

## Requisitos Algoritmicos

### Verificação de BCNF e 3FN

* Gerar cobertura mínima unitária de G.
* Calcular todas as chaves e o conjunto de atributos primos (presente em alguma chave).
* Para cada DF unitária L-> A E G não trivial (A não pertence a L):
    - Se closure(L) = U, então L é superchave (ok para BCNF e 3NF).
    - Senão, BCNF é violada. Para 3FN, está ok apenas se A for primo. se A não for primo, 3FN é violada.

obs: não é necessário enumerar todas as DFs de F+; A cobertura mínima detecta as violações usuais.

## Exemplos de uso

Fecho:
``` bash
U={A,B,C,D,E}
F={A->BC, C->D, BD->E}
```
``` bash
$ fdtool closure --fds exemplos/e1.fds --X A
ABCDE
$ fdtool closure --fds exemplos/e1.fds --X BD
BDE
```

Cobertura mínima:
``` bash
$ fdtool mincover --fds exemplos/e1.fds
A->B
A->C
C->D
BD->E
```

Chaves:
``` bash
$ fdtool keys --fds exemplos/e2.fds
AC
```

Formas normais:
``` bash
U={A,B,C}; F={ A->B, B->C }
$ fdtool normalform --fds exemplos/e3.fds
BCNF: VIOLATIONS
3NF: VIOLATIONS
VIOLATION BCNF: B->C (B not superkey)
VIOLATION 3NF: B->C (C not prime, B not superkey)
```
