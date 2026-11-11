#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

/*
 * SISTEMA DE CAIXA ELETRONICO
 * Versao didatica: contas em arquivo binario (contas.dat).
 * Na versao do desafio, as movimentacoes tambem ficam em transacoes.dat.
 */
#define ARQUIVO_CONTAS "contas.dat"
#define ARQUIVO_TRANSACOES "transacoes.dat"
#define TAM_NOME 100
#define TAM_CPF 15
#define HISTORICO_ATIVO 1 /* Esta versao registra e lista as transacoes. */

typedef enum
{
    CONTA_ENCERRADA = 0,
    CONTA_ATIVA = 1
} SituacaoConta;

typedef struct
{
    long numero;
    char titular[TAM_NOME];
    char cpf[TAM_CPF];
    double saldo;
    SituacaoConta situacao;
} Conta;

typedef enum
{
    DEPOSITO = 0,
    SAQUE = 1,
    TRANSFERENCIA_ENVIADA = 2,
    TRANSFERENCIA_RECEBIDA = 3
} TipoTransacao;

typedef struct
{
    long conta;
    long contaRelacionada;
    TipoTransacao tipo;
    double valor;
} Transacao;

/* Evita registrar saque/deposito separados durante uma transferencia. */
static int transferenciaEmAndamento = 0;

/* Le uma linha inteira e remove o caractere de quebra de linha. */
static int lerLinha(const char *mensagem, char *destino, size_t tamanho)
{
    int caractere;
    size_t comprimento;

    printf("%s", mensagem);
    if (fgets(destino, (int)tamanho, stdin) == NULL)
    {
        return 0;
    }

    comprimento = strlen(destino);
    if (comprimento > 0 && destino[comprimento - 1] == '\n')
    {
        destino[comprimento - 1] = '\0';
    }
    else
    {
        /* Descarta o restante se a entrada for maior que o buffer. */
        while ((caractere = getchar()) != '\n' && caractere != EOF)
        {
        }
    }
    return 1;
}

static int lerLong(const char *mensagem, long *valor)
{
    char linha[128];
    char *fim;
    long lido;

    if (!lerLinha(mensagem, linha, sizeof(linha)))
        return 0;
    errno = 0;
    lido = strtol(linha, &fim, 10);
    if (linha[0] == '\0' || *fim != '\0' || errno == ERANGE)
    {
        printf("Entrada invalida. Digite um numero inteiro.\n");
        return -1;
    }
    *valor = lido;
    return 1;
}

static int lerDouble(const char *mensagem, double *valor)
{
    char linha[128];
    char *fim;
    double lido;

    if (!lerLinha(mensagem, linha, sizeof(linha)))
        return 0;
    errno = 0;
    lido = strtod(linha, &fim);
    if (linha[0] == '\0' || *fim != '\0' || errno == ERANGE || !isfinite(lido))
    {
        printf("Entrada invalida. Digite um valor numerico.\n");
        return -1;
    }
    *valor = lido;
    return 1;
}

static int lerTextoObrigatorio(const char *mensagem, char *destino, size_t tamanho)
{
    if (!lerLinha(mensagem, destino, tamanho))
        return 0;
    if (destino[0] == '\0')
    {
        printf("Este campo nao pode ficar vazio.\n");
        return -1;
    }
    return 1;
}

/* Abre um arquivo de contas existente ou cria um novo sem apagar dados. */
static FILE *abrirArquivoContas(void)
{
    FILE *arquivo = fopen(ARQUIVO_CONTAS, "r+b");
    if (arquivo == NULL && errno == ENOENT)
    {
        arquivo = fopen(ARQUIVO_CONTAS, "w+b");
    }
    if (arquivo == NULL)
    {
        perror("Nao foi possivel abrir o arquivo de contas");
    }
    return arquivo;
}

/*
 * Procura um registro pelo numero.
 * Se encontrar, retorna 1 e informa o registro e sua posicao no arquivo.
 */
static int buscarConta(FILE *arquivo, long numero, Conta *conta, long *posicao)
{
    Conta atual;
    long inicio;

    if (arquivo == NULL)
        return 0;
    rewind(arquivo);
    while ((inicio = ftell(arquivo)) >= 0 && fread(&atual, sizeof(Conta), 1, arquivo) == 1)
    {
        if (atual.numero == numero)
        {
            if (conta != NULL)
                *conta = atual;
            if (posicao != NULL)
                *posicao = inicio;
            return 1;
        }
    }
    return 0;
}

