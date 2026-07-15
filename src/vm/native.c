#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include "vm/native.h"
#include "core/object.h"
#include "core/memory.h"

static bool native_print(int arg_count, Value* args, Value* result) {
    for (int i = 0; i < arg_count; i++) {
        if (i > 0) printf(" ");
        print_value(args[i]);
    }
    printf("\n");
    *result = NIL_VAL;
    return true;
}

static bool native_input(int arg_count, Value* args, Value* result) {
    if (arg_count > 0 && IS_STRING(args[0])) {
        printf("%s", AS_CSTRING(args[0]));
        fflush(stdout);
    }

    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[--len] = '\0';
        }

        if (len == 4 && memcmp(buffer, "true", 4) == 0) {
            *result = BOOL_VAL(true);
        } else if (len == 5 && memcmp(buffer, "false", 5) == 0) {
            *result = BOOL_VAL(false);
        } else if (len == 3 && memcmp(buffer, "nil", 3) == 0) {
            *result = NIL_VAL;
        } else {
            char* end;
            double num = strtod(buffer, &end);
            if (*end == '\0' && end != buffer) {
                *result = NUMBER_VAL(num);
            } else {
                *result = OBJ_VAL(copy_string(buffer, (int)len));
            }
        }
    } else {
        *result = OBJ_VAL(copy_string("", 0));
    }
    return true;
}

static bool native_len(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: len() expects 1 argument\n");
        return false;
    }
    if (IS_STRING(args[0])) {
        *result = NUMBER_VAL((double)AS_STRING(args[0])->length);
        return true;
    }
    if (IS_LIST(args[0])) {
        *result = NUMBER_VAL((double)AS_LIST(args[0])->count);
        return true;
    }
    if (IS_TENSOR(args[0])) {
        *result = NUMBER_VAL((double)AS_TENSOR(args[0])->size);
        return true;
    }
    if (IS_MATRIX(args[0])) {
        *result = NUMBER_VAL((double)(AS_MATRIX(args[0])->rows * AS_MATRIX(args[0])->cols));
        return true;
    }
    fprintf(stderr, "JTS GO: len() argument must be a string, list, tensor, or matrix\n");
    return false;
}

static bool native_type(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: type() expects 1 argument\n");
        return false;
    }
    const char* type_name = "unknown";
    switch (args[0].type) {
        case VAL_NIL:     type_name = "nil"; break;
        case VAL_BOOL:    type_name = "boolean"; break;
        case VAL_NUMBER:  type_name = "number"; break;
        case VAL_OBJ:
            if (IS_STRING(args[0])) {
                type_name = "string";
            } else if (IS_FUNCTION(args[0])) {
                type_name = "function";
            } else if (IS_LIST(args[0])) {
                type_name = "list";
            } else if (IS_DICT(args[0])) {
                type_name = "dict";
            } else if (IS_CLASS(args[0])) {
                type_name = "class";
            } else if (IS_INSTANCE(args[0])) {
                type_name = "instance";
            } else if (IS_TENSOR(args[0])) {
                type_name = "tensor";
            } else if (IS_MATRIX(args[0])) {
                type_name = "matrix";
            } else if (IS_HTTP_SERVER(args[0])) {
                type_name = "http_server";
            } else {
                type_name = "object";
            }
            break;
    }
    *result = OBJ_VAL(copy_string(type_name, (int)strlen(type_name)));
    return true;
}

static bool native_append(int arg_count, Value* args, Value* result) {
    if (arg_count != 2) {
        fprintf(stderr, "JTS GO: append() expects 2 arguments (list, value)\n");
        return false;
    }
    if (!IS_LIST(args[0])) {
        fprintf(stderr, "JTS GO: append() first argument must be a list\n");
        return false;
    }
    list_append(AS_LIST(args[0]), args[1]);
    *result = args[0];
    return true;
}

