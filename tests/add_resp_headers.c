#include <stdio.h>
#include <stdlib.h>

#include "../src/common.h"
#include "../src/http.c"

int main(void) {
    Http_Response response = {0};
    http_response_init(&response);

    http_response_header_add(&response, "Content-Type", "text/html");
    http_response_header_add(&response, "Connection", "close");

    printf("HTTP Response headers:\n");
    for (u32 i = 0; i < response.headers_count; i += 1) {
        printf("\t%s: %s\n", response.headers[i].key, response.headers[i].value);
    }

    http_response_deinit(&response);

    return 0;
}
