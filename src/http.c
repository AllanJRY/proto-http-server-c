#include <ctype.h>
#include <string.h>

#include "common.h"

#define MAX_LEN_HTTP_REQUEST        8192 * 4
#define MAX_LEN_HTTP_REQUEST_HEADER 8192
#define MAX_LEN_HTTP_METHOD         8
#define MAX_LEN_HTTP_PATH           2048
#define MAX_LEN_HTTP_PROTOCOL       16

typedef enum Http_Request_Parse_Result {
    HTTP_PARSE_OK,
    HTTP_PARSE_INVALID,
} Http_Request_Parse_Result;

typedef struct Http_Header {
    char key[256];
    char value[512];
} Http_Header;

typedef struct Http_Request {
    char         method[MAX_LEN_HTTP_METHOD];
    char         path[MAX_LEN_HTTP_PATH];
    char         protocol[MAX_LEN_HTTP_PROTOCOL];
    Http_Header* headers; // should evolved to become hash table.
    u32          headers_cap;
    u32          headers_count;
} Http_Request;

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
    strncpy(request->method, buf_offset - word_len, word_len); // verify word_len

    while (isspace(*buf_offset)) buf_offset += 1;

    word_len =_read_word(&buf_offset);
    strncpy(request->path, buf_offset - word_len, word_len); // verify word_len

    while (isspace(*buf_offset)) buf_offset += 1;

    word_len =_read_word(&buf_offset);
    strncpy(request->protocol, buf_offset - word_len, word_len); // verify word_len

    while (isspace(*buf_offset)) buf_offset += 1;
    // end parse request line

    // start parse headers
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
        strncpy(request->headers[request->headers_count].key, buf_offset - word_len, word_len);

        buf_offset += 1;
        while (isspace(*buf_offset)) buf_offset += 1;

        word_len =_read_until_str(&buf_offset, "\r\n", 2);
        strncpy(request->headers[request->headers_count].value, buf_offset - word_len, word_len);

        request->headers_count += 1;
    }
    // end parse headers

    return HTTP_PARSE_OK;
}