static bool native_number(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: number() expects 1 argument\n");
        return false;
    }
    if (IS_NUMBER(args[0])) {
        *result = args[0];
        return true;
    }
    if (IS_STRING(args[0])) {
        char* end;
        double value = strtod(AS_CSTRING(args[0]), &end);
        if (*end != '\0') {
            fprintf(stderr, "JTS GO: number() invalid conversion from '%s'\n", AS_CSTRING(args[0]));
            return false;
        }
        *result = NUMBER_VAL(value);
        return true;
    }
    fprintf(stderr, "JTS GO: number() argument must be a string or number\n");
    return false;
}

static bool native_string(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: str() expects 1 argument\n");
        return false;
    }
    if (IS_STRING(args[0])) {
        *result = args[0];
        return true;
    }
    if (IS_NUMBER(args[0])) {
        Value str = number_to_string(AS_NUMBER(args[0]));
        *result = str;
        return true;
    }
    if (IS_BOOL(args[0])) {
        if (AS_BOOL(args[0])) {
            *result = OBJ_VAL(copy_string("true", 4));
        } else {
            *result = OBJ_VAL(copy_string("false", 5));
        }
        return true;
    }
    if (IS_NIL(args[0])) {
        *result = OBJ_VAL(copy_string("nil", 3));
        return true;
    }
    if (IS_TENSOR(args[0])) {
        ObjTensor* t = AS_TENSOR(args[0]);
        char buf[256];
        int pos = 0;
        buf[pos++] = '[';
        for (int i = 0; i < t->size && pos < 250; i++) {
            if (i > 0) { buf[pos++] = ','; buf[pos++] = ' '; }
            Value v = NUMBER_VAL(t->data[i]);
            ObjString* s = AS_STRING(number_to_string(AS_NUMBER(v)));
            int copy_len = s->length;
            if (pos + copy_len > 250) copy_len = 250 - pos;
            memcpy(buf + pos, s->chars, copy_len);
            pos += copy_len;
        }
        buf[pos++] = ']';
        buf[pos] = '\0';
        *result = OBJ_VAL(copy_string(buf, pos));
        return true;
    }
    if (IS_MATRIX(args[0])) {
        ObjMatrix* m = AS_MATRIX(args[0]);
        char buf[512];
        int pos = 0;
        buf[pos++] = '[';
        for (int i = 0; i < m->rows && pos < 500; i++) {
            if (i > 0) { buf[pos++] = '\n'; }
            buf[pos++] = '[';
            for (int j = 0; j < m->cols && pos < 500; j++) {
                if (j > 0) { buf[pos++] = ','; buf[pos++] = ' '; }
                Value v = NUMBER_VAL(m->data[i][j]);
                ObjString* s = AS_STRING(number_to_string(AS_NUMBER(v)));
                int copy_len = s->length;
                if (pos + copy_len > 500) copy_len = 500 - pos;
                memcpy(buf + pos, s->chars, copy_len);
                pos += copy_len;
            }
            buf[pos++] = ']';
        }
        buf[pos++] = ']';
        buf[pos] = '\0';
        *result = OBJ_VAL(copy_string(buf, pos));
        return true;
    }
    if (IS_LIST(args[0])) {
        ObjList* list = AS_LIST(args[0]);
        char buf[1024];
        int pos = 0;
        buf[pos++] = '[';
        for (int i = 0; i < list->count && pos < 1020; i++) {
            if (i > 0) { buf[pos++] = ','; buf[pos++] = ' '; }
            if (IS_STRING(list->values[i])) {
                ObjString* s = AS_STRING(list->values[i]);
                buf[pos++] = '"';
                int copy_len = s->length;
                if (pos + copy_len + 1 > 1020) copy_len = 1020 - pos - 1;
                memcpy(buf + pos, s->chars, copy_len);
                pos += copy_len;
                buf[pos++] = '"';
            } else if (IS_NUMBER(list->values[i])) {
                Value v = NUMBER_VAL(AS_NUMBER(list->values[i]));
                ObjString* s = AS_STRING(number_to_string(AS_NUMBER(v)));
                int copy_len = s->length;
                if (pos + copy_len > 1020) copy_len = 1020 - pos;
                memcpy(buf + pos, s->chars, copy_len);
                pos += copy_len;
            } else if (IS_BOOL(list->values[i])) {
                if (AS_BOOL(list->values[i])) {
                    memcpy(buf + pos, "true", 4); pos += 4;
                } else {
                    memcpy(buf + pos, "false", 5); pos += 5;
                }
            } else if (IS_NIL(list->values[i])) {
                memcpy(buf + pos, "nil", 3); pos += 3;
            } else {
                buf[pos++] = '?';
            }
        }
        buf[pos++] = ']';
        buf[pos] = '\0';
        *result = OBJ_VAL(copy_string(buf, pos));
        return true;
    }
    fprintf(stderr, "JTS GO: str() unsupported type\n");
    return false;
}

