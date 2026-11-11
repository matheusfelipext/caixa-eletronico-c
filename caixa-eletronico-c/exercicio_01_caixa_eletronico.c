#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQ_CONTAS          "contas.dat"
#define ARQ_TRANSACOES      "transacoes.dat"
#define ARQ_TEMP            "contas.tmp"
#define ARQ_TEMP_TRANSACOES "transacoes.tmp"

/* Os valores sao em reais com centavos; abaixo disso consideramos zero. */
#define TOLERANCIA 0.005

typedef enum
{
    CONTA_ENCERRADA,
    CONTA_ATIVA
} SituacaoConta;

typedef struct
{
    long numero;
    char titular[100];
    char cpf[15];
    double saldo;
    SituacaoConta situacao;
} Conta;

typedef enum
{
    DEPOSITO,
    SAQUE,
    TRANSFERENCIA_ENVIADA,
    TRANSFERENCIA_RECEBIDA
} TipoTransacao;

typedef struct
{
    long conta;
    long contaRelacionada;
    TipoTransacao tipo;
    double valor;
} Transacao;

static void limparBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* Le uma linha. Se a entrada acabar (EOF), encerra o programa. */
static void lerLinha(char *destino, size_t tamanho)
{
    char *quebra;

    if (fgets(destino, (int)tamanho, stdin) == NULL)
    {
        printf("\nEntrada encerrada.\n");
        exit(0);
    }

    quebra = strchr(destino, '\n');
    if (quebra != NULL)
        *quebra = '\0';
    else
        limparBuffer();
}

static long lerLong(void)
{
    char linha[64];
    char *fim;
    long valor;

    lerLinha(linha, sizeof(linha));
    valor = strtol(linha, &fim, 10);

    if (fim == linha || *fim != '\0')
        return -1;
    return valor;
}

static double lerDouble(void)
{
    char linha[64];
    char *fim;
    double valor;
    int i;

    lerLinha(linha, sizeof(linha));
    for (i = 0; linha[i] != '\0'; i++)
    {
        if (linha[i] == ',')
            linha[i] = '.';
    }

    valor = strtod(linha, &fim);
    if (fim == linha || *fim != '\0')
        return -1.0;
    if (valor != valor || valor > 1e12) /* NaN ou valor absurdo */
        return -1.0;
    return valor;
}

static const char *nomeSituacao(SituacaoConta s)
{
    return (s == CONTA_ATIVA) ? "ATIVA" : "ENCERRADA";
}

static const char *nomeTipoTransacao(TipoTransacao t)
{
    switch (t)
    {
        case DEPOSITO:               return "DEPOSITO";
        case SAQUE:                  return "SAQUE";
        case TRANSFERENCIA_ENVIADA:  return "TRANSF. ENVIADA";
        case TRANSFERENCIA_RECEBIDA: return "TRANSF. RECEBIDA";
        default:                     return "DESCONHECIDO";
    }
}

int buscarConta(FILE *arquivo, long numeroConta, Conta *conta)
{
    Conta c;

    rewind(arquivo);
    while (fread(&c, sizeof(Conta), 1, arquivo) == 1)
    {
        if (c.numero == numeroConta)
        {
            *conta = c;
            fseek(arquivo, -(long)sizeof(Conta), SEEK_CUR);
            return 1;
        }
    }
    return 0;
}

int atualizarConta(FILE *arquivo, const Conta *conta)
{
    Conta existente;

    if (!buscarConta(arquivo, conta->numero, &existente))
        return 0;

    if (fwrite(conta, sizeof(Conta), 1, arquivo) != 1)
        return 0;

    fflush(arquivo);
    return 1;
}

static int registrarTransacao(long conta, long relacionada,
                              TipoTransacao tipo, double valor)
{
    Transacao t;
    int ok;
    FILE *arquivo = fopen(ARQ_TRANSACOES, "ab");

    if (arquivo == NULL)
        return 0;

    t.conta = conta;
    t.contaRelacionada = relacionada;
    t.tipo = tipo;
    t.valor = valor;

    ok = (fwrite(&t, sizeof(Transacao), 1, arquivo) == 1);
    fclose(arquivo);
    return ok;
}

