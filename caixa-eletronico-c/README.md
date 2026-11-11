# Sistema de Caixa Eletrônico em C

Projeto didático para praticar `struct`, `enum`, funções, ponteiros, arquivos binários, busca e atualização de registros.

## Arquivos

- `exercicio_01_caixa_eletronico.c`: solução principal do exercício. Guarda as contas em `contas.dat`.
- `desafio_01_historico_transacoes.c`: versão do desafio adicional. Além das contas, registra as movimentações em `transacoes.dat` e permite consultar o histórico pelo menu.

Cada arquivo `.c` é um programa independente: compile **um de cada vez**.

## Como compilar

Com GCC:

```bash
gcc -std=c11 -Wall -Wextra -pedantic exercicio_01_caixa_eletronico.c -o caixa
./caixa
```

Para a versão com histórico:

```bash
gcc -std=c11 -Wall -Wextra -pedantic desafio_01_historico_transacoes.c -o caixa_historico
./caixa_historico
```

No Windows, se o executável não for iniciado por `./`, execute `caixa.exe` ou `caixa_historico.exe`.

## Onde os dados são salvos?

Os arquivos binários `contas.dat` e `transacoes.dat` são criados automaticamente na pasta em que o programa é executado. Não os apague se quiser manter os dados de testes.

> Os arquivos binários gravam a representação de `struct` usada pelo compilador. São adequados para este exercício local, mas não são um formato portátil para compartilhar dados entre sistemas diferentes ou versões diferentes do programa.

## Como estudar o código sem apenas copiar

Sugestão de ordem de leitura:

1. **Leia os tipos:** entenda por que `Conta` agrupa os dados do cliente e por que `SituacaoConta` é um `enum`.
2. **Estude `buscarConta`:** observe o `FILE *`, `fread`, `rewind`, `ftell` e como a função devolve os dados pelo ponteiro `Conta *`.
3. **Estude `atualizarConta`:** `fseek` posiciona o arquivo no registro encontrado e `fwrite` grava a conta atualizada.
4. **Siga um depósito:** menu → leitura dos dados → `depositar` → busca → validação → atualização binária.
5. **Compare saque e depósito:** eles seguem etapas parecidas, mas possuem validações diferentes.
6. **Estude `transferir`:** as duas contas são validadas antes das alterações. Se a segunda gravação falhar, o código tenta restaurar os valores anteriores.
7. **Compare as versões:** no desafio, `HISTORICO_ATIVO` fica ativado. Procure `registrarTransacoes`, `Transacao` e `listarHistorico`.

## Exercícios para você praticar

Tente resolver estes pequenos passos sem pedir o código completo:

- **Nível 1:** altere uma mensagem do menu e compile novamente.
- **Nível 2:** faça a listagem mostrar também o CPF.
- **Nível 3:** adicione um filtro para listar somente contas ativas.
- **Nível 4:** impeça o cadastro de um CPF que já pertença a outra conta.
- **Nível 5:** adicione data e hora à estrutura `Transacao` e mostre-as no histórico.

Antes de alterar uma função, escreva em uma frase o que ela recebe, o que precisa verificar e o que deve devolver. Isso ajuda a construir seu raciocínio.

## Observações importantes

- O saldo inicial é zero.
- Números de conta duplicados são rejeitados.
- Contas encerradas continuam no arquivo e não podem movimentar dinheiro.
- Uma conta só pode ser encerrada com saldo zero.
- Valores de depósito, saque e transferência precisam ser maiores que zero.
- Esta é uma implementação acadêmica e local, não um sistema bancário real. A recuperação após falhas físicas de disco e a operação concorrente por múltiplos processos exigiriam mecanismos adicionais.
