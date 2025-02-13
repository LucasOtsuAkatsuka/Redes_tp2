# Redes TP2

## Descrição

Este repositório contém a implementação do trabalho prático 2 da disciplina de Redes de Computadores. O objetivo do projeto é desenvolver um sistema cliente-servidor que gerencia conexões simultâneas e troca de informações entre clientes e dois servidores distintos.

## Tecnologias Utilizadas

- **Linguagem**: C
- **Bibliotecas/Frameworks**: socket, netinet/in.h, arpa/inet.h, unistd.h, string.h, sys/socket.h, sys/select.h
- **Protocolo de comunicação**: TCP

## Estrutura do Projeto

```
Redes_tp2/
│-- server.c         # Implementação do servidor
│-- client.c         # Implementação do cliente
│-- README.md           # Documentação do projeto
```

## Como Executar

### Requisitos

- Compilador GCC instalado
- Sistema operacional Linux ou compatível com sockets POSIX

### Compilação

Compile os arquivos fonte com o GCC:

```sh
gcc server.c -o server
gcc client.c -o client
```

### Execução

1. **Iniciar o servidor:**

   ```sh
   ./server <v4|v6> <port>
   ```

   Exemplo:

   ```sh
   ./server v4 12345
   ```

2. **Iniciar o cliente:**

   ```sh
   ./client <serverIP> <server port1> <server port2>
   ```

   Exemplo:

   ```sh
   ./client ::1 12345 54321
   ```

## Funcionalidades

### Servidor

- Aceita conexões simultâneas de até 10 clientes por servidor.
- Possui dois servidores operando em portas diferentes (12345 e 54321).
- Gerencia clientes através de identificadores únicos.
- Responde a solicitações dos clientes, incluindo:
  - `REQ_ADD`: Adiciona um cliente e retorna um ID.
  - `REQ_REM <id>`: Remove um cliente.
  - `REQ_INFOSE`: Retorna o valor atual de produção de energia.
  - `REQ_INFOSCII`: Retorna o consumo atual de energia.
  - `REQ_STATUS`: Retorna o estado da produção de energia (alta, moderada ou baixa).
  - `REQ_UP`: Aumenta o consumo de energia.
  - `REQ_NONE`: Mantém o consumo atual.
  - `REQ_DOWN`: Reduz o consumo de energia.

### Cliente

- Conecta-se simultaneamente aos dois servidores.
- Envia comandos para interagir com os servidores.
- Permite as seguintes interações:
  - `kill`: Solicita a remoção do cliente nos servidores.
  - `display info se`: Solicita a produção atual de energia ao servidor SE.
  - `display info scii`: Solicita o consumo atual de energia ao servidor SEII.
  - `query condition`: Solicita o estado da produção e ajusta o consumo baseado na resposta.

## Autores

- **Lucas Jun Otsu Akatsuka**