void cadastrarConta(void)
{
    Conta nova, existente;
    FILE *arquivo;

    memset(&nova, 0, sizeof(nova));

    printf("Numero da conta: ");
    nova.numero = lerLong();
    if (nova.numero <= 0)
    {
        printf("Numero de conta invalido.\n");
        return;
    }

    arquivo = fopen(ARQ_CONTAS, "rb");
    if (arquivo != NULL)
    {
        int existe = buscarConta(arquivo, nova.numero, &existente);
        fclose(arquivo);
        if (existe)
        {
            printf("Ja existe uma conta com o numero %ld.\n", nova.numero);
            return;
        }
    }

    printf("Nome do titular: ");
    lerLinha(nova.titular, sizeof(nova.titular));
    if (nova.titular[0] == '\0')
    {
        printf("Nome invalido.\n");
        return;
    }

    printf("CPF: ");
    lerLinha(nova.cpf, sizeof(nova.cpf));
    if (nova.cpf[0] == '\0')
    {
        printf("CPF invalido.\n");
        return;
    }

    nova.saldo = 0.0;
    nova.situacao = CONTA_ATIVA;

    arquivo = fopen(ARQ_CONTAS, "ab");
    if (arquivo == NULL)
    {
        printf("Erro ao abrir o arquivo de contas.\n");
        return;
    }

    if (fwrite(&nova, sizeof(Conta), 1, arquivo) != 1)
        printf("Erro ao gravar a conta.\n");
    else
        printf("Conta %ld cadastrada com sucesso.\n", nova.numero);

    fclose(arquivo);
}

static void exibirConta(const Conta *c)
{
    printf("\nNumero:   %ld\n", c->numero);
    printf("Titular:  %s\n", c->titular);
    printf("CPF:      %s\n", c->cpf);
    printf("Saldo:    R$ %.2f\n", c->saldo);
    printf("Situacao: %s\n", nomeSituacao(c->situacao));
}

void consultarConta(void)
{
    Conta c;
    long numero;
    FILE *arquivo;

    printf("Numero da conta: ");
    numero = lerLong();

    arquivo = fopen(ARQ_CONTAS, "rb");
    if (arquivo == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return;
    }

    if (buscarConta(arquivo, numero, &c))
        exibirConta(&c);
    else
        printf("Conta %ld nao encontrada.\n", numero);

    fclose(arquivo);
}

static int depositarSaldo(long numeroConta, double valor)
{
    Conta c;
    FILE *arquivo;

    if (valor <= 0)
    {
        printf("Valor deve ser maior que zero.\n");
        return 0;
    }

    arquivo = fopen(ARQ_CONTAS, "rb+");
    if (arquivo == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return 0;
    }

    if (!buscarConta(arquivo, numeroConta, &c))
    {
        printf("Conta %ld nao encontrada.\n", numeroConta);
        fclose(arquivo);
        return 0;
    }

    if (c.situacao != CONTA_ATIVA)
    {
        printf("Conta %ld esta encerrada.\n", numeroConta);
        fclose(arquivo);
        return 0;
    }

    c.saldo += valor;

    if (!atualizarConta(arquivo, &c))
    {
        printf("Erro ao gravar a atualizacao.\n");
        fclose(arquivo);
        return 0;
    }

    fclose(arquivo);
    return 1;
}

static int sacarSaldo(long numeroConta, double valor)
{
    Conta c;
    FILE *arquivo;

    if (valor <= 0)
    {
        printf("Valor deve ser maior que zero.\n");
        return 0;
    }

    arquivo = fopen(ARQ_CONTAS, "rb+");
    if (arquivo == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return 0;
    }

    if (!buscarConta(arquivo, numeroConta, &c))
    {
        printf("Conta %ld nao encontrada.\n", numeroConta);
        fclose(arquivo);
        return 0;
    }

    if (c.situacao != CONTA_ATIVA)
    {
        printf("Conta %ld esta encerrada.\n", numeroConta);
        fclose(arquivo);
        return 0;
    }

    if (valor - c.saldo > TOLERANCIA)
    {
        printf("Saldo insuficiente (saldo: R$ %.2f).\n", c.saldo);
        fclose(arquivo);
        return 0;
    }

    c.saldo -= valor;
    if (c.saldo < TOLERANCIA && c.saldo > -TOLERANCIA)
        c.saldo = 0.0;

    if (!atualizarConta(arquivo, &c))
    {
        printf("Erro ao gravar a atualizacao.\n");
        fclose(arquivo);
        return 0;
    }

    fclose(arquivo);
    return 1;
}