/* Atualiza um registro na posicao informada, sem criar outro registro. */
static int atualizarConta(FILE *arquivo, const Conta *conta, long posicao)
{
    if (fseek(arquivo, posicao, SEEK_SET) != 0)
        return 0;
    if (fwrite(conta, sizeof(Conta), 1, arquivo) != 1)
        return 0;
    return fflush(arquivo) == 0;
}

#if HISTORICO_ATIVO
/* Acrescenta uma ou mais movimentacoes ao final do arquivo de historico. */
static int registrarTransacoes(const Transacao *transacoes, size_t quantidade)
{
    FILE *arquivo = fopen(ARQUIVO_TRANSACOES, "ab");
    int sucesso;
    if (arquivo == NULL)
    {
        perror("Aviso: nao foi possivel abrir o historico");
        return 0;
    }
    sucesso = fwrite(transacoes, sizeof(Transacao), quantidade, arquivo) == quantidade;
    if (fflush(arquivo) != 0)
        sucesso = 0;
    if (fclose(arquivo) != 0)
        sucesso = 0;
    return sucesso;
}

static const char *nomeTipoTransacao(TipoTransacao tipo)
{
    switch (tipo)
    {
    case DEPOSITO:
        return "DEPOSITO";
    case SAQUE:
        return "SAQUE";
    case TRANSFERENCIA_ENVIADA:
        return "TRANSFERENCIA ENVIADA";
    case TRANSFERENCIA_RECEBIDA:
        return "TRANSFERENCIA RECEBIDA";
    default:
        return "DESCONHECIDA";
    }
}

static void listarHistorico(void)
{
    FILE *arquivo = fopen(ARQUIVO_TRANSACOES, "rb");
    Transacao transacao;
    int encontrou = 0;

    if (arquivo == NULL)
    {
        if (errno == ENOENT)
            printf("Ainda nao ha movimentacoes registradas.\n");
        else
            perror("Erro ao abrir o historico");
        return;
    }

    printf("\n--- HISTORICO DE TRANSACOES ---\n");
    while (fread(&transacao, sizeof(Transacao), 1, arquivo) == 1)
    {
        encontrou = 1;
        printf("Conta: %ld | Tipo: %s | Valor: R$ %.2f",
               transacao.conta, nomeTipoTransacao(transacao.tipo), transacao.valor);
        if (transacao.contaRelacionada != 0)
        {
            printf(" | Conta relacionada: %ld", transacao.contaRelacionada);
        }
        printf("\n");
    }
    if (ferror(arquivo))
        perror("Erro durante a leitura do historico");
    if (!encontrou)
        printf("Ainda nao ha movimentacoes registradas.\n");
    fclose(arquivo);
}
#endif

static void imprimirConta(const Conta *conta)
{
    printf("\nNumero: %ld\n", conta->numero);
    printf("Titular: %s\n", conta->titular);
    printf("CPF: %s\n", conta->cpf);
    printf("Saldo: R$ %.2f\n", conta->saldo);
    printf("Situacao: %s\n", conta->situacao == CONTA_ATIVA ? "ATIVA" : "ENCERRADA");
}

static int cadastrarConta(void)
{
    FILE *arquivo = abrirArquivoContas();
    Conta conta;
    long numero;
    int resultado;

    if (arquivo == NULL)
        return 0;
    resultado = lerLong("Numero da nova conta: ", &numero);
    if (resultado <= 0)
    {
        fclose(arquivo);
        return 0;
    }
    if (numero <= 0)
    {
        printf("O numero da conta deve ser maior que zero.\n");
        fclose(arquivo);
        return 0;
    }
    if (buscarConta(arquivo, numero, NULL, NULL))
    {
        printf("Ja existe uma conta com esse numero.\n");
        fclose(arquivo);
        return 0;
    }

    memset(&conta, 0, sizeof(conta));
    conta.numero = numero;
    resultado = lerTextoObrigatorio("Nome do titular: ", conta.titular, sizeof(conta.titular));
    if (resultado <= 0)
    {
        fclose(arquivo);
        return 0;
    }
    resultado = lerTextoObrigatorio("CPF: ", conta.cpf, sizeof(conta.cpf));
    if (resultado <= 0)
    {
        fclose(arquivo);
        return 0;
    }
    conta.saldo = 0.0;
    conta.situacao = CONTA_ATIVA;

    if (fseek(arquivo, 0, SEEK_END) != 0 || fwrite(&conta, sizeof(conta), 1, arquivo) != 1 || fflush(arquivo) != 0)
    {
        perror("Erro ao gravar a conta");
        fclose(arquivo);
        return 0;
    }
    fclose(arquivo);
    printf("Conta cadastrada com sucesso! Saldo inicial: R$ 0,00.\n");
    return 1;
}

