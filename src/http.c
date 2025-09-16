#include <ctype.h>
#include <string.h>

#include "common.h"

#define MAX_LEN_HTTP_REQUEST        8192 * 4
#define MAX_LEN_HTTP_REQUEST_HEADER 8192
#define MAX_LEN_HTTP_METHOD         8
#define MAX_LEN_HTTP_PATH           2048
#define MAX_LEN_HTTP_PROTOCOL       16

typedef struct Http_Header {
    char key[256];
    char value[512];
} Http_Header;

typedef enum Http_Request_Parse_Result {
    HTTP_PARSE_OK,
    HTTP_PARSE_INVALID,
} Http_Request_Parse_Result;

typedef struct Http_Request {
    char         method[MAX_LEN_HTTP_METHOD];
    char         path[MAX_LEN_HTTP_PATH];
    char         protocol[MAX_LEN_HTTP_PROTOCOL];
    Http_Header* headers; // should evolved to become hash table.
    u32          headers_cap;
    u32          headers_count;
} Http_Request;

typedef struct Http_Response {
    u32          status_code;
    char         reason_phrase[64];
    Http_Header* headers; // should evolved to become hash table.
    u32          headers_cap;
    u32          headers_count;
    char*        body; // or u8 ?
    size_t       body_len;
} Http_Response;

// FIXME: could exceed buffer size.
static u32 _read_word(char** buf_offset) {
    u32 word_len = 0;

    while (!isspace(**buf_offset)) {
        *buf_offset += 1;
        word_len    += 1;
    }

    return word_len;
}

// FIXME: could exceed buffer size.
static u32 _read_until(char** buf_offset, char needle) {
    u32 word_len = 0;

    while (**buf_offset != needle) {
        *buf_offset += 1;
        word_len    += 1;
    }

    return word_len;
}

// FIXME: could exceed buffer size.
static u32 _read_until_str(char** buf_offset, const char* str_needle, u32 str_needle_len) {
    u32 str_len = 0;

    while (memcmp(*buf_offset, str_needle, str_needle_len) != 0) {
        *buf_offset += 1;
        str_len    += 1;
    }

    return str_len;
}

// TODO: better handle invalid raw_req_buffer data
Http_Request_Parse_Result http_request_read(char* raw_req_buffer, Http_Request* request) {

    // TODO: for loop perform better than sscanf?
    // Can we avoid copying the data (need to be sure raw request buffer live long enough) ?
    // Compare my way of doing this against lowlevel to see where I can improve

    char* buf_offset = raw_req_buffer;

    // start parse request line
    u32 word_len =_read_word(&buf_offset);
    if (word_len >= sizeof(request->method)) word_len = sizeof(request->method) - 1;
    strncpy(request->method, buf_offset - word_len, word_len);
    request->method[word_len] = '\0';

    while (isspace(*buf_offset)) buf_offset += 1;

    word_len =_read_word(&buf_offset);
    if (word_len >= sizeof(request->path)) word_len = sizeof(request->path) - 1;
    strncpy(request->path, buf_offset - word_len, word_len);
    request->path[word_len] = '\0';

    while (isspace(*buf_offset)) buf_offset += 1;

    word_len =_read_word(&buf_offset);
    if (word_len >= sizeof(request->protocol)) word_len = sizeof(request->protocol) - 1;
    strncpy(request->protocol, buf_offset - word_len, word_len);
    request->protocol[word_len] = '\0';

    while (isspace(*buf_offset)) buf_offset += 1;
    // end parse request line

    // start parse headers
    // TODO: use realloc on both
    if(request->headers == NULL) {
        request->headers = malloc(8 * sizeof(Http_Header));
        if (request->headers == NULL) {
            perror("headers_alloc");
            exit(EXIT_FAILURE);
        }

        memset(request->headers, 0, 8 * sizeof(Http_Header));

        request->headers_cap   = 8;
        request->headers_count = 0;
    }

    while (memcmp(buf_offset, "\r\n\r\n", 4) != 0) {
        while (isspace(*buf_offset)) buf_offset += 1;

        if (request->headers_cap == request->headers_count) {
            request->headers = realloc(request->headers, (request->headers_cap * 2) * sizeof(Http_Header));
            if (request->headers == NULL) {
                perror("headers_alloc");
                exit(EXIT_FAILURE);
            }

            memset(request->headers + request->headers_cap, 0, request->headers_cap * sizeof(Http_Header));

            request->headers_cap *= 2;
        }


        word_len =_read_until(&buf_offset, ':');
        strncpy(request->headers[request->headers_count].key, buf_offset - word_len, sizeof(request->headers[request->headers_count].key) - 1);

        buf_offset += 1;
        while (isspace(*buf_offset)) buf_offset += 1;

        word_len =_read_until_str(&buf_offset, "\r\n", 2);
        strncpy(request->headers[request->headers_count].value, buf_offset - word_len, sizeof(request->headers[request->headers_count].value) - 1);

        request->headers_count += 1;
    }
    // end parse headers

    return HTTP_PARSE_OK;
}