static bool native_math(int arg_count, Value* args, Value* result) {
    if (arg_count != 2) {
        fprintf(stderr, "JTS GO: math() expects 2 arguments (function, value)\n");
        return false;
    }
    if (!IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: math() first argument must be a string\n");
        return false;
    }
    if (!IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: math() second argument must be a number\n");
        return false;
    }
    const char* func = AS_CSTRING(args[0]);
    double x = AS_NUMBER(args[1]);
    if (strcmp(func, "sin") == 0) *result = NUMBER_VAL(sin(x));
    else if (strcmp(func, "cos") == 0) *result = NUMBER_VAL(cos(x));
    else if (strcmp(func, "tan") == 0) *result = NUMBER_VAL(tan(x));
    else if (strcmp(func, "sqrt") == 0) *result = NUMBER_VAL(sqrt(x));
    else if (strcmp(func, "abs") == 0) *result = NUMBER_VAL(fabs(x));
    else if (strcmp(func, "log") == 0) *result = NUMBER_VAL(log(x));
    else if (strcmp(func, "exp") == 0) *result = NUMBER_VAL(exp(x));
    else if (strcmp(func, "pow") == 0) *result = NUMBER_VAL(pow(x, AS_NUMBER(args[1])));
    else if (strcmp(func, "floor") == 0) *result = NUMBER_VAL(floor(x));
    else if (strcmp(func, "ceil") == 0) *result = NUMBER_VAL(ceil(x));
    else if (strcmp(func, "round") == 0) *result = NUMBER_VAL(round(x));
    else { fprintf(stderr, "JTS GO: unknown math function '%s'\n", func); return false; }
    return true;
}

static bool native_tensor(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: tensor() expects 1 argument (nested list)\n");
        return false;
    }
    if (!IS_LIST(args[0])) {
        fprintf(stderr, "JTS GO: tensor() argument must be a list\n");
        return false;
    }
    ObjList* list = AS_LIST(args[0]);
    int ndim = 1;
    int shape[4];
    shape[0] = list->count;
    int total = list->count;
    double* data = (double*)reallocate(NULL, 0, sizeof(double) * total);
    for (int i = 0; i < list->count; i++) {
        if (IS_NUMBER(list->values[i])) {
            data[i] = AS_NUMBER(list->values[i]);
        } else if (IS_LIST(list->values[i])) {
            ObjList* inner = AS_LIST(list->values[i]);
            shape[1] = inner->count;
            total = shape[0] * shape[1];
            data = (double*)reallocate(data, sizeof(double) * (i), sizeof(double) * total);
            for (int j = 0; j < inner->count; j++) {
                if (IS_NUMBER(inner->values[j])) {
                    data[i * shape[1] + j] = AS_NUMBER(inner->values[j]);
                } else {
                    data[i * shape[1] + j] = 0.0;
                }
            }
            ndim = 2;
        } else {
            data[i] = 0.0;
        }
    }
    *result = OBJ_VAL(new_tensor(ndim, shape, data));
    return true;
}