static void consultarConta(void)
{
    FILE *arquivo = abrirArquivoContas();
    Conta conta;
    long numero;
    int resultado;

    if (arquivo == NULL)
        return;
    resultado = lerLong("Numero da conta para consulta: ", &numero);
    if (resultado <= 0)
    {
        fclose(arquivo);
        return;
    }
    if (buscarConta(arquivo, numero, &conta, NULL))
        imprimirConta(&conta);
    else
        printf("Conta nao encontrada.\n");
    fclose(arquivo);
}

static int depositar(long numero, double valor)
{
    FILE *arquivo;
    Conta conta, original;
    long posicao;
    int sucesso;
#if HISTORICO_ATIVO
    Transacao transacao;
#endif

    if (valor <= 0.0 || !isfinite(valor))
    {
        printf("O deposito deve ser maior que zero.\n");
        return 0;
    }
    arquivo = abrirArquivoContas();
    if (arquivo == NULL)
        return 0;
    if (!buscarConta(arquivo, numero, &conta, &posicao))
    {
        printf("Conta nao encontrada.\n");
        fclose(arquivo);
        return 0;
    }
    if (conta.situacao != CONTA_ATIVA)
    {
        printf("Nao e permitido movimentar uma conta encerrada.\n");
        fclose(arquivo);
        return 0;
    }
    original = conta;
    conta.saldo += valor;
    if (!isfinite(conta.saldo))
    {
        printf("Operacao invalida: o saldo excederia o limite numerico.\n");
        fclose(arquivo);
        return 0;
    }
    sucesso = atualizarConta(arquivo, &conta, posicao);
    if (!sucesso)
    {
        perror("Erro ao atualizar o saldo");
        (void)atualizarConta(arquivo, &original, posicao);
        fclose(arquivo);
        return 0;
    }
#if HISTORICO_ATIVO
    if (!transferenciaEmAndamento)
    {
        transacao.conta = numero;
        transacao.contaRelacionada = 0;
        transacao.tipo = DEPOSITO;
        transacao.valor = valor;
        if (!registrarTransacoes(&transacao, 1))
        {
            printf("Falha ao registrar o historico; tentando cancelar o deposito.\n");
            (void)atualizarConta(arquivo, &original, posicao);
            fclose(arquivo);
            return 0;
        }
    }
#endif
    fclose(arquivo);
    if (!transferenciaEmAndamento)
    {
        printf("Deposito realizado. Novo saldo: R$ %.2f\n", conta.saldo);
    }
    return 1;
}

static int sacar(long numero, double valor)
{
    FILE *arquivo;
    Conta conta, original;
    long posicao;
#if HISTORICO_ATIVO
    Transacao transacao;
#endif

    if (valor <= 0.0 || !isfinite(valor))
    {
        printf("O saque deve ser maior que zero.\n");
        return 0;
    }
    arquivo = abrirArquivoContas();
    if (arquivo == NULL)
        return 0;
    if (!buscarConta(arquivo, numero, &conta, &posicao))
    {
        printf("Conta nao encontrada.\n");
        fclose(arquivo);
        return 0;
    }
    if (conta.situacao != CONTA_ATIVA)
    {
        printf("Nao e permitido movimentar uma conta encerrada.\n");
        fclose(arquivo);
        return 0;
    }
    if (conta.saldo < valor)
    {
        printf("Saldo insuficiente. Saldo atual: R$ %.2f\n", conta.saldo);
        fclose(arquivo);
        return 0;
    }
    original = conta;
    conta.saldo -= valor;
    if (!atualizarConta(arquivo, &conta, posicao))
    {
        perror("Erro ao atualizar o saldo");
        (void)atualizarConta(arquivo, &original, posicao);
        fclose(arquivo);
        return 0;
    }
#if HISTORICO_ATIVO
    if (!transferenciaEmAndamento)
    {
        transacao.conta = numero;
        transacao.contaRelacionada = 0;
        transacao.tipo = SAQUE;
        transacao.valor = valor;
        if (!registrarTransacoes(&transacao, 1))
        {
            printf("Falha ao registrar o historico; tentando cancelar o saque.\n");
            (void)atualizarConta(arquivo, &original, posicao);
            fclose(arquivo);
            return 0;
        }
    }
#endif
    fclose(arquivo);
    if (!transferenciaEmAndamento)
    {
        printf("Saque realizado. Novo saldo: R$ %.2f\n", conta.saldo);
    }
    return 1;
}

