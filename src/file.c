#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void sanitize_path(const char* requested_path, char* sanitized_path, size_t buffer_size) {
    const char* web_root = "/mnt/c/Users/ajarry/Dev/proto-http-server-c/www";
    snprintf(sanitized_path, buffer_size, "%s%s", web_root, requested_path);

    // prevent directory traversal by normalizing the path
    if (strstr(sanitized_path, "..")) {
        strncpy(sanitized_path, "/mnt/c/Users/ajarry/Dev/proto-http-server-c/www/404.html", buffer_size - 1); // serve a 404 page
    }
}

void serve_file(const char* path, Http_Response* response) {
    FILE* file = fopen(path, "rb+");
    if(file == NULL) {
        response->status_code = 404;
        strncpy(response->reason_phrase, "Not Found", sizeof(response->reason_phrase) - 1);

        serve_file("/mnt/c/Users/ajarry/Dev/proto-http-server-c/www/404.html", response);
        return;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* file_content = malloc(file_size + 1);
    if (file_content == NULL) {
        perror("failed to allocate memory for file content");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fread(file_content, 1, file_size, file);
    fclose(file);
    file_content[file_size] = '\0';

    response->body     = file_content;
    response->body_len = file_size;

    if (strstr(path, ".html")) {
        http_response_header_add(response, "Content-Type", "text/html");
    } else if(strstr(path, ".css")) {
        http_response_header_add(response, "Content-Type", "text/css");
    } else if(strstr(path, ".js")) {
        http_response_header_add(response, "Content-Type", "application/javascript");
    } else if(strstr(path, ".png")) {
        http_response_header_add(response, "Content-Type", "image/png");
    } else {
        http_response_header_add(response, "Content-Type", "application/octet-stream");
    }

    char content_len[32];
    snprintf(content_len, sizeof(content_len), "%zu", file_size);
    http_response_header_add(response, "Content-Length", content_len);
}