static bool native_matrix(int arg_count, Value* args, Value* result) {
    if (arg_count < 1) {
        fprintf(stderr, "JTS GO: matrix() expects at least 1 argument\n");
        return false;
    }
    if (!IS_LIST(args[0])) {
        fprintf(stderr, "JTS GO: matrix() argument must be a nested list\n");
        return false;
    }
    ObjList* rows_list = AS_LIST(args[0]);
    int rows = rows_list->count;
    int cols = 0;
    if (rows > 0 && IS_LIST(rows_list->values[0])) {
        cols = AS_LIST(rows_list->values[0])->count;
    }
    double** data = (double**)reallocate(NULL, 0, sizeof(double*) * rows);
    for (int i = 0; i < rows; i++) {
        data[i] = (double*)reallocate(NULL, 0, sizeof(double) * cols);
        if (IS_LIST(rows_list->values[i])) {
            ObjList* row = AS_LIST(rows_list->values[i]);
            for (int j = 0; j < cols; j++) {
                if (j < row->count && IS_NUMBER(row->values[j])) {
                    data[i][j] = AS_NUMBER(row->values[j]);
                } else {
                    data[i][j] = 0.0;
                }
            }
        }
    }
    *result = OBJ_VAL(new_matrix(rows, cols, data));
    return true;
}

static bool native_matmul(int arg_count, Value* args, Value* result) {
    if (arg_count != 2) {
        fprintf(stderr, "JTS GO: matmul() expects 2 arguments\n");
        return false;
    }
    if (!IS_MATRIX(args[0]) || !IS_MATRIX(args[1])) {
        fprintf(stderr, "JTS GO: matmul() arguments must be matrices\n");
        return false;
    }
    ObjMatrix* a = AS_MATRIX(args[0]);
    ObjMatrix* b = AS_MATRIX(args[1]);
    if (a->cols != b->rows) {
        fprintf(stderr, "JTS GO: matmul() incompatible dimensions (%dx%d) * (%dx%d)\n",
                a->rows, a->cols, b->rows, b->cols);
        return false;
    }
    double** data = (double**)reallocate(NULL, 0, sizeof(double*) * a->rows);
    for (int i = 0; i < a->rows; i++) {
        data[i] = (double*)reallocate(NULL, 0, sizeof(double) * b->cols);
        for (int j = 0; j < b->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < a->cols; k++) {
                sum += a->data[i][k] * b->data[k][j];
            }
            data[i][j] = sum;
        }
    }
    *result = OBJ_VAL(new_matrix(a->rows, b->cols, data));
    return true;
}

static bool native_sigmoid(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: sigmoid() expects 1 argument\n");
        return false;
    }
    if (!IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: sigmoid() argument must be a number\n");
        return false;
    }
    double x = AS_NUMBER(args[0]);
    *result = NUMBER_VAL(1.0 / (1.0 + exp(-x)));
    return true;
}

static bool native_relu(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: relu() expects 1 argument\n");
        return false;
    }
    if (!IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: relu() argument must be a number\n");
        return false;
    }
    double x = AS_NUMBER(args[0]);
    *result = NUMBER_VAL(x > 0 ? x : 0);
    return true;
}

static bool native_mse(int arg_count, Value* args, Value* result) {
    if (arg_count != 2) {
        fprintf(stderr, "JTS GO: mse() expects 2 arguments\n");
        return false;
    }
    if (!IS_LIST(args[0]) || !IS_LIST(args[1])) {
        fprintf(stderr, "JTS GO: mse() arguments must be lists\n");
        return false;
    }
    ObjList* predicted = AS_LIST(args[0]);
    ObjList* actual = AS_LIST(args[1]);
    if (predicted->count != actual->count) {
        fprintf(stderr, "JTS GO: mse() lists must have same length\n");
        return false;
    }
    double sum = 0.0;
    for (int i = 0; i < predicted->count; i++) {
        if (IS_NUMBER(predicted->values[i]) && IS_NUMBER(actual->values[i])) {
            double diff = AS_NUMBER(predicted->values[i]) - AS_NUMBER(actual->values[i]);
            sum += diff * diff;
        }
    }
    *result = NUMBER_VAL(sum / predicted->count);
    return true;
}

