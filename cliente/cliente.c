#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1" // Endereço IP do servidor
#define SERVER_PORT 8080     // Porta do servidor
#define BUFFER_MAX_SIZE 1024
#define BUFFER_LONG 1024*1024
#define BUFFER_LOCAL 50

void TiraEnter(char *buffer){
    size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
}

void enviarArquivo(int sockfd, const char *nome_arquivo) {
    printf("---Enviando Arquivo---\n");
    char bufferLocal[BUFFER_LOCAL];
    // Abre o arquivo para leitura
    FILE *arquivo = fopen(nome_arquivo, "rb");
    if (arquivo == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }


    // Obtém o tamanho do arquivo
    fseek(arquivo, 0, SEEK_END);
    long tamanho_arquivo = ftell(arquivo);
    rewind(arquivo);
    // Avisar para o servidor que se trata de um upload
    
    // Envia o tamanho do arquivo para o servidor
    char tamanho_str[32];
    sprintf(tamanho_str, "%ld", tamanho_arquivo);
    printf("Tamanho do Arquivo: %s\n",tamanho_str);
    if (send(sockfd, tamanho_str, strlen(tamanho_str), 0) == -1) {
        perror("Erro ao enviar o tamanho do arquivo");
        exit(EXIT_FAILURE);
    }

    //ack
    if (recv(sockfd, bufferLocal, BUFFER_LOCAL, 0) < 0) {
                perror("Erro ao receber a resposta");
                exit(1);
    }
    printf("Confirmacao:%s\n",bufferLocal);

    // Envia o conteúdo do arquivo para o servidor
    char buffer[1024];
    size_t bytes_lidos;
    while ((bytes_lidos = fread(buffer, 1, sizeof(buffer), arquivo)) > 0) {
        if (send(sockfd, buffer, bytes_lidos, 0) == -1) {
            perror("Erro ao enviar o arquivo");
            exit(EXIT_FAILURE);
        }
    }

    fclose(arquivo);
}

void receberArquivo(int sockfd, const char *nome_arquivo) {
    printf("---RECEBIMENTO---\n");
    char bufferLocal[] = "OK";
    // Recebe o tamanho do arquivo do cliente
    char tamanho_str[32];
    memset(tamanho_str,0,sizeof(tamanho_str));
    if (recv(sockfd, tamanho_str, sizeof(tamanho_str), 0) == -1) {
        perror("Erro ao receber o tamanho do arquivo");
        exit(EXIT_FAILURE);
    }
    
    printf("Tamanho Arquivo: %s\n",tamanho_str);
    long tamanho_arquivo = atol(tamanho_str);
    printf("Tamanho Arquivo: %ld\n",tamanho_arquivo);
 
    if (send(sockfd, bufferLocal, BUFFER_LOCAL, 0) < 0) {
                perror("Erro ao enviar o comando");
                exit(1);
    }
    

    // Cria um buffer para armazenar os dados do arquivo
    char *buffer = (char*)malloc(tamanho_arquivo);
    if (buffer == NULL) {
        perror("Erro ao alocar memória");
        exit(EXIT_FAILURE);
    }
    // Recebe o conteúdo do arquivo do cliente
    long bytes_recebidos = 0;
    while (bytes_recebidos < tamanho_arquivo) {
        long bytes_restantes = tamanho_arquivo - bytes_recebidos;
        long bytes_atual = recv(sockfd, buffer + bytes_recebidos, bytes_restantes, 0);
        if (bytes_atual == -1) {
            perror("Erro ao receber o arquivo");
            exit(EXIT_FAILURE);
        } else if (bytes_atual == 0) {
            perror("Conexão encerrada pelo cliente");
            exit(EXIT_FAILURE);
        }
        bytes_recebidos += bytes_atual;
    }

    // Salva o conteúdo do arquivo em um arquivo local
    FILE *arquivo = fopen(nome_arquivo, "wb");
    if (arquivo == NULL) {
        perror("Erro ao criar o arquivo local");
        exit(EXIT_FAILURE);
    }

    if (fwrite(buffer, 1, tamanho_arquivo, arquivo) != tamanho_arquivo) {
        perror("Erro ao escrever o arquivo local");
        exit(EXIT_FAILURE);
    }

    fclose(arquivo);
    free(buffer);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Erro ao criar o socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_addr.sin_addr)) <= 0) {
        perror("Endereço inválido");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Erro ao conectar");
        exit(EXIT_FAILURE);
    }



    
    // Código

    char comando[BUFFER_MAX_SIZE];
    char respComando[BUFFER_LONG];
    char *ret1;
    char *ret2;
    char resposta[100];
    

    while (1)
    {
        printf("Digite um Comando:\n");
        fgets(comando,BUFFER_MAX_SIZE,stdin);
        //printf("PASSEI DO FGETS");

        // removendo último caractere (enter) para \0
        TiraEnter(comando);

        if (send(sockfd, comando, strlen(comando), 0) < 0) {
                perror("Erro ao enviar o comando");
                exit(1);
        }

        ret1 = strstr(comando,"$upload");
        if (ret1 != NULL)
        {
            char nomeArquivo[50];
            char diretorioArquivo[100];
            printf("Digite o caminho do Arquivo:\n");
            fgets(diretorioArquivo,100,stdin);  // Diretório do arquivo.
            TiraEnter(diretorioArquivo);

            // nome do arquivo
            if (recv(sockfd, resposta, 100, 0) < 0) {
                perror("Erro ao receber a resposta");
                exit(1);
            }

            //digite o nome do arquivo
            printf("%s",resposta);
            fgets(nomeArquivo,50,stdin);
            TiraEnter(nomeArquivo);

            //envio do nome do arquivo.
            if (send(sockfd,nomeArquivo , strlen(nomeArquivo), 0) < 0) {
                perror("Erro ao enviar o comando");
                exit(1);
            }

            enviarArquivo(sockfd, diretorioArquivo);
            printf("Arquivo enviado com sucesso!\n");
	    goto continua;
        }

        ret2 = strstr(comando,"$download");
        if (ret2 != NULL)
        {
            char arquivoDownload[50];
            char diretorioDownload[100];

            // o servidor pedindo diretorio
            if (recv(sockfd, resposta, 100, 0) < 0) {
                perror("Erro ao receber a resposta");
                exit(1);
            }
            printf("%s",resposta);
            fgets(diretorioDownload,100,stdin);
            TiraEnter(diretorioDownload);

            // o cliente enviando o diretorio
            if (send(sockfd,diretorioDownload , strlen(diretorioDownload), 0) < 0) {
                perror("Erro ao enviar o comando");
                exit(1);
            }
            
            printf("Digite o Nome que o seu arquivo vai ser salvo:\n");
            fgets(arquivoDownload,50,stdin);
            TiraEnter(arquivoDownload);

            const char *nome_arquivo_recebido = arquivoDownload; // Nome que vai ser Salvo
            receberArquivo(sockfd, nome_arquivo_recebido);
            printf("Arquivo recebido com sucesso!\n");
            goto continua;
        }

        int n = strcmp(comando,"exit");
        if(n == 0)
        {
            printf("Voce Escolheu Sair\n");
            break;
        }
        
        //recebimento
        if (recv(sockfd, respComando, BUFFER_LONG, 0) < 0) {
                perror("Erro ao receber a resposta");
                exit(1);
        }
        printf("\n%s\n",respComando);

        continua:
        memset(respComando,0,sizeof(respComando));
        memset(comando,0,sizeof(comando));
	    memset(resposta,0,sizeof(resposta));

    
    }
    close(sockfd);

    return 0;
}


