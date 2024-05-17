#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

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

typedef struct {
    int id;
} Cliente;

int main(int argc, char **argv) {
    if (argc != 3) {
        usage(argc, argv);
    }

    srand(time(NULL));

    // Gera um número aleatório entre 20 e 50
    int producao = 20 + rand() % 31;

    Cliente clientes_SE[MAX_CLIENTS];
    Cliente clientes_SEII[MAX_CLIENTS];

    for (int l = 0; l < MAX_CLIENTS; l++) {
        clientes_SEII[l].id = 0;
    }

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

    // Escutando conexões
    if (0 != listen(server_socket, MAX_CLIENTS)) {
        perror("listen");
        exit(1);
    }

    fd_set read_fds, master_fds;
    FD_ZERO(&master_fds);
    FD_SET(server_socket, &master_fds);
    int fdmax = server_socket;

    char portstr[BUFSZ];
    addrtostr(addr, portstr, BUFSZ);
    printf("Servidor na porta %s em execução. Aguardando conexões...\n", portstr);

    while (1) {
        read_fds = master_fds;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) {
                    struct sockaddr_storage cstorage;
                    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
                    socklen_t caddrlen = sizeof(cstorage);

                    int client_socket = accept(server_socket, caddr, &caddrlen);
                    if (client_socket == -1) {
                        perror("accept");
                    } else {
                        // Verifica o servidor com base na porta
                        if (atoi(argv[2]) == 12345) { // Servidor SE
                            if (active_connections_SE >= 2) {
                                printf("Client limit exceeded\n");
                                send(client_socket, "ERROR(01)\n", strlen("ERROR(01)\n"), 0);
                                close(client_socket);
                            } else {
                                FD_SET(client_socket, &master_fds);
                                if (client_socket > fdmax) {
                                    fdmax = client_socket;
                                }
                                char addrstr[BUFSZ];
                                addrtostr(caddr, addrstr, BUFSZ);
                                printf("Nova conexão de %s no socket %d (Servidor SE)\n", addrstr, client_socket);
                                send(client_socket, "conectou", strlen("conectou"), 0);
                                active_connections_SE++;
                            }
                        } else if (atoi(argv[2]) == 54321) { // Servidor SEII
                            if (active_connections_SEII >= 2) {
                                printf("Client limit exceeded\n");
                                send(client_socket, "ERROR(01)\n", strlen("ERROR(01)\n"), 0);
                                close(client_socket);
                            } else {
                                FD_SET(client_socket, &master_fds);
                                if (client_socket > fdmax) {
                                    fdmax = client_socket;
                                }
                                char addrstr[BUFSZ];
                                addrtostr(caddr, addrstr, BUFSZ);
                                printf("Nova conexão de %s no socket %d (Servidor SEII)\n", addrstr, client_socket);
                                send(client_socket, "conectou", strlen("conectou"), 0);
                                active_connections_SEII++;
                            }
                        }
                    }
                } else {
                    char resposta[BUFSZ];
                    char buffer[BUFSZ];
                    int bytes_received = recv(i, buffer, sizeof(buffer), 0);
                    if (bytes_received <= 0) {
                        if (bytes_received == 0) {
                            printf("Socket %d encerrou a conexão\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_fds);
                        if (atoi(argv[2]) == 12345) { // Servidor SE
                            active_connections_SE--;
                        } else if (atoi(argv[2]) == 54321) { // Servidor SEII
                            active_connections_SEII--;
                        }
                    } else {
                        buffer[bytes_received] = '\0';
                        printf("Mensagem recebida do socket %d: %s\n", i, buffer);
                        if (strncmp(buffer, "REQ_ADD", 7) == 0) {
                            if (atoi(argv[2]) == 12345) {
                                int client_id = i;
                                snprintf(resposta, BUFSZ, "RES_ADD %d\n", client_id);
                                send(i, resposta, strlen(resposta), 0);
                                printf("Client %d added\n", client_id);
                                for (int p = 0; p < MAX_CLIENTS; p++) {
                                    if (clientes_SE[p].id == 0) {
                                        clientes_SE[p].id = i;
                                        break;
                                    }
                                }
                                printf("Lista de Clientes do servidor SE: ");
                                for (int i = 0; i < MAX_CLIENTS; i++) {
                                    printf("%d ", clientes_SE[i].id);
                                }
                                printf("\n");
                            } else if (atoi(argv[2]) == 54321) {
                                int client_id2 = i;
                                snprintf(resposta, BUFSZ, "RES_ADD %d\n", client_id2);
                                send(i, resposta, strlen(resposta), 0);
                                for (int p = 0; p < MAX_CLIENTS; p++) {
                                    if (clientes_SEII[p].id == 0) {
                                        clientes_SEII[p].id = i;
                                        break;
                                    }
                                }
                                printf("Lista de Clientes do servidor SEII: ");
                                for (int i = 0; i < MAX_CLIENTS; i++) {
                                    printf("%d ", clientes_SEII[i].id);
                                }
                                printf("\n");
                            }

                        } else if (strncmp(buffer, "REQ_REM", 7) == 0) {
                            int id;
                            sscanf(buffer, "REQ_REM %d", &id);
                            if (atoi(argv[2]) == 12345) {
                                int achou = 0;
                                for (int j = 0; j < MAX_CLIENTS; j++) {
                                    if (clientes_SE[j].id == id) {
                                        achou = 1;
                                        clientes_SE[j].id = 0;
                                        break;
                                    }
                                }
                                if (achou) {
                                    snprintf(resposta, BUFSZ, "OK 01\n");
                                    send(i, resposta, strlen(resposta), 0);
                                    printf("Servidor SE Client %d removed\n", id);
                                    close(i);
                                    FD_CLR(i, &master_fds);
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
                                    printf("Servidor SCII Client %d removed\n", id);
                                    close(i);
                                    FD_CLR(i, &master_fds);
                                    active_connections_SEII--;
                                    
                                } else {
                                    snprintf(resposta, BUFSZ, "ERROR 02\n");
                                    send(i, resposta, strlen(resposta), 0);
                                }
                            }
                        }else if (strncmp(buffer, "REQ_INFOSE", 10) == 0) {
                            sprintf(resposta, "RES_INFOSE %d kWh", producao);
                            printf("%s\n", resposta);
                            send(i, resposta, strlen(resposta), 0);
                        }
                    }
                }
            }
        }
    }

    return 0;
}