static bool native_http_server(int arg_count, Value* args, Value* result) {
    if (arg_count < 1) {
        fprintf(stderr, "JTS GO: http_server() expects at least 1 argument (port)\n");
        return false;
    }
    if (!IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: http_server() first argument must be a port number\n");
        return false;
    }
    int port = (int)AS_NUMBER(args[0]);
    ObjHttpServer* server = new_http_server(port);
    if (arg_count > 1) {
        server->handler = args[1];
    } else {
        server->handler = OBJ_VAL(copy_string("<html><body><h1>JTS GO Server</h1></body></html>", 46));
    }
    *result = OBJ_VAL(server);
    return true;
}

static bool native_http_start(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: http_start() expects 1 argument\n");
        return false;
    }
    if (!IS_HTTP_SERVER(args[0])) {
        fprintf(stderr, "JTS GO: http_start() argument must be an http_server\n");
        return false;
    }

    ObjHttpServer* server = AS_HTTP_SERVER(args[0]);
    int port = server->port;

    const char* body = "Hello from JTS GO!";
    int body_len = (int)strlen(body);
    if (IS_STRING(server->handler)) {
        body = AS_CSTRING(server->handler);
        body_len = AS_STRING(server->handler)->length;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "JTS GO: WSAStartup failed\n");
        return false;
    }
#endif

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        fprintf(stderr, "JTS GO: Failed to create socket\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((u_short)port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        fprintf(stderr, "JTS GO: Failed to bind to port %d\n", port);
        closesocket(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    if (listen(server_fd, 10) == SOCKET_ERROR) {
        fprintf(stderr, "JTS GO: Failed to listen\n");
        closesocket(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    printf("JTS GO HTTP Server running at http://localhost:%d\n", port);
    printf("Press Ctrl+C to stop\n\n");

    server->running = true;

    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == INVALID_SOCKET) continue;

        char request[4096] = {0};
        int bytes_read = recv(client_fd, request, sizeof(request) - 1, 0);
        if (bytes_read <= 0) {
            closesocket(client_fd);
            continue;
        }

        char method[16] = {0};
        char path[256] = {0};
        sscanf(request, "%15s %255s", method, path);
        printf("[%s] %s %s\n", "200", method, path);

        char header[512];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n", body_len);

        send(client_fd, header, header_len, 0);
        send(client_fd, body, body_len, 0);
        closesocket(client_fd);
    }

    closesocket(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif

    *result = NIL_VAL;
    return true;
}

static bool native_http_request(int arg_count, Value* args, Value* result) {
    if (arg_count < 1) {
        fprintf(stderr, "JTS GO: http_request() expects at least 1 argument\n");
        return false;
    }
    if (!IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: http_request() argument must be a URL string\n");
        return false;
    }

    const char* url = AS_CSTRING(args[0]);

    char host[256] = {0};
    char path[256] = "/";
    int port = 80;

    const char* p = url;
    if (strncmp(p, "http://", 7) == 0) p += 7;
    else if (strncmp(p, "https://", 8) == 0) { p += 8; port = 443; }

    const char* slash = strchr(p, '/');
    const char* colon = strchr(p, ':');
    if (colon && (!slash || colon < slash)) {
        int hlen = (int)(colon - p);
        if (hlen > 255) hlen = 255;
        memcpy(host, p, hlen);
        host[hlen] = '\0';
        port = atoi(colon + 1);
        if (slash) strcpy(path, slash);
    } else {
        int hlen = slash ? (int)(slash - p) : (int)strlen(p);
        if (hlen > 255) hlen = 255;
        memcpy(host, p, hlen);
        host[hlen] = '\0';
        if (slash) strcpy(path, slash);
    }

#ifdef _WIN32
    WSADATA wsa;
    WSACleanup();
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "JTS GO: Failed to create socket\n");
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((u_short)port);

    struct hostent* he = gethostbyname(host);
    if (!he) {
        fprintf(stderr, "JTS GO: Cannot resolve host '%s'\n", host);
        closesocket(sock);
        return false;
    }
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "JTS GO: Cannot connect to %s:%d\n", host, port);
        closesocket(sock);
        return false;
    }

    char request[1024];
    int req_len = snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    send(sock, request, req_len, 0);

    char response[65536] = {0};
    int total = 0;
    int n;
    while ((n = recv(sock, response + total, sizeof(response) - total - 1, 0)) > 0) {
        total += n;
    }
    closesocket(sock);

    ObjList* result_list = new_list();
    list_append(result_list, NUMBER_VAL(200));

    char* body_start = strstr(response, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        int body_len = total - (int)(body_start - response);
        list_append(result_list, OBJ_VAL(copy_string(body_start, body_len)));
    } else {
        list_append(result_list, OBJ_VAL(copy_string("", 0)));
    }

    *result = OBJ_VAL(result_list);
    return true;
}

