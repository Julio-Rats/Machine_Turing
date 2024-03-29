#include "exec.h"

static size_t n_exec = 0;

/*
  Fun��o onde procura pelo nome do bloco, e seta as vari�veis de controle da
    MT, de acordo com as vari�veis do bloco a ser executado.
        vari�veis de um bloco:
                name          : Nome do bloco.
                initState     : Numero do estado inicial do bloco.
                position_file : Posi��o no arquivo onde inicia o bloco.
                ================> vari�veis de controle da MT <==============
                name_bloco    : Nome do bloco a ser executado.
                estado_atual  : Estado atual na MT.
                seek          : Posi��o do arquivo para busca de blocos.
                n_bloco       : Numero de blocos salvos no vetor (length).
*/
void setseekbloco(FILE *arq, char *name_bloco)
{
    strcpy(bloco_atual, name_bloco);
    for (size_t i = 0; i < n_blocos; i++)
        if (strcmp(blocos[i].name, name_bloco) == 0)
        {
            strcpy(estado_atual, blocos[i].initState);
            seek = blocos[i].position_file;
            return;
        }
    fprintf(stderr, "\nERRO BLOCO '%s' N�O ENCONTRADO\n", name_bloco);
    para(arq);
}

void exec(FILE *arq)
{
    n_exec = 0;                                           // Contador de n�meros de execu��o.
    pilha_blocos = initStack();                           // Pilha para chamadas de blocos.
    cabecote = 0;                                         // Reset posi��o do cabe�ote sobre a fita.
    char **vetoken = NULL;                                // Vetor com as tokens das linhas de instru��o.
    char *line = (char *)malloc(sizeof(char) * TAM_LINE); // "String" leitura do aquivo.
    char type_cod;                                        // Tipo de instru��o a ser executada, 0 = instru��o normal, 1 = instru��o chamada bloco.

    setseekbloco(arq, "main");  // Seta vari�veis de controle do bloco main.
    fseek(arq, seek, SEEK_SET); //  Aplica posi��o inicio do bloco main.

    fgets(line, TAM_LINE - 1, arq); // Leitura de uma linha do arq.
    while (!feof(arq))
    {
        if (NO_LOOP)
            if (n_exec >= LIMIT_LOOP) // Verifica quant de intera��es, tratar loops.
            {
                if (modo != type_s)
                {
                    print(0, arq); // Chama fun��o para imprimir
                    modo = type_v; // Seta para imprimir.
                }                  // Verifica se esta no modo de n�o impress�o.
                fprintf(stderr, "\nMT PARADA, POSSIVEL LOOP INFINITO\n\n");
                para(arq);
            }
        if (line[0] == ';') // Verifica linha de coment�rio.
        {
            fgets(line, TAM_LINE - 1, arq);
            continue;
        }

        vetoken = decodline(line); // Decodifica linha e retorna tokens

        if (!vetoken) // Verifica se a linha foi de coment�rio.
        {
            fgets(line, TAM_LINE - 1, arq);
            continue;
        }

        if (strcmp(vetoken[0], "fim") == 0) // Verifica se n�o existe transi��o definida.
        {
            strtok(estado_atual, "\n"); //  Remove \n para apresenta��o no printf.
            modo = type_v;              //   Muda modo para que acha sempre essa impress�o.
            print(0, arq);              //    Printa essa ultima execu��o.
            // ----- MANDA MSG COM ERRO PARA O USU�RIO ----- //
            fprintf(stderr, "\nERROR TRANSI��O BLOCO '%s' ESTADO '%s' COM '%c' N�O DEFINIDA\n\n", bloco_atual, estado_atual, fita[cabecote]);
            fprintf(stderr, "ESTADO '%s' N�O EXISTE ", estado_atual);
            fprintf(stderr, "OU SIMBOLO '%C' N�O DEFINIDO NO ALFABETO\n", fita[cabecote]);
            para(arq); // Finaliza MT.
        }

        if (atoi(vetoken[0]) == atoi(estado_atual)) // Verifica se esta numa transi��o desse estado.
        {
            if (cont < 3) // Verifica erro de SINTAXE.
            {
                fprintf(stderr, "ERRO SINTAXE BLOCO '%s' ESTADO '%s'\n\n",
                        bloco_atual, estado_atual);
                fclose(arq);
                exit(EXIT_FAILURE);
            }
            if (strcmp(vetoken[2], "--") == 0) // Define o tipo da instru��o.
                type_cod = 0;                  // Instru��o normal.
            else
                type_cod = 1; // Instru��o

            simbolo_atual[0] = fita[cabecote]; // Seta simbolo atual da fita.
            simbolo_atual[1] = '\0';
            if ((type_cod == 0) && ((strcmp(simbolo_atual, vetoken[1]) == 0) ||
                                    (strcmp(vetoken[1], "*") == 0)))
            { // Verifica se pertence a esta transi��o.

                n_exec++;                // Incrementa num de execu��o feitas.
                execinstr(vetoken, arq); // Executa linha de instru��es normais.
            }
            else if (type_cod == 1)
                execblock(vetoken, arq); // Executa linha de instru��es de blocos.
        }

        if (vetoken) // Desaloca Tokens.
        {
            free(vetoken);
            vetoken = NULL;
        }
        fgets(line, TAM_LINE - 1, arq); // Leitura da proxima linha e loop.
    }
}
/*
    Para execu��o da MT.
*/
void para(FILE *arq)
{
    printf("\nMT ENCERROU:  %ld execu��es efetuadas\n\n", n_exec);
    fclose(arq);
    exit(EXIT_SUCCESS);
}
/*
      Executa transi��o 'setando' as vari�veis de controle
        da MT, printa se estiver no modo correto, e volta para
        inicio do bloco no aquivo para busca de nova transi��o
        com novo estado e simbolo.
*/
void execinstr(char **vetline, FILE *arq)
{

    char fin = 0; // Define se Fim de execu��o; 1==fim.
    // Verifica erro de SINTAXE.
    if (cont < 6)
    {
        fprintf(stderr, "ERRO SINTAXE BLOCO '%s' ESTADO '%s'\n\n",
                bloco_atual, estado_atual);
        fclose(arq);
        exit(EXIT_FAILURE);
    }
    // simbolo_novo
    if (strcmp(vetline[3], "*") != 0)
        fita[cabecote] = vetline[3][0];

    
    // movimento cabe�ote
    if (strcmp(vetline[4], "e") == 0)
    {
        // Tratamento se o cabecote ir al�m do inicio da fila, �ndice -1 em C.
        if (cabecote == 0)
        {
            for (int i = (strlen(fita) - 2); i >= 0; i--)
                fita[i + 1] = fita[i];

            fita[0] = '_';
            cabecote = 0;
        }
        else
        {
            cabecote--;
        }
    }else if (strcmp(vetline[4], "d") == 0)
        cabecote++;

    // Verifica erro de SINTAXE.
    else if (strcmp(vetline[4], "i") != 0)
    {
        fprintf(stderr, "\nERRO SINTAXE MOVIMENTO BLOCO '%s' ESTADO '%s' COM SIMBOLO '%s'\n\n",
                bloco_atual, estado_atual, estado_atual);
        fclose(arq);
        exit(EXIT_FAILURE);
    }
    
    // Estado novo
    if (strcmp(vetline[5], "pare") == 0) // Verifica Fim do algor�timo.
        fin = 1;
    else if (strcmp(vetline[5], "retorne") == 0) // Verifica retorno de um bloco.
    {
        do
        {
            recall *retorno = popStack(pilha_blocos); // Desempilha bloco da pilha.
            if (!retorno)                             // Trata algum erro se ocorrer.
            {
                printf("\nERRO DE RETORNO DO BLOCO '%s' \n", bloco_atual);
                para(arq);
            }
            /* Seta vari�veis de controle:
                bloco_atual : Nome do bloco atual.
                n_bloco_atual : Posi��o no vetor (�ndice).
                novo_estado   : Novo Estado da MT, estado de retorno.
                estado_atual  : Estado atual da MT.
            */
            strcpy(bloco_atual, (char *)retorno->recall_bloco);
            strcpy(novo_estado, retorno->recall_state);
            strcpy(estado_atual, novo_estado);
            // Verifica se a retornada � um pare.
            if (strcmp(novo_estado, "pare") == 0)
            {
                fin = 1;
                strcpy(novo_estado, (char *)retorno->final_state);
                strcpy(estado_atual, novo_estado);
            }
        } while (strcmp(novo_estado, "retorne") == 0);
    }
    else
    {
        // Caso n�o seja um retorno seta o novo estado.
        if (strcmp(vetline[5], "*") == 0)
            strcpy(novo_estado, estado_atual);
        else
            strcpy(novo_estado, vetline[5]);
    }
    // Tratar o comando "!".
    if (cont > 6)
        if (strcmp("!", vetline[6]) == 0)
            modo = type_v;
    /*
      Imprime a execu��o e seta o inicio do bloco no arq para
          nova busca de transi��o para novo estado e simbolo.
    */
    print(fin, arq);
    setseekbloco(arq, bloco_atual);
    strcpy(estado_atual, novo_estado);
    fseek(arq, seek, SEEK_SET);
}
/*
    Executa instru��o de chamada de blocos, empilha o bloco atual.
*/
void execblock(char **vetline, FILE *arq)
{
    print(0, arq); // Imprime execu��o atual.
    recall *retorno = NULL;
    retorno = (recall *)malloc(sizeof(recall)); // Aloca ED de chamada de bloco.
    if (!retorno)                               // Trata poss�vel erro de aloca��o.
    {
        fprintf(stderr, "\nERRO ALOCA��O DE CHAMADA DE BLOCO\n\n");
        exit(EXIT_FAILURE);
    }

    strcpy((char *)retorno->recall_bloco, bloco_atual); // Seta na strtc nome do Bloco atual.
    if (strcmp(vetline[2], "pare") == 0)
        strcpy(retorno->final_state, estado_atual); // Seta na strtc estado de retorno.

    strcpy(retorno->recall_state, vetline[2]); //    Seta na strtc estado de retorno.
    pushStack(pilha_blocos, retorno);          //    Empilha esse bloco na ED pilha.
    setseekbloco(arq, vetline[1]);             //    Seta vari�veis de controle.
    print(0, arq);                             //    Imprime execu��o.
    fseek(arq, seek, SEEK_SET);                //    Seta posi��o no arq com novo bloco.

    if (cont > 3) // Trata operador "!"
        if (strcmp("!", vetline[3]) == 0)
            modo = type_v;
}