static int transferir(long origemNumero, long destinoNumero, double valor)
{
    FILE *arquivo;
    Conta origem, destino;
    int saqueConcluido;
    int depositoConcluido;
#if HISTORICO_ATIVO
    Transacao transacoes[2];
#endif

    if (origemNumero == destinoNumero)
    {
        printf("A conta de origem e a de destino devem ser diferentes.\n");
        return 0;
    }
    if (valor <= 0.0 || !isfinite(valor))
    {
        printf("A transferencia deve ser maior que zero.\n");
        return 0;
    }

    /* Primeiro validamos as duas contas, antes de movimentar qualquer saldo. */
    arquivo = abrirArquivoContas();
    if (arquivo == NULL)
        return 0;
    if (!buscarConta(arquivo, origemNumero, &origem, NULL))
    {
        printf("Conta de origem nao encontrada.\n");
        fclose(arquivo);
        return 0;
    }
    if (!buscarConta(arquivo, destinoNumero, &destino, NULL))
    {
        printf("Conta de destino nao encontrada.\n");
        fclose(arquivo);
        return 0;
    }
    if (origem.situacao != CONTA_ATIVA || destino.situacao != CONTA_ATIVA)
    {
        printf("As duas contas precisam estar ativas.\n");
        fclose(arquivo);
        return 0;
    }
    if (origem.saldo < valor)
    {
        printf("Saldo insuficiente na conta de origem.\n");
        fclose(arquivo);
        return 0;
    }
    if (!isfinite(destino.saldo + valor))
    {
        printf("O saldo de destino excederia o limite numerico.\n");
        fclose(arquivo);
        return 0;
    }
    fclose(arquivo);

    /* A transferencia e composta pelas funcoes existentes sacar() e depositar(). */
    transferenciaEmAndamento = 1;
    saqueConcluido = sacar(origemNumero, valor);
    if (!saqueConcluido)
    {
        transferenciaEmAndamento = 0;
        printf("Transferencia cancelada antes do deposito.\n");
        return 0;
    }

    depositoConcluido = depositar(destinoNumero, valor);
    if (!depositoConcluido)
    {
        /* Compensacao: devolve o valor a origem se o deposito falhar. */
        if (!depositar(origemNumero, valor))
        {
            printf("ERRO: nao foi possivel devolver o valor a conta de origem.\n");
        }
        transferenciaEmAndamento = 0;
        printf("O deposito falhou; a transferencia foi cancelada.\n");
        return 0;
    }
    transferenciaEmAndamento = 0;

#if HISTORICO_ATIVO
    /* Uma transferencia gera dois registros relacionados no historico. */
    transacoes[0].conta = origemNumero;
    transacoes[0].contaRelacionada = destinoNumero;
    transacoes[0].tipo = TRANSFERENCIA_ENVIADA;
    transacoes[0].valor = valor;
    transacoes[1].conta = destinoNumero;
    transacoes[1].contaRelacionada = origemNumero;
    transacoes[1].tipo = TRANSFERENCIA_RECEBIDA;
    transacoes[1].valor = valor;
    if (!registrarTransacoes(transacoes, 2))
    {
        printf("Falha ao gravar historico; tentando reverter a transferencia.\n");
        transferenciaEmAndamento = 1;
        {
            int saqueReversao = sacar(destinoNumero, valor);
            int depositoReversao = depositar(origemNumero, valor);
            if (!saqueReversao || !depositoReversao)
            {
                printf("ERRO: a reversao nao foi concluida completamente. Confira os saldos.\n");
            }
        }
        transferenciaEmAndamento = 0;
        return 0;
    }
#endif

    printf("Transferencia realizada com sucesso: R$ %.2f da conta %ld para a conta %ld.\n",
           valor, origemNumero, destinoNumero);
    return 1;
}

