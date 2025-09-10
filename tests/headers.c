#include <stdio.h>
#include <stdlib.h>

#include "../src/common.h"
#include "../src/http.c"

int main(void) {
    char* raw_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: curl/8.5.0\r\n"
        "Accept: */*\r\n"
        "\r\n";

    Http_Request request = {0};
    http_request_read(raw_request, &request);

    printf("parsed HTTP headers: \n");
    for (u32 i = 0; i < request.headers_count; i += 1) {
        printf("\t%s='%s'\n", request.headers[i].key, request.headers[i].value);
    }

    free(request.headers);

    return 0;
}