static bool native_sqrt(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: sqrt() expects 1 number argument\n");
        return false;
    }
    *result = NUMBER_VAL(sqrt(AS_NUMBER(args[0])));
    return true;
}

static bool native_upper(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: upper() expects 1 string argument\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    char* buf = ALLOCATE(char, str->length + 1);
    for (int i = 0; i < str->length; i++) {
        buf[i] = (str->chars[i] >= 'a' && str->chars[i] <= 'z') ?
                  str->chars[i] - 32 : str->chars[i];
    }
    buf[str->length] = '\0';
    *result = OBJ_VAL(take_string(buf, str->length));
    return true;
}

static bool native_lower(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: lower() expects 1 string argument\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    char* buf = ALLOCATE(char, str->length + 1);
    for (int i = 0; i < str->length; i++) {
        buf[i] = (str->chars[i] >= 'A' && str->chars[i] <= 'Z') ?
                  str->chars[i] + 32 : str->chars[i];
    }
    buf[str->length] = '\0';
    *result = OBJ_VAL(take_string(buf, str->length));
    return true;
}

static bool native_trim(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: trim() expects 1 string argument\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    int start = 0, end = str->length - 1;
    while (start < str->length && (str->chars[start] == ' ' || str->chars[start] == '\t' || str->chars[start] == '\n')) start++;
    while (end >= start && (str->chars[end] == ' ' || str->chars[end] == '\t' || str->chars[end] == '\n')) end--;
    if (start > end) {
        *result = OBJ_VAL(copy_string("", 0));
    } else {
        *result = OBJ_VAL(copy_string(&str->chars[start], end - start + 1));
    }
    return true;
}

