#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>


#define SERVER_PORT 8080  // Porta do servidor
#define BUFFER_MAX_SIZE 1024 //tamanho do buffer
#define BUFFER_LONG 1024*1024 //Maior do Buffer
#define BUFFER_LOCAL 50


char* RespostaComando(char* buffer, char* comando)
{
    FILE* output;
    char word[BUFFER_MAX_SIZE];
    output = popen(comando, "r"); //executa o comando do shell e guarda a saída

    if (output == NULL)
    {
        fputs("/nFalha ao executar comando!/n", stderr);
    }
    else

        while (fgets(word, BUFFER_MAX_SIZE - 1, output) != NULL)
        {
            strcat(buffer, word);
        }

    pclose(output);

    return buffer;
}

void enviarArquivo(int sockfd, const char *nome_arquivo) {
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

void handleClient(int client_sockfd)
{
    char buffer[BUFFER_MAX_SIZE];
    char arg[BUFFER_LONG];
    char resp[100];
    char *ans;
    char *ret1;
    char *ret2;
    while (1) 
    {
        if (recv(client_sockfd, buffer, BUFFER_MAX_SIZE, 0) < 0) {
            perror("Erro ao receber a resposta");
            exit(1);
        }

        ret1 = strstr(buffer, "$upload");
        if (ret1 != NULL) {
            printf("---RECEBIMENTO---\n");
            // Pedindo o nome do arquivo
            if (send(client_sockfd, "Nome do Arquivo no Servidor(com a extensao):\n", strlen("Nome do Arquivo no Servidor(com a extensao):\n"), 0) < 0) {
                perror("Erro ao enviar o comando");
                exit(1);
            }

            // Recebimento do nome do arquivo
            if (recv(client_sockfd, resp, 100, 0) < 0) {
                perror("Erro ao receber a resposta");
                exit(1);
            }
            printf("Nome do Arquivo Adicionado: %s\n", resp);

            const char *nome_arquivo_recebido = resp; // Nome do arquivo a ser recebido
            receberArquivo(client_sockfd, nome_arquivo_recebido);
            printf("Arquivo recebido com sucesso!\n");
            goto continua;
        }

        ret2 = strstr(buffer, "$download");
        if (ret2 != NULL) {
            printf("---ENVIANDO ARQUIVO---\n");
            // Pedindo o diretório
            if (send(client_sockfd, "Diretorio do Arquivo para Download:\n", strlen("Diretorio do Arquivo para Download:\n"), 0) < 0) {
                perror("Erro ao enviar o comando");
                exit(1);
            }

            // Recebendo o diretório
            if (recv(client_sockfd, resp, 100, 0) < 0) {
                perror("Erro ao receber a resposta");
                exit(1);
            }
            printf("Diretorio: %s\n", resp);
            const char *nome_arquivo_enviado = resp; // Nome do arquivo a ser enviado
            enviarArquivo(client_sockfd, nome_arquivo_enviado);
            printf("Arquivo enviado com sucesso!\n");
            goto continua;
        }
        
        ans = NULL;
        ans = RespostaComando(arg, buffer);
        strcat(ans," ");

        if (send(client_sockfd, ans, strlen(ans), 0) < 0) {
            perror("Erro ao enviar a resposta");
            exit(1);
        }
        continua:
        memset(arg,0,sizeof(arg));
        memset(buffer,0,sizeof(buffer));
        memset(resp,0,sizeof(resp));
    }
}



int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Erro ao criar o socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Erro ao fazer o bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) == -1) {
        perror("Erro ao aguardar conexões");
        exit(EXIT_FAILURE);
    }

    printf("Aguardando conexões...\n");

    while(1)
    {
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sockfd == -1) {
        perror("Erro ao aceitar conexão");
        exit(EXIT_FAILURE);
    }

    printf("Conexão estabelecida\n");


    //novos clientes
    pid_t pid = fork();
        if (pid == -1) {
            perror("Erro ao criar processo filho");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Processo filho - manipula o cliente
            close(sockfd);
            handleClient(client_sockfd);
            exit(0);
        } else {
            // Processo pai - continua aceitando conexões
            close(client_sockfd);
            waitpid(-1, NULL, WNOHANG);  // Limpar processos filhos concluídos
        }
    }


    //Código
    close(sockfd);

    return 0;
}




