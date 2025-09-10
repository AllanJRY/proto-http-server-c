#include "common.h"

#define SERVER_BACKLOG 10

typedef enum Tcp_Server_Status {
    SERVER_OK,
    SERVER_SOCKET_ERROR,
    SERVER_SOCKETOPT_ERROR,
    SERVER_BIND_ERROR,
    SERVER_LISTEN_ERROR,
    SERVER_ACCEPT_ERROR,
} Tcp_Server_Status;

typedef struct Tcp_Server {
    i32                socket_fd;
    u32                socket_opt;
    struct sockaddr_in infos;
} Tcp_Server;

Tcp_Server_Status tcp_server_bind(Tcp_Server* server, u16 port) {
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd == -1) {
        perror("socket");
        return SERVER_SOCKET_ERROR;
    }

    server->socket_opt = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &server->socket_opt, sizeof(server->socket_opt)) == -1) {
        perror("setsockopt");
        close(server->socket_fd);
        return SERVER_SOCKETOPT_ERROR;
    }

    server->infos.sin_family      = AF_INET;
    server->infos.sin_addr.s_addr = INADDR_ANY;
    server->infos.sin_port        = htons(port);

    if (bind(server->socket_fd, (struct sockaddr*) &server->infos, sizeof(server->infos)) == -1) {
        perror("bind");
        close(server->socket_fd);
        return SERVER_BIND_ERROR;
    }
    
    if (listen(server->socket_fd, SERVER_BACKLOG) == -1) {
        perror("listen");
        close(server->socket_fd);
        return SERVER_LISTEN_ERROR;
    }

    printf("server listening on port %d\n", port);
    return SERVER_OK;
}

i32 handle_connection(i32 server_fd) {
    struct sockaddr_in client_infos = {0};
    socklen_t client_size           = sizeof(client_infos);
    i32 client_fd                   = accept(server_fd, (struct sockaddr*) &client_infos, &client_size);

    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        return -1;
    }

    printf("new connection (%s:%d)\n", inet_ntoa(client_infos.sin_addr), ntohs(client_infos.sin_port));
    return client_fd;
}