static bool native_split(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        fprintf(stderr, "JTS GO: split() expects (string, delimiter)\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* delim = AS_STRING(args[1]);
    ObjList* list = new_list();

    if (delim->length == 0) {
        for (int i = 0; i < str->length; i++) {
            list_append(list, OBJ_VAL(copy_string(&str->chars[i], 1)));
        }
    } else {
        int start = 0;
        for (int i = 0; i <= str->length - delim->length; i++) {
            if (memcmp(&str->chars[i], delim->chars, delim->length) == 0) {
                list_append(list, OBJ_VAL(copy_string(&str->chars[start], i - start)));
                start = i + delim->length;
                i = start - 1;
            }
        }
        list_append(list, OBJ_VAL(copy_string(&str->chars[start], str->length - start)));
    }

    *result = OBJ_VAL(list);
    return true;
}

static bool native_contains(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        fprintf(stderr, "JTS GO: contains() expects (string, substring)\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* sub = AS_STRING(args[1]);
    bool found = false;
    if (sub->length <= str->length) {
        for (int i = 0; i <= str->length - sub->length; i++) {
            if (memcmp(&str->chars[i], sub->chars, sub->length) == 0) {
                found = true;
                break;
            }
        }
    }
    *result = BOOL_VAL(found);
    return true;
}

static bool native_replace(int arg_count, Value* args, Value* result) {
    if (arg_count != 3 || !IS_STRING(args[0]) || !IS_STRING(args[1]) || !IS_STRING(args[2])) {
        fprintf(stderr, "JTS GO: replace() expects (string, old, new)\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* old = AS_STRING(args[1]);
    ObjString* new_str = AS_STRING(args[2]);
    char* buf = ALLOCATE(char, str->length * 2 + 1);
    int pos = 0;
    for (int i = 0; i < str->length;) {
        if (i <= str->length - old->length && memcmp(&str->chars[i], old->chars, old->length) == 0) {
            memcpy(buf + pos, new_str->chars, new_str->length);
            pos += new_str->length;
            i += old->length;
        } else {
            buf[pos++] = str->chars[i++];
        }
    }
    buf[pos] = '\0';
    *result = OBJ_VAL(take_string(buf, pos));
    return true;
}

static bool native_substring(int arg_count, Value* args, Value* result) {
    if (arg_count < 2 || arg_count > 3 || !IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: substring() expects (string, start[, end])\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    int start = (int)AS_NUMBER(args[1]);
    int end = arg_count == 3 ? (int)AS_NUMBER(args[2]) : str->length;
    if (start < 0) start = 0;
    if (end > str->length) end = str->length;
    if (start >= end) {
        *result = OBJ_VAL(copy_string("", 0));
    } else {
        *result = OBJ_VAL(copy_string(&str->chars[start], end - start));
    }
    return true;
}

static bool native_starts_with(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        fprintf(stderr, "JTS GO: starts_with() expects (string, prefix)\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* prefix = AS_STRING(args[1]);
    if (prefix->length > str->length) {
        *result = BOOL_VAL(false);
    } else {
        *result = BOOL_VAL(memcmp(str->chars, prefix->chars, prefix->length) == 0);
    }
    return true;
}

static bool native_ends_with(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        fprintf(stderr, "JTS GO: ends_with() expects (string, suffix)\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* suffix = AS_STRING(args[1]);
    if (suffix->length > str->length) {
        *result = BOOL_VAL(false);
    } else {
        *result = BOOL_VAL(memcmp(&str->chars[str->length - suffix->length],
                                   suffix->chars, suffix->length) == 0);
    }
    return true;
}

static bool native_length(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: length() expects 1 argument\n");
        return false;
    }
    if (IS_STRING(args[0])) {
        *result = NUMBER_VAL((double)AS_STRING(args[0])->length);
        return true;
    }
    if (IS_LIST(args[0])) {
        *result = NUMBER_VAL((double)AS_LIST(args[0])->count);
        return true;
    }
    fprintf(stderr, "JTS GO: length() argument must be a string or list\n");
    return false;
}

static bool native_remove(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: remove() expects (list, index)\n");
        return false;
    }
    ObjList* list = AS_LIST(args[0]);
    int index = (int)AS_NUMBER(args[1]);
    if (index < 0 || index >= list->count) {
        fprintf(stderr, "JTS GO: remove() index out of bounds\n");
        return false;
    }
    *result = list->values[index];
    for (int i = index; i < list->count - 1; i++) {
        list->values[i] = list->values[i + 1];
    }
    list->count--;
    return true;
}

static bool native_pop(int arg_count, Value* args, Value* result) {
    if (arg_count < 1 || !IS_LIST(args[0])) {
        fprintf(stderr, "JTS GO: pop() expects (list[, index])\n");
        return false;
    }
    ObjList* list = AS_LIST(args[0]);
    if (list->count == 0) {
        fprintf(stderr, "JTS GO: pop() from empty list\n");
        return false;
    }
    int index = list->count - 1;
    if (arg_count == 2 && IS_NUMBER(args[1])) {
        index = (int)AS_NUMBER(args[1]);
        if (index < 0 || index >= list->count) {
            fprintf(stderr, "JTS GO: pop() index out of bounds\n");
            return false;
        }
    }
    *result = list->values[index];
    for (int i = index; i < list->count - 1; i++) {
        list->values[i] = list->values[i + 1];
    }
    list->count--;
    return true;
}

static bool native_sort(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_LIST(args[0])) {
        fprintf(stderr, "JTS GO: sort() expects 1 list argument\n");
        return false;
    }
    ObjList* list = AS_LIST(args[0]);
    // Simple bubble sort
    for (int i = 0; i < list->count - 1; i++) {
        for (int j = 0; j < list->count - i - 1; j++) {
            if (IS_NUMBER(list->values[j]) && IS_NUMBER(list->values[j + 1])) {
                if (AS_NUMBER(list->values[j]) > AS_NUMBER(list->values[j + 1])) {
                    Value temp = list->values[j];
                    list->values[j] = list->values[j + 1];
                    list->values[j + 1] = temp;
                }
            }
        }
    }
    *result = args[0];
    return true;
}

static bool native_read_file(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: read_file() expects 1 string argument\n");
        return false;
    }
    const char* path = AS_CSTRING(args[0]);
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "JTS GO: Cannot open file '%s'\n", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = ALLOCATE(char, size + 1);
    size_t bytes_read = fread(buf, 1, size, f);
    buf[bytes_read] = '\0';
    fclose(f);
    *result = OBJ_VAL(take_string(buf, (int)bytes_read));
    return true;
}

static bool native_write_file(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        fprintf(stderr, "JTS GO: write_file() expects (filename, content)\n");
        return false;
    }
    const char* path = AS_CSTRING(args[0]);
    ObjString* content = AS_STRING(args[1]);
    FILE* f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "JTS GO: Cannot write to file '%s'\n", path);
        return false;
    }
    fwrite(content->chars, 1, content->length, f);
    fclose(f);
    *result = BOOL_VAL(true);
    return true;
}

