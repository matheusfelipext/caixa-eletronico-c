# Caixa Eletrônico em C

Sistema de caixa eletrônico em linguagem C, executado no terminal, com persistência em **arquivos binários**.

## Funcionalidades

- Cadastrar conta (número, titular e CPF)
- Consultar conta
- Depósito, saque e transferência entre contas
- Listar contas (todas, somente ativas ou somente encerradas)
- Extrato por conta
- Encerrar e reativar conta (só encerra com saldo zerado)
- Excluir definitivamente as contas encerradas (admin)

## Como compilar e executar

Requer o `gcc` (Linux, macOS, ou MinGW/WSL no Windows).

```bash
gcc -Wall -Wextra -o caixa caixa.c
./caixa
```

No Windows (MinGW), execute `caixa.exe`.

## Menu

```
===== CAIXA ELETRONICO =====
 1 - Cadastrar conta
 2 - Consultar conta
 3 - Realizar deposito
 4 - Realizar saque
 5 - Realizar transferencia
 6 - Listar contas
 7 - Encerrar conta
 8 - Extrato da conta
 9 - Reativar conta
10 - Excluir contas encerradas (admin)
 0 - Sair
```

## Arquivos gerados

Os arquivos são criados automaticamente na pasta onde o programa é executado:

| Arquivo | Conteúdo | Tamanho de cada registro |
|---|---|---|
| `contas.dat` | Contas cadastradas (struct `Conta`) | 144 bytes |
| `transacoes.dat` | Histórico de movimentações (struct `Transacao`) | 32 bytes |
| `contas.tmp` | Temporário, usado só na exclusão de contas encerradas | - |

O `contas.dat` só existe depois do primeiro cadastro, e o `transacoes.dat` depois da primeira movimentação.

> Esses arquivos contêm dados pessoais (CPF), por isso estão no `.gitignore` e **não devem ser enviados ao GitHub**.

## Regras de negócio

- Número da conta deve ser maior que zero e único.
- Valores de depósito, saque e transferência devem ser maiores que zero.
- Saque e transferência exigem saldo suficiente.
- Contas encerradas não movimentam (nem recebem nem enviam).
- Uma conta só pode ser encerrada com saldo igual a zero.
- Ao excluir uma conta encerrada, suas transações também são removidas.

## Conceitos usados

`struct`, `enum`, ponteiros, funções, manipulação de arquivos binários (`fopen`, `fread`, `fwrite`, `fseek`, `rewind`, `remove`, `rename`) e validação de entrada.