void http_response_init(Http_Response* response) {
    response->status_code = 200;
    strncpy(response->reason_phrase, "OK", sizeof(response->reason_phrase) - 1);
    response->headers       = NULL;
    response->headers_cap   = 0;
    response->headers_count = 0;
    response->body          = NULL;
    response->body_len      = 0;
}

void http_response_deinit(Http_Response* response) {
    if (response->headers != NULL) {
        free(response->headers);
    }
    response->headers_cap   = 0;
    response->headers_count = 0;
}

void http_response_header_add(Http_Response* response, const char* key, const char* value) {
    if (response->headers == NULL || response->headers_count == response->headers_cap) {
        u32 new_cap = response->headers_cap;
        if (new_cap == 0) { new_cap = 8;  }
        else              { new_cap *= 2; }

        response->headers = realloc(response->headers, new_cap * sizeof(Http_Header));

        if (response->headers == NULL) {
            perror("http_response_add_header_alloc");
            exit(EXIT_FAILURE);
        }

        memset(response->headers + response->headers_cap, 0, (new_cap - response->headers_cap) * sizeof(Http_Header));
        response->headers_cap = new_cap;
    }

    strncpy(response->headers[response->headers_count].key, key, sizeof(response->headers[response->headers_count].key) - 1);
    strncpy(response->headers[response->headers_count].value, value, sizeof(response->headers[response->headers_count].value) - 1);
    response->headers_count += 1;
}

char* http_response_build(const Http_Response* response, size_t* response_len) {
    size_t buffer_size = 1024;
    char* buffer       = malloc(buffer_size);

    if (buffer == NULL) {
        perror("http_response_build_alloc");
        exit(EXIT_FAILURE);
    }

    size_t offset = snprintf(buffer, buffer_size, "HTTP/1.1 %d %s\r\n", response->status_code, response->reason_phrase);

    for (size_t i = 0; i < response->headers_count; i += 1) {
        // by using NULL and 0 has buffer and buffer_size, it doesn't write to any buffer by give us the expected size of 
        // string, which is useful in our case to check if we will overflow the buffer.
        size_t header_len = snprintf(NULL, 0, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);

        // LowLevel's solution, but doesn't seems to be a that a good one.. lots of potential reallocation.
        while(offset + header_len + 1 > buffer_size) {
            buffer_size += 2;
            buffer = realloc(buffer, buffer_size);
            if (buffer == NULL) {
                perror("http_response_build_realloc");
                exit(EXIT_FAILURE);
            }
        }

        offset += snprintf(buffer + offset, buffer_size, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);
    }
    
    offset += snprintf(buffer + offset, buffer_size - offset, "\r\n");

    if (response->body != NULL) {
        // LowLevel's solution, but doesn't seems to be a that a good one.. lots of potential reallocation.
        while(offset + response->body_len + 1 > buffer_size) {
            buffer_size += 2;
            buffer = realloc(buffer, buffer_size);
            if (buffer == NULL) {
                perror("http_response_build_realloc");
                exit(EXIT_FAILURE);
            }
        }
        memcpy(buffer + offset, response->body, response->body_len);
        offset += response->body_len;
    }

    *response_len = offset;
    return buffer;
}

void http_response_send(int client_fd, const Http_Response* response) {
    size_t response_len = 0;
    char* response_data = http_response_build(response, &response_len);

    size_t total_sent = 0;
    while(total_sent < response_len) {
        ssize_t bytes_sent = send(client_fd, response_data + total_sent, response_len - total_sent, 0);
        if (bytes_sent <= 0) {
            perror("http_response_send");
            break;
        }
        total_sent -= bytes_sent;
    }

    free(response_data);
}


