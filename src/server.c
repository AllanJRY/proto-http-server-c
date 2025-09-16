#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "common.h"

#include "tcp.c"
#include "http.c"

#define SERVER_PORT 8080

int main(void) {
    Tcp_Server server = {0};
    if (tcp_server_bind(&server, SERVER_PORT) != SERVER_OK) {
        printf("server initialization failed\n");
        exit(EXIT_FAILURE);
    }

    while(true) {
        i32 client_fd = handle_connection(server.socket_fd);
        if (client_fd == -1) {
            printf("server failed to handle client connection\n");
            close(server.socket_fd);
            exit(EXIT_FAILURE);
        }

        // TODO: put it in Http_Request struct ?
        char buffer[MAX_LEN_HTTP_REQUEST] = {0};
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            printf("server failed to read the client socket msg\n");
            close(server.socket_fd);
            exit(EXIT_FAILURE);
        }
        buffer[bytes_read] = '\0';

        Http_Request request = {0};
        if (http_request_read(buffer, &request) != HTTP_PARSE_OK) {
            printf("server failed to read the http request\n");
            close(server.socket_fd);
            exit(EXIT_FAILURE);
        }

        printf("request received (method=%s path=%s protocol=%s)\n", request.method, request.path, request.protocol);

        Http_Response response;
        http_response_init(&response);
        http_response_header_add(&response, "Content-Type", "text/html");
        http_response_header_add(&response, "Connection", "close");

        response.body = "<html><body><h1>Hello, World!</h1></body></html>";
        response.body_len = strlen((char*) response.body);

        http_response_send(client_fd, &response);

        http_response_deinit(&response);
        free(request.headers);

        close(client_fd);
        
        printf("response send and client connection closed\n");
    }

    close(server.socket_fd);

    return 0;
}