static void listarContas(void)
{
    FILE *arquivo = abrirArquivoContas();
    Conta conta;
    int encontrou = 0;

    if (arquivo == NULL)
        return;
    rewind(arquivo);
    printf("\n--- CONTAS CADASTRADAS ---\n");
    while (fread(&conta, sizeof(Conta), 1, arquivo) == 1)
    {
        encontrou = 1;
        printf("Conta: %ld | Titular: %s | Saldo: R$ %.2f | Situacao: %s\n",
               conta.numero, conta.titular, conta.saldo,
               conta.situacao == CONTA_ATIVA ? "ATIVA" : "ENCERRADA");
    }
    if (ferror(arquivo))
        perror("Erro durante a leitura das contas");
    if (!encontrou)
        printf("Nenhuma conta cadastrada.\n");
    fclose(arquivo);
}

static int encerrarConta(long numero)
{
    FILE *arquivo = abrirArquivoContas();
    Conta conta;
    long posicao;

    if (arquivo == NULL)
        return 0;
    if (!buscarConta(arquivo, numero, &conta, &posicao))
    {
        printf("Conta nao encontrada.\n");
        fclose(arquivo);
        return 0;
    }
    if (conta.situacao == CONTA_ENCERRADA)
    {
        printf("Esta conta ja esta encerrada.\n");
        fclose(arquivo);
        return 0;
    }
    if (fabs(conta.saldo) > 0.0000001)
    {
        printf("A conta so pode ser encerrada quando o saldo for zero.\n");
        fclose(arquivo);
        return 0;
    }
    conta.situacao = CONTA_ENCERRADA;
    if (!atualizarConta(arquivo, &conta, posicao))
    {
        perror("Erro ao encerrar a conta");
        fclose(arquivo);
        return 0;
    }
    fclose(arquivo);
    printf("Conta encerrada. O registro foi mantido no arquivo.\n");
    return 1;
}

static void mostrarMenu(void)
{
    printf("\n========== CAIXA ELETRONICO ==========\n");
    printf("1 - Cadastrar conta\n");
    printf("2 - Consultar conta\n");
    printf("3 - Realizar deposito\n");
    printf("4 - Realizar saque\n");
    printf("5 - Realizar transferencia\n");
    printf("6 - Listar contas\n");
    printf("7 - Encerrar conta\n");
#if HISTORICO_ATIVO
    printf("8 - Consultar historico de transacoes\n");
#endif
    printf("0 - Sair\n");
    printf("Escolha uma opcao: ");
}

int main(void)
{
    long opcao, numero, origem, destino;
    double valor;
    int resultado;

    for (;;)
    {
        mostrarMenu();
        resultado = lerLong("", &opcao);
        if (resultado == 0)
        {
            printf("\nEntrada encerrada. Finalizando o programa.\n");
            break;
        }
        if (resultado < 0)
            continue;

        switch (opcao)
        {
        case 1:
            (void)cadastrarConta();
            break;
        case 2:
            consultarConta();
            break;
        case 3:
            resultado = lerLong("Numero da conta: ", &numero);
            if (resultado <= 0)
                break;
            resultado = lerDouble("Valor do deposito: R$ ", &valor);
            if (resultado <= 0)
                break;
            (void)depositar(numero, valor);
            break;
        case 4:
            resultado = lerLong("Numero da conta: ", &numero);
            if (resultado <= 0)
                break;
            resultado = lerDouble("Valor do saque: R$ ", &valor);
            if (resultado <= 0)
                break;
            (void)sacar(numero, valor);
            break;
        case 5:
            resultado = lerLong("Conta de origem: ", &origem);
            if (resultado <= 0)
                break;
            resultado = lerLong("Conta de destino: ", &destino);
            if (resultado <= 0)
                break;
            resultado = lerDouble("Valor da transferencia: R$ ", &valor);
            if (resultado <= 0)
                break;
            (void)transferir(origem, destino, valor);
            break;
        case 6:
            listarContas();
            break;
        case 7:
            resultado = lerLong("Numero da conta para encerrar: ", &numero);
            if (resultado <= 0)
                break;
            (void)encerrarConta(numero);
            break;
#if HISTORICO_ATIVO
        case 8:
            listarHistorico();
            break;
#endif
        case 0:
            printf("Programa encerrado.\n");
            return 0;
        default:
            printf("Opcao inexistente. Tente novamente.\n");
        }
    }
    return 0;
}
