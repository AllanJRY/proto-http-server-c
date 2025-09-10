#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "common.h"

#include "tcp.c"

#define SERVER_PORT 8080

int main(void) {
    Tcp_Server server = {0};
    if (tcp_server_bind(&server, SERVER_PORT) != SERVER_OK) {
        printf("server initialization failed\n");
        exit(EXIT_FAILURE);
    }

    i32 client_fd = handle_connection(server.socket_fd);
    if (client_fd == -1) {
        printf("server failed to handle client connection\n");
        close(server.socket_fd);
        exit(EXIT_FAILURE);
    }

    close(client_fd);
    close(server.socket_fd);

    return 0;
}

