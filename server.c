#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

#define BUFSZ 500
#define MAX_CLIENTS 10

int active_connections_SE = 0;
int active_connections_SEII = 0;

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <port>\n", argv[0]);
    printf("example: %s v4 12345\n", argv[0]);
    exit(EXIT_FAILURE);
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            perror("ntop");
            exit(1);
        }
        port = ntohs(addr4->sin_port);
    } else if (addr->sa_family == AF_INET6) {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)) {
            perror("ntop");
            exit(1);
        }
        port = ntohs(addr6->sin6_port);
    } else {
        perror("erro protocol family");
        exit(1);
    }
    if (str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t) atoi(portstr);
    if (port == 0) {
        return -1;
    }
    port = htons(port);

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    } else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    } else {
        return -1;
    }
}

//criando um struct client para manipular os ids
typedef struct {
    int id;
} Cliente;

int main(int argc, char **argv) {
    if (argc != 3) {
        usage(argc, argv);
    }

//declarando a função para sortear numeros aleatórios
    srand(time(NULL));

    // Gera um número aleatório entre 20 e 50 de producao
    int producao = 20 + rand() % 31;
    //Gera um número aleatório entre 20 e 50 de consumo
    int consumo = rand() % 101;
    //instanciando old_value
    int old_value;

//criando um array do tipo client para manipular os ids dos client em cada servidor, separadamente
    Cliente clientes_SE[MAX_CLIENTS];
    Cliente clientes_SEII[MAX_CLIENTS];

//iniciando todos os ids com 0 para o servidor seii
    for (int l = 0; l < MAX_CLIENTS; l++) {
        clientes_SEII[l].id = 0;
    }

//iniciando todos os ids com 0 para o servidor se
    for (int l = 0; l < MAX_CLIENTS; l++) {
        clientes_SE[l].id = 0;
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    // Criando o socket
    int server_socket = socket(storage.ss_family, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket falhou\n");
        exit(1);
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);

    // Vinculando o socket para o IP e porta especificadas
    if (0 != bind(server_socket, addr, sizeof(storage))) {
        perror("bind");
        exit(1);
    }

    // Escutando o liten
    if (0 != listen(server_socket, MAX_CLIENTS)) {
        perror("listen");
        exit(1);
    }
    printf("Starting to listen..\n\n");


    fd_set read_fds, master_fds; // Declara conjuntos de descritores de arquivo para leitura
    FD_ZERO(&master_fds); // Limpa todos os descritores de arquivo no conjunto master_fds
    FD_SET(server_socket, &master_fds); // Adiciona o socket do servidor ao conjunto master_fds
    int fdmax = server_socket; // Inicializa fdmax com o valor do socket do servidor

    char portstr[BUFSZ];

    // Converte o endereço em uma string legível por humanos e armazena em portstr
    addrtostr(addr, portstr, BUFSZ);
    

    while (1) {
        read_fds = master_fds;
        
        //iniciando o select
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        //loop para lidar com os diferentes clients
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) {
                    struct sockaddr_storage cstorage;
                    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
                    socklen_t caddrlen = sizeof(cstorage);

                    //aceitando o client que esta tentando se conectar
                    int client_socket = accept(server_socket, caddr, &caddrlen);
                    if (client_socket == -1) {
                        perror("accept");
                    } else {
                        // Verifica o servidor com base na porta
                        if (atoi(argv[2]) == 12345) { // Servidor SE
                            //se o numero simultaneo de conexões for maior que 10, imprime client limit exceeded, e manda ERROR 01 para o respectivo client
                            if (active_connections_SE >= MAX_CLIENTS) {
                                printf("Client limit exceeded\n\n");
                                send(client_socket, "ERROR 01\n", strlen("ERROR 01\n"), 0);
                                close(client_socket); //fecha a comunicação com o respectivo client
                            } else {
                                //se não, instancia o novo socket do client
                                FD_SET(client_socket, &master_fds);
                                if (client_socket > fdmax) {
                                    fdmax = client_socket;
                                }
                                char addrstr[BUFSZ];
                                addrtostr(caddr, addrstr, BUFSZ);
                                //envia para o client "conectou"
                                send(client_socket, "conectou", strlen("conectou"), 0);
                                //aumenta as conexões simultaneas em 1
                                active_connections_SE++;
                            }
                        } else if (atoi(argv[2]) == 54321) { // Servidor SEII
                            if (active_connections_SEII >= MAX_CLIENTS) {
                                printf("Client limit exceeded\n\n");
                                send(client_socket, "ERROR 01\n", strlen("ERROR 01\n"), 0);
                                close(client_socket);
                            } else {
                                FD_SET(client_socket, &master_fds);
                                if (client_socket > fdmax) {
                                    fdmax = client_socket;
                                }
                                char addrstr[BUFSZ];
                                addrtostr(caddr, addrstr, BUFSZ);
                                //printf("Nova conexão de %s no socket %d (Servidor SEII)\n", addrstr, client_socket);
                                send(client_socket, "conectou", strlen("conectou"), 0);
                                active_connections_SEII++;
                            }
                        }
                    }
                } else {
                    //declara um char para armazenar a resposta do servidor
                    char resposta[BUFSZ];
                    //declara um char para receber o comando do client
                    char buffer[BUFSZ];
                    int bytes_received = recv(i, buffer, sizeof(buffer), 0);
                    //verifica se ainda tem conexão com o client
                    if (bytes_received <= 0) {
                        //se não tiver mais conexão, encerra a conexão com o respectivo client, fechando seu socket
                        if (bytes_received == 0) {
                            printf("Client %d encerrou a conexão\n\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_fds);
                        //se o client perder a conexão, as conexões simultaneas são decrementadas em ambos os servidores
                        if (atoi(argv[2]) == 12345) { // Servidor SE
                            active_connections_SE--;
                        } else if (atoi(argv[2]) == 54321) { // Servidor SEII
                            active_connections_SEII--;
                        }
                    } else {
                        buffer[bytes_received] = '\0';
                        //imprime a mensagem recebida do client
                        printf("%s", buffer);
                        //se a mensagem do client for REQ_ADD, responde RES_ADD id e atribui o id do client no array de client
                        if (strncmp(buffer, "REQ_ADD", 7) == 0) {
                            if (atoi(argv[2]) == 12345) {
                                int client_id = i;
                                snprintf(resposta, BUFSZ, "RES_ADD %d\n", client_id);
                                send(i, resposta, strlen(resposta), 0);
                                printf("Client %d added\n\n", client_id);
                                //faz uma varredura para alocar o id aonde tem 0
                                for (int p = 0; p < MAX_CLIENTS; p++) {
                                    if (clientes_SE[p].id == 0) {
                                        clientes_SE[p].id = i;
                                        break;
                                    }
                                }
                            } else if (atoi(argv[2]) == 54321) {
                                int client_id2 = i;
                                snprintf(resposta, BUFSZ, "RES_ADD %d\n", client_id2);
                                send(i, resposta, strlen(resposta), 0);
                                printf("Client %d added\n\n", client_id2);
                                for (int p = 0; p < MAX_CLIENTS; p++) {
                                    if (clientes_SEII[p].id == 0) {
                                        clientes_SEII[p].id = i;
                                        break;
                                    }
                                }
                            }
                        //se a mensagem do client for REQ_REM
                        } else if (strncmp(buffer, "REQ_REM", 7) == 0) {
                            int id;
                            //armazena o id do client em id
                            sscanf(buffer, "REQ_REM %d", &id);
                            if (atoi(argv[2]) == 12345) {
                                int achou = 0;
                                //vê se tem 0 no array de ids, se caso tiver 0 é porque tem vaga disponível
                                for (int j = 0; j < MAX_CLIENTS; j++) {
                                    if (clientes_SE[j].id == id) {
                                        achou = 1; //se tiver 0, achou vai para 1
                                        clientes_SE[j].id = 0;
                                        break;
                                    }
                                }
                                //se achou for verdadeiro, ou seja, 1, o servidor manda OK 01 para o client e remove o respectivo client
                                if (achou) {
                                    snprintf(resposta, BUFSZ, "OK 01\n");
                                    send(i, resposta, strlen(resposta), 0);
                                    printf("Servidor SE Client %d removed\n\n", id);
                                    close(i);
                                    FD_CLR(i, &master_fds);

                                    //depois de remover o respectivo client, decremento 1 dos client conectados simultaneamente
                                    active_connections_SE--;
                                    
                                } else {
                                    snprintf(resposta, BUFSZ, "ERROR 02\n");
                                    send(i, resposta, strlen(resposta), 0);
                                }
                            } else if (atoi(argv[2]) == 54321) {
                                int achou = 0;
                                for (int j = 0; j < MAX_CLIENTS; j++) {
                                    if (clientes_SEII[j].id == id) {
                                        achou = 1;
                                        clientes_SEII[j].id = 0;
                                        break;
                                    }
                                }
                                if (achou) {
                                    snprintf(resposta, BUFSZ, "OK 01\n");
                                    send(i, resposta, strlen(resposta), 0);
                                    printf("Servidor SCII Client %d removed\n\n", id);
                                    close(i);
                                    FD_CLR(i, &master_fds);
                                    active_connections_SEII--;
                                    
                                } else {
                                    snprintf(resposta, BUFSZ, "ERROR 02\n");
                                    send(i, resposta, strlen(resposta), 0);
                                }
                            }

                        //se a mensagem for REQ_INFOSE, o servidor SE retorna o valor de produção
                        }else if (strncmp(buffer, "REQ_INFOSE", 10) == 0) {
                            sprintf(resposta, "RES_INFOSE %d kWh", producao);
                            printf("%s\n\n", resposta);
                            send(i, resposta, strlen(resposta), 0);
                        //se a mensagem for REQ_INFOSCII, o servidor scii retorna o valor de consumo
                        }else if (strncmp(buffer, "REQ_INFOSCII", 12) == 0) {
                            sprintf(resposta, "RES_INFOSCII %d%%", consumo);
                            printf("%s\n\n", resposta);
                            send(i, resposta, strlen(resposta), 0);

                        //se for REQ_STATUS o servidor verifica a condição da produção, dependendo do valor da produção o servidor manda uma devida mensagem para o client
                        }else if (strncmp(buffer, "REQ_STATUS", 10) == 0){
                            if (producao >= 41){
                                snprintf(resposta, BUFSZ, "RES_STATUS alta");
                                printf("%s\n\n", resposta);
                                send(i, resposta, strlen(resposta), 0);
                            }else if (producao >= 31 && producao <= 40 ){
                                snprintf(resposta, BUFSZ, "RES_STATUS moderada");
                                printf("%s\n\n", resposta);
                                send(i, resposta, strlen(resposta), 0);
                            }else if (producao >= 20 && producao <= 30){
                                snprintf(resposta, BUFSZ, "RES_STATUS baixa");
                                printf("%s\n\n", resposta);
                                send(i, resposta, strlen(resposta), 0);
                            } 
                            producao = 20 + rand() % 31;
                        //se o client mandar REQ_UP, o valor do consumo atual é aumentador aleatóriamente, e então o servidor manda o consumo antigo e o consumo novo
                        }else if (strncmp(buffer, "REQ_UP", 6) == 0){
                            old_value = consumo;
                            consumo = old_value + rand() % (101 - old_value);
                            sprintf(resposta, "RES_UP %d %d", old_value, consumo);
                            printf("%s\n\n", resposta);
                            send(i, resposta, strlen(resposta), 0);
                        
                        //se o client mandar REQ_NONE, o valor do consumo atual é mantido, e então o servidor manda o consumo antigo
                        }else if (strncmp(buffer, "REQ_NONE", 8) == 0){
                            sprintf(resposta, "RES_NONE %d", consumo);
                            printf("%s\n\n", resposta);
                            send(i, resposta, strlen(resposta), 0);
                        
                        //se o client mandar REQ_DOWN, o valor do consumo atual é diminuido aleatóriamente, e então o servidor manda o consumo antigo e o consumo novo
                        }else if (strncmp(buffer, "REQ_DOWN", 8) == 0){
                            old_value = consumo;
                            consumo = rand() % (old_value + 1);
                            sprintf(resposta, "RES_DOWN %d %d", old_value, consumo);
                            printf("%s\n\n", resposta);
                            send(i, resposta, strlen(resposta), 0);
                        }
                        //limpa o buffer e a resposta
                        memset(buffer, 0, sizeof(buffer));
                        memset(resposta, 0, sizeof(resposta));
                    }
                }
            }
        }
    }

    return 0;
}