int depositar(long numeroConta, double valor)
{
    if (!depositarSaldo(numeroConta, valor))
        return 0;

    if (!registrarTransacao(numeroConta, 0, DEPOSITO, valor))
        printf("Aviso: nao foi possivel gravar no historico.\n");
    return 1;
}

int sacar(long numeroConta, double valor)
{
    if (!sacarSaldo(numeroConta, valor))
        return 0;

    if (!registrarTransacao(numeroConta, 0, SAQUE, valor))
        printf("Aviso: nao foi possivel gravar no historico.\n");
    return 1;
}

int transferir(long contaOrigem, long contaDestino, double valor)
{
    Conta origem, destino;
    FILE *arquivo;

    if (contaOrigem == contaDestino)
    {
        printf("Conta de origem e destino devem ser diferentes.\n");
        return 0;
    }

    if (valor <= 0)
    {
        printf("Valor deve ser maior que zero.\n");
        return 0;
    }

    arquivo = fopen(ARQ_CONTAS, "rb");
    if (arquivo == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return 0;
    }

    if (!buscarConta(arquivo, contaOrigem, &origem))
    {
        printf("Conta de origem %ld nao encontrada.\n", contaOrigem);
        fclose(arquivo);
        return 0;
    }

    if (!buscarConta(arquivo, contaDestino, &destino))
    {
        printf("Conta de destino %ld nao encontrada.\n", contaDestino);
        fclose(arquivo);
        return 0;
    }

    fclose(arquivo);

    if (origem.situacao != CONTA_ATIVA)
    {
        printf("Conta de origem esta encerrada.\n");
        return 0;
    }

    if (destino.situacao != CONTA_ATIVA)
    {
        printf("Conta de destino esta encerrada.\n");
        return 0;
    }

    if (valor - origem.saldo > TOLERANCIA)
    {
        printf("Saldo insuficiente na origem (saldo: R$ %.2f).\n", origem.saldo);
        return 0;
    }

    if (!sacarSaldo(contaOrigem, valor))
        return 0;

    if (!depositarSaldo(contaDestino, valor))
    {
        printf("Falha no deposito. Estornando saque...\n");
        if (!depositarSaldo(contaOrigem, valor))
            printf("ERRO GRAVE: nao foi possivel estornar. Confira a conta %ld.\n",
                   contaOrigem);
        return 0;
    }

    /* Historico: so registra depois que as duas etapas deram certo */
    if (!registrarTransacao(contaOrigem, contaDestino, TRANSFERENCIA_ENVIADA, valor) ||
        !registrarTransacao(contaDestino, contaOrigem, TRANSFERENCIA_RECEBIDA, valor))
        printf("Aviso: nao foi possivel gravar no historico.\n");

    return 1;
}

static void listarContasFiltro(int filtro)
{
    Conta c;
    int total = 0;
    FILE *arquivo = fopen(ARQ_CONTAS, "rb");

    if (arquivo == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return;
    }

    printf("\n%-10s %-30s %-15s %-14s %s\n",
           "Numero", "Titular", "CPF", "Saldo", "Situacao");
    printf("--------------------------------------------------------------------------------\n");

    while (fread(&c, sizeof(Conta), 1, arquivo) == 1)
    {
        if (filtro == 1 && c.situacao != CONTA_ATIVA)     continue;
        if (filtro == 2 && c.situacao != CONTA_ENCERRADA) continue;

        printf("%-10ld %-30.30s %-15s R$ %-11.2f %s\n",
               c.numero, c.titular, c.cpf, c.saldo, nomeSituacao(c.situacao));
        total++;
    }

    if (total == 0)
        printf("(nenhuma conta nesse filtro)\n");
    else
        printf("\nTotal: %d conta(s)\n", total);

    fclose(arquivo);
}

void listarContas(void)
{
    long opcao;

    printf("\n1 - Todas\n2 - Somente ativas\n3 - Somente encerradas\nFiltro: ");
    opcao = lerLong();

    switch (opcao)
    {
        case 1: listarContasFiltro(0); break;
        case 2: listarContasFiltro(1); break;
        case 3: listarContasFiltro(2); break;
        default: printf("Filtro invalido.\n");
    }
}