static bool native_import_file(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: import() expects 1 string argument\n");
        return false;
    }
    const char* path = AS_CSTRING(args[0]);
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "JTS GO: Cannot open file '%s'\n", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = ALLOCATE(char, size + 1);
    size_t bytes_read = fread(buf, 1, size, f);
    buf[bytes_read] = '\0';
    fclose(f);
    InterpretResult res = interpret(buf);
    free(buf);
    *result = BOOL_VAL(res == INTERPRET_OK);
    return (res == INTERPRET_OK);
}

typedef struct {
    const char* name;
    int arity;
    bool (*function)(int arg_count, Value* args, Value* result);
} NativeDef;

static NativeDef native_functions[] = {
    {"print", -1, native_print},
    {"input", -1, native_input},
    {"len",    1, native_len},
    {"type",   1, native_type},
    {"append", 2, native_append},
    {"number", 1, native_number},
    {"str",    1, native_string},
    {"math",   2, native_math},
    {"tensor", 1, native_tensor},
    {"matrix", -1, native_matrix},
    {"matmul", 2, native_matmul},
    {"sigmoid", 1, native_sigmoid},
    {"relu",  1, native_relu},
    {"mse",   2, native_mse},
    {"http_server", -1, native_http_server},
    {"http_start",  1, native_http_start},
    {"http_request", -1, native_http_request},
    {"sqrt",  1, native_sqrt},
    // String methods
    {"upper", 1, native_upper},
    {"lower", 1, native_lower},
    {"trim", 1, native_trim},
    {"split", 2, native_split},
    {"contains", 2, native_contains},
    {"replace", 3, native_replace},
    {"substring", -1, native_substring},
    {"starts_with", 2, native_starts_with},
    {"ends_with", 2, native_ends_with},
    {"length", 1, native_length},
    // List ops
    {"remove", 2, native_remove},
    {"pop", -1, native_pop},
    {"sort", 1, native_sort},
    // File I/O
    {"read_file", 1, native_read_file},
    {"write_file", 2, native_write_file},
    // Import
    {"import_file", 1, native_import_file},
    {NULL,     0, NULL}
};

void register_native_functions(void) {
    // Native functions are registered by name in the VM
    // They're called via string matching in OP_CALL
}

bool call_native(const char* name, int arg_count, Value* args, Value* result) {
    for (int i = 0; native_functions[i].name != NULL; i++) {
        if (strcmp(name, native_functions[i].name) == 0) {
            if (native_functions[i].arity >= 0 &&
                arg_count != native_functions[i].arity) {
                fprintf(stderr, "JTS GO: %s() expects %d arguments but got %d.\n",
                        name, native_functions[i].arity, arg_count);
                return false;
            }
            return native_functions[i].function(arg_count, args, result);
        }
    }
    return false;
}
