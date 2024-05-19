#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

void usage(int argc, char **argv) {
    printf("usage: %s <serverIP> <server port1> <server port2>\n", argv[0]);
    printf("example: %s ::1 12345 54321\n", argv[0]);
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage) {
    if (addrstr == NULL || portstr == NULL) {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0) {
        return -1;
    }
    port = htons(port);

    if (inet_pton(AF_INET6, addrstr, &(((struct sockaddr_in6*)storage)->sin6_addr)) == 1) {
        // IPv6 address
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
    } else if (inet_pton(AF_INET, addrstr, &(((struct sockaddr_in*)storage)->sin_addr)) == 1) {
        // IPv4 address
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
    } else {
        return -1;
    }

    return 0;
}

#define BUFSZ 500

int main(int argc, char **argv) {
    if (argc != 4) {
        usage(argc, argv);
    }

    struct sockaddr_storage server_addr1, server_addr2;

    if (0 != addrparse(argv[1], argv[2], &server_addr1) || 0 != addrparse(argv[1], argv[3], &server_addr2)) {
        usage(argc, argv);
    }

    int client_socket1 = socket(server_addr1.ss_family, SOCK_STREAM, 0);
    int client_socket2 = socket(server_addr2.ss_family, SOCK_STREAM, 0);

    if (client_socket1 == -1 || client_socket2 == -1) {
        perror("socket falhou\n");
        exit(1);
    }

    // Conectando ao servidor 1
    if (connect(client_socket1, (struct sockaddr *)&server_addr1, sizeof(server_addr1)) != 0) {
        perror("Conexão ao servidor 1 falhou\n");
        exit(1);
    }

    // Conectando ao servidor 2
    if (connect(client_socket2, (struct sockaddr *)&server_addr2, sizeof(server_addr2)) != 0) {
        perror("Conexão ao servidor 2 falhou\n");
        exit(1);
    }

    char buff1[BUFSZ];
    char buff2[BUFSZ];
    char comando_cliente[BUFSZ];
    char comando [BUFSZ];

    int old_value;
    int new_value;

    //Verificando se os servidores estão cheios
    recv(client_socket1, buff1, sizeof(buff1), 0);
    recv(client_socket2, buff2, sizeof(buff2), 0);
    int id_inicial1=0;
    int id_inicial2=0;
    //Se tiver cheio, fecha os dois sockets e imprime a mensagem Client limit exceeded
    if (strncmp(buff1, "ERROR 01", 8) == 0 || strncmp(buff2, "ERROR 01", 8) == 0) {
        printf("Client limit exceeded\n");
        close(client_socket1);
        close(client_socket2);
        //limpa os buffs
        memset(buff1, 0, sizeof(buff1));
        memset(buff2, 0, sizeof(buff2));
        exit(1);
    //se não estiver cheio, envia REQ_ADD para os dois servidores, recebe as respostas dos servidores, e imprime as respectivas mensagens
    }else{
        send(client_socket1, "REQ_ADD\n", strlen("REQ_ADD\n"), 0);
        recv(client_socket1, buff1, sizeof(buff1), 0);
        
        sscanf(buff1, "RES_ADD %d", &id_inicial1);
        printf("Servidor SE New ID: %d\n", id_inicial1);
        memset(buff1, 0, sizeof(buff1));

        send(client_socket2, "REQ_ADD\n", strlen("REQ_ADD\n"), 0);
        recv(client_socket2, buff2, sizeof(buff2), 0);
        
        sscanf(buff2, "RES_ADD %d\n", &id_inicial2);
        printf("Servidor SEII New ID: %d\n\n", id_inicial2);
        memset(buff2, 0, sizeof(buff2));
    }


    //loop para comunicação dos comandos entre o cliente e os servidores
    while (1) {
        
        //cliente insere um comando
        printf("Digite um comando: ");
        fgets(comando_cliente, sizeof(comando_cliente), stdin);
        // Removendo caractere "\n"
        comando_cliente[strcspn(comando_cliente, "\n")] = 0;

        //se o cliente digitar kill, ele manda REQ_REM id para os dois servidores, assim os servidores respondem e imprimem as respectivas mensagens
        if (strncmp(comando_cliente, "kill", 4) == 0){
            snprintf(comando, BUFSZ, "REQ_REM %d\n", id_inicial1);
            send(client_socket1, comando, strlen(comando), 0);
            recv(client_socket1, buff1, sizeof(buff1), 0);
            if (strncmp(buff1, "ERROR 02", 8) == 0){
                printf("Client not found\n\n");
            }else if (strncmp(buff1, "OK 01", 5) == 0){
                close(client_socket1);
            }
            memset(comando, 0, sizeof(comando));

            snprintf(comando, BUFSZ, "REQ_REM %d\n", id_inicial1);
            send(client_socket2, comando, strlen(comando), 0);
            recv(client_socket2, buff2, sizeof(buff2), 0);
            if (strncmp(buff2, "ERROR 02", 8) == 0){
                printf("Client not found\n");
            }else if (strncmp(buff2, "OK 01", 5) == 0){
                printf("Successful disconnect\n");
                close(client_socket2);
                break;
            }
        
        //se o client digitar display info se, manda para o servidor REQ_INFOSE, o servidor trata a msg, responde o client, o client trata a msg do servidor e imprime a producao atual
        }else if (strncmp(comando_cliente, "display info se", 15) == 0){
            char msg [100];
            snprintf(comando, BUFSZ, "REQ_INFOSE\n");
            send(client_socket2, comando, strlen(comando), 0);
            recv(client_socket2, buff2, sizeof(buff2), 0);
            sscanf(buff2, "RES_INFOSE %[^\n]", msg);
            printf("producao atual: %s\n\n", msg);
            memset(comando, 0, sizeof(comando));

        //se o client digitar display info scii, manda para o servidor REQ_INFOSCII, o servidor trata a msg, responde o client, o client trata a msg do servidor e imprime o consumo atual
        }else if (strncmp(comando_cliente, "display info scii", 17) == 0){
            char msg [100];
            snprintf(comando, BUFSZ, "REQ_INFOSCII\n");
            send(client_socket1, comando, strlen(comando), 0);
            recv(client_socket1, buff1, sizeof(buff1), 0);
            sscanf(buff1, "RES_INFOSCII %[^\n]", msg);
            printf("consumo atual: %s\n\n", msg);
            memset(comando, 0, sizeof(comando));

        //o client ao digitar query condition, manda para o servidor SCII REQ_STATUS, o servidor SCII trata a msg e responde para o client o estado atual
        }else if (strncmp(comando_cliente, "query condition", 15) == 0){
            char msg [100];
            snprintf(comando, BUFSZ, "REQ_STATUS\n");
            send(client_socket2, comando, strlen(comando), 0);
            recv(client_socket2, buff2, sizeof(buff2), 0);
            sscanf(buff2, "RES_STATUS %[^\n]", msg);
            printf("estado atual: %s\n", msg);
            memset(comando, 0, sizeof(comando));
            memset(buff2, 0, sizeof(buff1));
            //se o estado atual for alta, o client envia RES_UP para o servidor, e imprime o consumo antigo e o consumo atual
            if (strncmp(msg, "alta", 4) == 0) {
                snprintf(comando, BUFSZ, "REQ_UP\n");
                send(client_socket1, comando, strlen(comando), 0);
                recv(client_socket1, buff1, sizeof(buff1), 0);
                sscanf(buff1, "RES_UP %d %d", &old_value, &new_value);
                printf("consumo antigo: %d\n", old_value);
                printf("consumo atual: %d\n\n", new_value);

            //se o estado atual for moderada, o client envia RES_NONE para o servidor, e imprime o consumo antigo
            }else if (strncmp(msg, "moderada", 8) == 0) {
                snprintf(comando, BUFSZ, "REQ_NONE\n");
                send(client_socket1, comando, strlen(comando), 0);
                recv(client_socket1, buff1, sizeof(buff1), 0);
                sscanf(buff1, "RES_NONE %d", &old_value);
                printf("consumo antigo: %d\n\n", old_value);

            //se o estado atual for baixa, o client envia RES_DOWN para o servidor, e imprime o consumo antigo e o consumo atual
            }else if (strncmp(msg, "baixa", 5) == 0) {
                snprintf(comando, BUFSZ, "REQ_DOWN\n");
                send(client_socket1, comando, strlen(comando), 0);
                recv(client_socket1, buff1, sizeof(buff1), 0);
                sscanf(buff1, "RES_DOWN %d %d", &old_value, &new_value);
                printf("consumo antigo: %d\n", old_value);
                printf("consumo atual: %d\n\n", new_value);
            }
        }

        //limpa os buffers
        memset(buff1, 0, sizeof(buff1));
        memset(buff2, 0, sizeof(buff2));
        memset(comando_cliente, 0, sizeof(comando_cliente));
        memset(comando, 0, sizeof(comando));
    }
    //fecha os sockets
    close(client_socket1);
    close(client_socket2);

    return 0;
}