int encerrarConta(long numeroConta)
{
    Conta c;
    FILE *arquivo = fopen(ARQ_CONTAS, "rb+");

    if (arquivo == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return 0;
    }

    if (!buscarConta(arquivo, numeroConta, &c))
    {
        printf("Conta %ld nao encontrada.\n", numeroConta);
        fclose(arquivo);
        return 0;
    }

    if (c.situacao == CONTA_ENCERRADA)
    {
        printf("Conta %ld ja esta encerrada.\n", numeroConta);
        fclose(arquivo);
        return 0;
    }

    if (c.saldo > TOLERANCIA || c.saldo < -TOLERANCIA)
    {
        printf("Conta possui saldo de R$ %.2f. Zere o saldo antes de encerrar.\n", c.saldo);
        fclose(arquivo);
        return 0;
    }

    c.situacao = CONTA_ENCERRADA;

    if (!atualizarConta(arquivo, &c))
    {
        printf("Erro ao gravar a atualizacao.\n");
        fclose(arquivo);
        return 0;
    }

    fclose(arquivo);
    printf("Conta %ld encerrada.\n", numeroConta);
    return 1;
}

int reativarConta(long numeroConta)
{
    Conta c;
    FILE *arquivo = fopen(ARQ_CONTAS, "rb+");

    if (arquivo == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return 0;
    }

    if (!buscarConta(arquivo, numeroConta, &c))
    {
        printf("Conta %ld nao encontrada.\n", numeroConta);
        fclose(arquivo);
        return 0;
    }

    if (c.situacao == CONTA_ATIVA)
    {
        printf("Conta %ld ja esta ativa.\n", numeroConta);
        fclose(arquivo);
        return 0;
    }

    c.situacao = CONTA_ATIVA;

    if (!atualizarConta(arquivo, &c))
    {
        printf("Erro ao gravar a atualizacao.\n");
        fclose(arquivo);
        return 0;
    }

    fclose(arquivo);
    printf("Conta %ld reativada.\n", numeroConta);
    return 1;
}

void extrato(long numeroConta)
{
    Conta c;
    Transacao t;
    int existe;
    int total = 0;
    FILE *arquivo;

    arquivo = fopen(ARQ_CONTAS, "rb");
    if (arquivo == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return;
    }
    existe = buscarConta(arquivo, numeroConta, &c);
    fclose(arquivo);

    if (!existe)
    {
        printf("Conta %ld nao encontrada.\n", numeroConta);
        return;
    }

    printf("\nExtrato da conta %ld - %s\n", c.numero, c.titular);
    printf("%-18s %-14s %s\n", "Tipo", "Valor", "Conta relacionada");
    printf("---------------------------------------------------\n");

    arquivo = fopen(ARQ_TRANSACOES, "rb");
    if (arquivo != NULL)
    {
        while (fread(&t, sizeof(Transacao), 1, arquivo) == 1)
        {
            if (t.conta != numeroConta)
                continue;

            printf("%-18s R$ %-11.2f ", nomeTipoTransacao(t.tipo), t.valor);
            if (t.contaRelacionada != 0)
                printf("%ld", t.contaRelacionada);
            else
                printf("-");
            printf("\n");
            total++;
        }
        fclose(arquivo);
    }

    if (total == 0)
        printf("(sem movimentacoes)\n");

    printf("---------------------------------------------------\n");
    printf("Saldo atual: R$ %.2f\n", c.saldo);
}

static void removerTransacoesDaConta(long numeroConta)
{
    Transacao t;
    FILE *origem = fopen(ARQ_TRANSACOES, "rb");
    FILE *temp;

    if (origem == NULL)
        return;

    temp = fopen(ARQ_TEMP_TRANSACOES, "wb");
    if (temp == NULL)
    {
        fclose(origem);
        return;
    }

    while (fread(&t, sizeof(Transacao), 1, origem) == 1)
    {
        if (t.conta != numeroConta)
            fwrite(&t, sizeof(Transacao), 1, temp);
    }

    fclose(origem);
    fclose(temp);

    remove(ARQ_TRANSACOES);
    rename(ARQ_TEMP_TRANSACOES, ARQ_TRANSACOES);
}