/*
  Imprime conforme requisitado no documento do trabalho.
*/
void print(int fin, FILE *arq)
{
    int return_scanf;
    int dots;
    int fitantes, fitapos;
    char flagvaziantes = 1;
    char fitaprint[LEN_FITA_PRINT + 6];
    char ponts[TAM_BLOCK];

    if (modo != type_r) // Modo de impress�o.
    {
        dots = TAM_BLOCK - strlen(bloco_atual) - 1;                      // Sobra do nome do bloco, a ser preenchido de pontos.
        fitapos = cabecote + (int)((LEN_FITA_PRINT) / 2) - strlen(fita); // Excesso ou falta de s�mbolos depois do cabe�ote.
        fitantes = (int)((LEN_FITA_PRINT) / 2) - cabecote;               // Excesso ou falta de s�mbolos antes do cabe�ote.
        // Verifica se houve excesso antes.
        if (fitantes < 0)
        {
            fitantes *= -1;    // "ABS"
            flagvaziantes = 0; // Desativa FLAG;
        }
        // Verifica se houve excesso apos.
        if (fitapos < 0)
            fitapos *= -1;
        int cont = 0;
        // Cria a "LINHA" de impress�o.
        if (flagvaziantes) // Trata se tiver menos de 20 antes do cabecote.
        {
            strcpy(fitaprint, "");
            for (int i = 0; i < fitantes; i++)
                strcat(fitaprint, "_");
            cont = fitantes;
            strncat(fitaprint, fita, cabecote);
            cont += cabecote;
        }
        else
        { // Trata se tiver mais de 20 antes do cabecote.
            strcpy(fitaprint, "");
            for (int i = fitantes; i < cabecote; i++)
                fitaprint[cont++] = fita[i];
            fitaprint[cont] = '\0';
        }

        char pont[6];
        // Cria "visual" do cabe�ote.
        pont[0] = (char)delim_cabecote[0];
        pont[1] = ' ';
        pont[2] = (char)fita[cabecote];
        pont[3] = ' ';
        pont[4] = (char)delim_cabecote[1];
        pont[5] = '\0';
        cont += 5;

        // Adiciona cabecote na impress�o.
        strcat(fitaprint, pont);
        // Adiciona a frente do cabecote na impress�o.
        for (int i = cabecote + 1; i < cabecote + (int)((LEN_FITA_PRINT) / 2) + 1; i++)
            fitaprint[cont++] = fita[i];
        fitaprint[LEN_FITA_PRINT + 6 - 1] = '\0';
        // ADD pontos ante do nome do bloco
        memset(ponts, '.', dots);
        ponts[dots] = '\0';
        // Imprimi.
        printf("%s%s.%04d:%s\n", ponts, bloco_atual, atoi(estado_atual), fitaprint);
        // Trata modo -s, quando o valor inspirar.
        if ((n_step <= n_exec) && (modo == type_s))
        {
            char op[3];
            long int n_temp = -1;
            printf("\nForne�a op��o '-r', '-v', '-s numero' ou 'qualquer outro para continuar': ");
            return_scanf = scanf("%s", op);
            // Pega a nova op de modo, se for -s ou -n trata poss�veis erros.
            if (strlen(op) > 1)
                switch (op[1])
                {
                case 'r':
                    modo = type_r;
                    break;
                case 'v':
                    modo = type_v;
                    break;
                case 's':
                    do
                    {
                        printf("Digite o numeros de passos: ");
                        return_scanf = scanf("%ld", &n_temp);
                    } while (return_scanf == EOF);
                    while (n_temp <= 0)
                        do
                        {
                            printf("Digite um numero maior que zero: ");
                            return_scanf = scanf("%ld", &n_temp);
                        } while (return_scanf == EOF);
                    n_step = n_exec + n_temp;
                    step_arg = n_temp;
                    break;
                case 'n':
                    n_step += step_arg;
                    break;
                default:
                    n_step += step_arg;
                    break;
                }
            else
                n_step += step_arg;
            printf("\n");
        }
    }
    // Caso o modo encontre um pare , ele entrara aq e finalizara a MT.
    if (fin)
    {
        if (modo == type_r)
        {
            modo = type_v;
            print(1, arq);
        }
        para(arq);
    }
}