int excluirContasEncerradas(void)
{
    Conta c;
    int mantidas = 0, removidas = 0;
    FILE *origem, *temp;

    origem = fopen(ARQ_CONTAS, "rb");
    if (origem == NULL)
    {
        printf("Nenhuma conta cadastrada.\n");
        return 0;
    }

    temp = fopen(ARQ_TEMP, "wb");
    if (temp == NULL)
    {
        printf("Erro ao criar arquivo temporario.\n");
        fclose(origem);
        return 0;
    }

    while (fread(&c, sizeof(Conta), 1, origem) == 1)
    {
        if (c.situacao == CONTA_ENCERRADA)
        {
            removidas++;
            removerTransacoesDaConta(c.numero);
            continue;
        }

        if (fwrite(&c, sizeof(Conta), 1, temp) != 1)
        {
            printf("Erro ao gravar no arquivo temporario. Operacao cancelada.\n");
            fclose(origem);
            fclose(temp);
            remove(ARQ_TEMP);
            return 0;
        }
        mantidas++;
    }

    fclose(origem);
    fclose(temp);

    if (remove(ARQ_CONTAS) != 0 || rename(ARQ_TEMP, ARQ_CONTAS) != 0)
    {
        printf("Erro ao substituir o arquivo de contas.\n");
        return 0;
    }

    printf("Exclusao concluida: %d removida(s), %d mantida(s).\n", removidas, mantidas);
    return 1;
}

static void menuDeposito(void)
{
    long numero;
    double valor;

    printf("Numero da conta: ");
    numero = lerLong();
    printf("Valor do deposito: ");
    valor = lerDouble();

    if (depositar(numero, valor))
        printf("Deposito realizado.\n");
}

static void menuSaque(void)
{
    long numero;
    double valor;

    printf("Numero da conta: ");
    numero = lerLong();
    printf("Valor do saque: ");
    valor = lerDouble();

    if (sacar(numero, valor))
        printf("Saque realizado.\n");
}

static void menuTransferencia(void)
{
    long origem, destino;
    double valor;

    printf("Conta de origem: ");
    origem = lerLong();
    printf("Conta de destino: ");
    destino = lerLong();
    printf("Valor: ");
    valor = lerDouble();

    if (transferir(origem, destino, valor))
        printf("Transferencia realizada.\n");
}

static void menuEncerrar(void)
{
    printf("Numero da conta a encerrar: ");
    encerrarConta(lerLong());
}

static void menuReativar(void)
{
    printf("Numero da conta a reativar: ");
    reativarConta(lerLong());
}

static void menuExtrato(void)
{
    printf("Numero da conta: ");
    extrato(lerLong());
}

static void menuExclusaoFisica(void)
{
    char resposta[4];

    printf("ATENCAO: remove DEFINITIVAMENTE todas as contas encerradas.\n");
    printf("Confirma? (s/n): ");
    lerLinha(resposta, sizeof(resposta));

    if (resposta[0] == 's' || resposta[0] == 'S')
        excluirContasEncerradas();
    else
        printf("Operacao cancelada.\n");
}

static void exibirMenu(void)
{
    printf("\n===== CAIXA ELETRONICO =====\n");
    printf(" 1 - Cadastrar conta\n");
    printf(" 2 - Consultar conta\n");
    printf(" 3 - Realizar deposito\n");
    printf(" 4 - Realizar saque\n");
    printf(" 5 - Realizar transferencia\n");
    printf(" 6 - Listar contas\n");
    printf(" 7 - Encerrar conta\n");
    printf(" 8 - Extrato da conta\n");
    printf(" 9 - Reativar conta\n");
    printf("10 - Excluir contas encerradas (admin)\n");
    printf(" 0 - Sair\n");
    printf("Opcao: ");
}

int main(void)
{
    long opcao;

    do
    {
        exibirMenu();
        opcao = lerLong();

        switch (opcao)
        {
            case 1:  cadastrarConta();      break;
            case 2:  consultarConta();      break;
            case 3:  menuDeposito();        break;
            case 4:  menuSaque();           break;
            case 5:  menuTransferencia();   break;
            case 6:  listarContas();        break;
            case 7:  menuEncerrar();        break;
            case 8:  menuExtrato();         break;
            case 9:  menuReativar();        break;
            case 10: menuExclusaoFisica();  break;
            case 0:  printf("Saindo...\n"); break;
            default: printf("Opcao invalida.\n");
        }
    } while (opcao != 0);

    return 0;
}
