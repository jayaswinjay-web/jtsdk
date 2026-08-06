#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <direct.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#define GET_CWD _getcwd
#define STRCASECMP _stricmp
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define GET_CWD getcwd
#define STRCASECMP strcasecmp
#endif

static int jts_argc = 0;
static const char** jts_argv = NULL;

void set_program_args(int argc, const char** argv) {
    jts_argc = argc;
    jts_argv = argv;
}

#include "vm/native.h"
#include "vm/vm.h"
#include "core/object.h"
#include "core/memory.h"

static bool native_dict_pop(int arg_count, Value* args, Value* result);

static ObjString* value_to_display_string(Value value) {
    if (IS_STRING(value)) return AS_STRING(value);
    if (IS_NUMBER(value)) return AS_STRING(number_to_string(AS_NUMBER(value)));
    if (IS_BOOL(value)) {
        return copy_string(AS_BOOL(value) ? "true" : "false",
                           AS_BOOL(value) ? 4 : 5);
    }
    if (IS_VOID(value)) return copy_string("void", 3);
    return copy_string("<?>", 4);
}

static bool native_print(int arg_count, Value* args, Value* result) {
    for (int i = 0; i < arg_count; i++) {
        if (i > 0) printf(" ");
        print_value(args[i]);
    }
    printf("\n");
    *result = VOID_VAL;
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
        } else if (len == 3 && memcmp(buffer, "void", 3) == 0) {
            *result = VOID_VAL;
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
    if (IS_DICT(args[0])) {
        *result = NUMBER_VAL((double)AS_DICT(args[0])->entries.live);
        return true;
    }
    if (IS_SET(args[0])) {
        *result = NUMBER_VAL((double)AS_SET(args[0])->count);
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
    fprintf(stderr, "JTS GO: len() argument must be a string, list, dict, set, tensor, or matrix\n");
    return false;
}

static bool native_type(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: type() expects 1 argument\n");
        return false;
    }
    const char* type_name = "unknown";
    switch (args[0].type) {
        case VAL_VOID:     type_name = "void"; break;
        case VAL_BOOL:    type_name = "boolean"; break;
        case VAL_NUMBER:  type_name = "number"; break;
        case VAL_OBJ:
            if (IS_STRING(args[0])) {
                type_name = "string";
            } else if (IS_FUNCTION(args[0]) || IS_CLOSURE(args[0])) {
                type_name = "function";
            } else if (IS_LIST(args[0])) {
                type_name = "list";
            } else if (IS_DICT(args[0])) {
                type_name = "dict";
            } else if (IS_SET(args[0])) {
                type_name = "set";
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
    if (IS_VOID(args[0])) {
        *result = OBJ_VAL(copy_string("void", 3));
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
            } else if (IS_VOID(list->values[i])) {
                memcpy(buf + pos, "void", 3); pos += 3;
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
    int* shape = (int*)reallocate(NULL, 0, sizeof(int) * 4);
    shape[0] = list->count;
    shape[1] = shape[2] = shape[3] = 0;
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

static bool native_http_route(int arg_count, Value* args, Value* result) {
    if (arg_count != 4) {
        fprintf(stderr, "JTS GO: http_route() expects 4 arguments (server, method, path, body)\n");
        return false;
    }
    if (!IS_HTTP_SERVER(args[0])) {
        fprintf(stderr, "JTS GO: http_route() first argument must be an http_server\n");
        return false;
    }
    if (!IS_STRING(args[1])) {
        fprintf(stderr, "JTS GO: http_route() method must be a string (GET, POST, etc.)\n");
        return false;
    }
    if (!IS_STRING(args[2])) {
        fprintf(stderr, "JTS GO: http_route() path must be a string (e.g. \"/\")\n");
        return false;
    }

    ObjHttpServer* server = AS_HTTP_SERVER(args[0]);
    if (server->route_count >= MAX_ROUTES) {
        fprintf(stderr, "JTS GO: http_route() too many routes (max %d)\n", MAX_ROUTES);
        return false;
    }

    HttpRoute* route = &server->routes[server->route_count];
    const char* method = AS_CSTRING(args[1]);
    const char* path = AS_CSTRING(args[2]);
    strncpy(route->method, method, MAX_METHOD_LEN - 1);
    route->method[MAX_METHOD_LEN - 1] = '\0';
    strncpy(route->path, path, MAX_PATH_LEN - 1);
    route->path[MAX_PATH_LEN - 1] = '\0';
    route->body = args[3];
    server->route_count++;

    *result = VOID_VAL;
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

        const char* resp_body = body;
        int resp_body_len = body_len;
        int status_code = 200;
        const char* content_type = "text/html; charset=utf-8";

        for (int i = 0; i < server->route_count; i++) {
            HttpRoute* route = &server->routes[i];
            if (STRCASECMP(route->method, method) == 0 && strcmp(route->path, path) == 0) {
                if (IS_STRING(route->body)) {
                    resp_body = AS_CSTRING(route->body);
                    resp_body_len = AS_STRING(route->body)->length;
                } else {
                    resp_body = "Route handler returned non-string";
                    resp_body_len = (int)strlen(resp_body);
                }
                if (resp_body_len > 0 && resp_body[0] == '{') {
                    content_type = "application/json; charset=utf-8";
                }
                break;
            }
        }

        if (server->route_count == 0 && resp_body == body) {
            status_code = 200;
        } else if (resp_body == body && server->route_count > 0) {
            resp_body = "404 Not Found";
            resp_body_len = (int)strlen(resp_body);
            status_code = 404;
            content_type = "text/plain; charset=utf-8";
        }

        printf("[%d] %s %s\n", status_code, method, path);

        char header[512];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n", status_code,
            status_code == 200 ? "OK" : "Not Found",
            content_type, resp_body_len);

        send(client_fd, header, header_len, 0);
        send(client_fd, resp_body, resp_body_len, 0);
        closesocket(client_fd);
    }

    closesocket(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif

    *result = VOID_VAL;
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

static bool native_sin(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: sin() expects 1 number argument\n");
        return false;
    }
    *result = NUMBER_VAL(sin(AS_NUMBER(args[0])));
    return true;
}

static bool native_cos(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: cos() expects 1 number argument\n");
        return false;
    }
    *result = NUMBER_VAL(cos(AS_NUMBER(args[0])));
    return true;
}

static bool native_tan(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: tan() expects 1 number argument\n");
        return false;
    }
    *result = NUMBER_VAL(tan(AS_NUMBER(args[0])));
    return true;
}

static bool native_log(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: log() expects 1 number argument\n");
        return false;
    }
    *result = NUMBER_VAL(log(AS_NUMBER(args[0])));
    return true;
}

static bool native_exp(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: exp() expects 1 number argument\n");
        return false;
    }
    *result = NUMBER_VAL(exp(AS_NUMBER(args[0])));
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
    if (arg_count != 2 || !IS_LIST(args[0])) {
        fprintf(stderr, "JTS GO: remove() expects (list, value)\n");
        return false;
    }
    ObjList* list = AS_LIST(args[0]);
    for (int i = 0; i < list->count; i++) {
        if (values_equal(list->values[i], args[1])) {
            *result = list->values[i];
            for (int j = i; j < list->count - 1; j++) {
                list->values[j] = list->values[j + 1];
            }
            list->count--;
            return true;
        }
    }
    fprintf(stderr, "JTS GO: remove() value not found in list\n");
    return false;
}

static bool native_pop(int arg_count, Value* args, Value* result) {
    if (arg_count < 1) {
        fprintf(stderr, "JTS GO: pop() expects (list_or_dict[, key])\n");
        return false;
    }
    if (IS_DICT(args[0])) {
        return native_dict_pop(arg_count, args, result);
    }
    if (!IS_LIST(args[0])) {
        fprintf(stderr, "JTS GO: pop() first argument must be a list or dict\n");
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
        if (index < 0) index = list->count + index;
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

static bool native_file_exists(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: file_exists() expects 1 string argument\n");
        return false;
    }
    FILE* f = fopen(AS_CSTRING(args[0]), "rb");
    if (f != NULL) {
        fclose(f);
        *result = BOOL_VAL(true);
    } else {
        *result = BOOL_VAL(false);
    }
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
    InterpretResult res = interpret_isolated(buf);
    free(buf);
    *result = BOOL_VAL(res == INTERPRET_OK);
    return (res == INTERPRET_OK);
}

static bool native_next(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_GENERATOR(args[0])) {
        fprintf(stderr, "JTS GO: next() expects 1 generator argument\n");
        return false;
    }
    ObjGenerator* gen = AS_GENERATOR(args[0]);
    InterpretResult res = vm_resume_generator(gen);
    if (res == INTERPRET_YIELD) {
        *result = pop();
        return true;
    } else if (res == INTERPRET_OK) {
        gen->exhausted = true;
        *result = pop();
        return true;
    } else {
        return false;
    }
}

static void path_join(char* out, size_t outsize, const char* dir, const char* rel) {
    if (dir == NULL || dir[0] == '\0') {
        snprintf(out, outsize, "%s", rel);
        return;
    }
    int dlen = (int)strlen(dir);
    if (dlen > 0 && (dir[dlen - 1] == '/' || dir[dlen - 1] == '\\')) {
        snprintf(out, outsize, "%s%s", dir, rel);
    } else {
        snprintf(out, outsize, "%s/%s", dir, rel);
    }
}

static bool file_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) return false;
    fclose(f);
    return true;
}

static bool resolve_scroll_path(const char* name, char* out, size_t outsize) {
    // name: "math" or "math.stats" -> try <root>/<rel>.jts and <root>/scrolls/<rel>.jts
    char rel[512];
    int rlen = 0;
    for (const char* p = name; *p; p++) {
        if (rlen < (int)sizeof(rel) - 1) rel[rlen++] = (*p == '.') ? '/' : *p;
    }
    rel[rlen] = '\0';

    const char* roots[3];
    int n_roots = 0;
    if (vm.program_dir != NULL) roots[n_roots++] = vm.program_dir;
    if (vm.install_dir != NULL) roots[n_roots++] = vm.install_dir;
    roots[n_roots++] = ""; // current working directory

    char candidate[1024];
    for (int i = 0; i < n_roots; i++) {
        path_join(candidate, sizeof(candidate), roots[i], rel);
        strncat(candidate, ".jts", sizeof(candidate) - strlen(candidate) - 1);
        if (file_exists(candidate)) {
            snprintf(out, outsize, "%s", candidate);
            return true;
        }
        char sub[1024];
        path_join(sub, sizeof(sub), roots[i], "scrolls");
        path_join(candidate, sizeof(candidate), sub, rel);
        strncat(candidate, ".jts", sizeof(candidate) - strlen(candidate) - 1);
        if (file_exists(candidate)) {
            snprintf(out, outsize, "%s", candidate);
            return true;
        }
    }
    return false;
}

static bool dict_get_str(ObjDict* dict, const char* key, Value* result) {
    return dict_get(dict, copy_string(key, (int)strlen(key)), result);
}

static void collect_new_globals(Table* before, ObjDict* exports) {
    for (int i = 0; i < vm.globals.capacity; i++) {
        Entry* e = &vm.globals.entries[i];
        if (e->key != NULL) {
            Value old;
            if (!table_get(before, e->key, &old)) {
                dict_set(exports, e->key, e->value);
            } else if (!values_equal(old, e->value)) {
                dict_set(exports, e->key, e->value);
            }
        }
    }
}

static void merge_exports(ObjDict* target, ObjDict* exports) {
    for (int i = 0; i < exports->entries.capacity; i++) {
        Entry* e = &exports->entries.entries[i];
        if (e->key != NULL) {
            dict_set(target, e->key, e->value);
        }
    }
}

static const char* NS_MARKER = "\x1fns";

static void mark_namespace(ObjDict* dict) {
    dict_set(dict, copy_string(NS_MARKER, 3), BOOL_VAL(true));
}

static bool is_namespace(ObjDict* dict) {
    Value v;
    return dict_get(dict, copy_string(NS_MARKER, 3), &v);
}

static bool attach_scroll_namespace(const char* name, ObjDict* exports) {
    // Split dotted name into segments
    char buf[512];
    snprintf(buf, sizeof(buf), "%s", name);
    char* segs[32];
    int n = 0;
    char* p = buf;
    while (p != NULL && *p != '\0' && n < 32) {
        segs[n++] = p;
        char* dot = strchr(p, '.');
        if (dot == NULL) break;
        *dot = '\0';
        p = dot + 1;
    }
    if (n == 0) return false;

    ObjDict* current = NULL;
    for (int i = 0; i < n - 1; i++) {
        ObjString* key = copy_string(segs[i], (int)strlen(segs[i]));
        Value holder;
        if (i == 0) {
            if (table_get(&vm.globals, key, &holder) && IS_DICT(holder)) {
                current = AS_DICT(holder);
            } else {
                current = new_dict();
                mark_namespace(current);
                push(OBJ_VAL(current));
                table_set(&vm.globals, key, OBJ_VAL(current));
                pop();
            }
        } else {
            if (current != NULL && dict_get(current, key, &holder) && IS_DICT(holder)) {
                current = AS_DICT(holder);
            } else {
                ObjDict* newd = new_dict();
                mark_namespace(newd);
                push(OBJ_VAL(newd));
                if (current != NULL) dict_set(current, key, OBJ_VAL(newd));
                pop();
                current = newd;
            }
        }
    }

    ObjString* last = copy_string(segs[n - 1], (int)strlen(segs[n - 1]));
    if (current == NULL) {
        Value holder;
        if (table_get(&vm.globals, last, &holder) && IS_DICT(holder)) {
            merge_exports(AS_DICT(holder), exports);
        } else {
            push(OBJ_VAL(exports));
            table_set(&vm.globals, last, OBJ_VAL(exports));
            pop();
        }
    } else {
        dict_set(current, last, OBJ_VAL(exports));
    }
    return true;
}

static bool native_bring_scroll(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: bring() expects 1 string argument\n");
        return false;
    }
    const char* name = AS_CSTRING(args[0]);
    if (name[0] == '\0') {
        fprintf(stderr, "JTS GO: bring() scroll name cannot be empty\n");
        return false;
    }

    ObjString* key = copy_string(name, (int)strlen(name));
    Value dummy;
    if (table_get(&vm.scrolls_loaded, key, &dummy)) {
        *result = BOOL_VAL(true);
        return true; // already loaded
    }

    char path[1024];
    if (!resolve_scroll_path(name, path, sizeof(path))) {
        fprintf(stderr, "JTS GO: Cannot find scroll '%s'\n", name);
        return false;
    }

    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "JTS GO: Cannot open scroll '%s' at '%s'\n", name, path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = ALLOCATE(char, size + 1);
    size_t bytes_read = fread(buf, 1, size, f);
    buf[bytes_read] = '\0';
    fclose(f);

    Table before;
    init_table(&before);
    // Snapshot: copy keys set so far (cheap, capacity is small)
    if (vm.globals.capacity > 0) {
        before.capacity = vm.globals.capacity;
        before.count = vm.globals.count;
        before.entries = ALLOCATE(Entry, vm.globals.capacity);
        for (int i = 0; i < vm.globals.capacity; i++) {
            before.entries[i] = vm.globals.entries[i];
        }
    }

    InterpretResult res = interpret_isolated(buf);
    free(buf);

    ObjDict* exports = new_dict();
    mark_namespace(exports);
    push(OBJ_VAL(exports));
    collect_new_globals(&before, exports);
    free_table(&before);

    if (res == INTERPRET_OK) {
        if (!attach_scroll_namespace(name, exports)) {
            pop();
            fprintf(stderr, "JTS GO: Failed to attach scroll '%s'\n", name);
            return false;
        }
        table_set(&vm.scrolls_loaded, key, BOOL_VAL(true));
        pop();
        *result = BOOL_VAL(true);
        return true;
    } else {
        pop();
        *result = BOOL_VAL(false);
        return false;
    }
}

static bool native_range(int arg_count, Value* args, Value* result) {
    int start = 0, end = 0, step = 1;
    if (arg_count == 1) {
        if (!IS_NUMBER(args[0])) { fprintf(stderr, "JTS GO: range() expects number arguments\n"); return false; }
        end = (int)AS_NUMBER(args[0]);
    } else if (arg_count == 2) {
        if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) { fprintf(stderr, "JTS GO: range() expects number arguments\n"); return false; }
        start = (int)AS_NUMBER(args[0]);
        end = (int)AS_NUMBER(args[1]);
    } else if (arg_count == 3) {
        if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1]) || !IS_NUMBER(args[2])) { fprintf(stderr, "JTS GO: range() expects number arguments\n"); return false; }
        start = (int)AS_NUMBER(args[0]);
        end = (int)AS_NUMBER(args[1]);
        step = (int)AS_NUMBER(args[2]);
        if (step == 0) { fprintf(stderr, "JTS GO: range() step cannot be zero\n"); return false; }
    } else {
        fprintf(stderr, "JTS GO: range() expects 1 to 3 arguments\n");
        return false;
    }
    ObjList* list = new_list();
    if (step > 0) {
        for (int i = start; i < end; i += step) list_append(list, NUMBER_VAL((double)i));
    } else {
        for (int i = start; i > end; i += step) list_append(list, NUMBER_VAL((double)i));
    }
    *result = OBJ_VAL(list);
    return true;
}

static bool native_abs(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: abs() expects 1 number argument\n");
        return false;
    }
    *result = NUMBER_VAL(fabs(AS_NUMBER(args[0])));
    return true;
}

static bool native_min(int arg_count, Value* args, Value* result) {
    ObjList* list = NULL;
    if (arg_count == 1 && IS_LIST(args[0])) {
        list = AS_LIST(args[0]);
        if (list->count == 0) { fprintf(stderr, "JTS GO: min() of empty list\n"); return false; }
        args = list->values;
        arg_count = list->count;
    }
    if (arg_count < 1) { fprintf(stderr, "JTS GO: min() expects at least 1 argument\n"); return false; }
    Value best = args[0];
    for (int i = 1; i < arg_count; i++) {
        bool less = false;
        if (IS_NUMBER(best) && IS_NUMBER(args[i])) {
            less = AS_NUMBER(args[i]) < AS_NUMBER(best);
        } else if (IS_STRING(best) && IS_STRING(args[i])) {
            less = AS_STRING(args[i])->length < AS_STRING(best)->length ||
                   (AS_STRING(args[i])->length == AS_STRING(best)->length &&
                    memcmp(AS_CSTRING(args[i]), AS_CSTRING(best), AS_STRING(best)->length) < 0);
        }
        if (less) best = args[i];
    }
    *result = best;
    return true;
}

static bool native_max(int arg_count, Value* args, Value* result) {
    ObjList* list = NULL;
    if (arg_count == 1 && IS_LIST(args[0])) {
        list = AS_LIST(args[0]);
        if (list->count == 0) { fprintf(stderr, "JTS GO: max() of empty list\n"); return false; }
        args = list->values;
        arg_count = list->count;
    }
    if (arg_count < 1) { fprintf(stderr, "JTS GO: max() expects at least 1 argument\n"); return false; }
    Value best = args[0];
    for (int i = 1; i < arg_count; i++) {
        bool greater = false;
        if (IS_NUMBER(best) && IS_NUMBER(args[i])) {
            greater = AS_NUMBER(args[i]) > AS_NUMBER(best);
        } else if (IS_STRING(best) && IS_STRING(args[i])) {
            greater = AS_STRING(args[i])->length > AS_STRING(best)->length ||
                      (AS_STRING(args[i])->length == AS_STRING(best)->length &&
                       memcmp(AS_CSTRING(args[i]), AS_CSTRING(best), AS_STRING(best)->length) > 0);
        }
        if (greater) best = args[i];
    }
    *result = best;
    return true;
}

static bool native_sum(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_LIST(args[0])) {
        fprintf(stderr, "JTS GO: sum() expects 1 list argument\n");
        return false;
    }
    ObjList* list = AS_LIST(args[0]);
    double total = 0.0;
    for (int i = 0; i < list->count; i++) {
        if (IS_NUMBER(list->values[i])) total += AS_NUMBER(list->values[i]);
    }
    *result = NUMBER_VAL(total);
    return true;
}

static bool native_pow(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: pow() expects 2 number arguments\n");
        return false;
    }
    *result = NUMBER_VAL(pow(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
    return true;
}

static bool native_round(int arg_count, Value* args, Value* result) {
    if (arg_count < 1 || arg_count > 2 || !IS_NUMBER(args[0])) {
        fprintf(stderr, "JTS GO: round() expects (number[, ndigits])\n");
        return false;
    }
    double x = AS_NUMBER(args[0]);
    if (arg_count == 2) {
        if (!IS_NUMBER(args[1])) { fprintf(stderr, "JTS GO: round() ndigits must be a number\n"); return false; }
        int digits = (int)AS_NUMBER(args[1]);
        double factor = pow(10.0, digits);
        *result = NUMBER_VAL(round(x * factor) / factor);
    } else {
        *result = NUMBER_VAL(round(x));
    }
    return true;
}

static bool native_floor(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) { fprintf(stderr, "JTS GO: floor() expects 1 number argument\n"); return false; }
    *result = NUMBER_VAL(floor(AS_NUMBER(args[0])));
    return true;
}

static bool native_ceil(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) { fprintf(stderr, "JTS GO: ceil() expects 1 number argument\n"); return false; }
    *result = NUMBER_VAL(ceil(AS_NUMBER(args[0])));
    return true;
}

static bool native_rand(int arg_count, Value* args, Value* result) {
    if (arg_count != 0) { fprintf(stderr, "JTS GO: rand() expects 0 arguments\n"); return false; }
    *result = NUMBER_VAL((double)rand() / (double)RAND_MAX);
    return true;
}

static bool native_randint(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: randint() expects 2 number arguments\n");
        return false;
    }
    int low = (int)AS_NUMBER(args[0]);
    int high = (int)AS_NUMBER(args[1]);
    if (high < low) { fprintf(stderr, "JTS GO: randint() max must be >= min\n"); return false; }
    *result = NUMBER_VAL((double)(low + rand() % (high - low + 1)));
    return true;
}

static bool native_seed(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) { fprintf(stderr, "JTS GO: seed() expects 1 number argument\n"); return false; }
    srand((unsigned)AS_NUMBER(args[0]));
    *result = VOID_VAL;
    return true;
}

static bool native_shuffle(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_LIST(args[0])) { fprintf(stderr, "JTS GO: shuffle() expects 1 list argument\n"); return false; }
    ObjList* list = AS_LIST(args[0]);
    for (int i = list->count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Value temp = list->values[i];
        list->values[i] = list->values[j];
        list->values[j] = temp;
    }
    *result = args[0];
    return true;
}

static bool native_int(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) { fprintf(stderr, "JTS GO: int() expects 1 argument\n"); return false; }
    if (IS_NUMBER(args[0])) {
        *result = NUMBER_VAL((double)(long long)AS_NUMBER(args[0]));
        return true;
    }
    if (IS_STRING(args[0])) {
        char* end;
        double v = strtod(AS_CSTRING(args[0]), &end);
        if (*end != '\0') { fprintf(stderr, "JTS GO: int() invalid conversion from '%s'\n", AS_CSTRING(args[0])); return false; }
        *result = NUMBER_VAL((double)(long long)v);
        return true;
    }
    if (IS_BOOL(args[0])) {
        *result = NUMBER_VAL(AS_BOOL(args[0]) ? 1.0 : 0.0);
        return true;
    }
    fprintf(stderr, "JTS GO: int() argument must be a number, string, or boolean\n");
    return false;
}

static bool native_float(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) { fprintf(stderr, "JTS GO: float() expects 1 argument\n"); return false; }
    if (IS_NUMBER(args[0])) { *result = args[0]; return true; }
    if (IS_STRING(args[0])) {
        char* end;
        double v = strtod(AS_CSTRING(args[0]), &end);
        if (*end != '\0') { fprintf(stderr, "JTS GO: float() invalid conversion from '%s'\n", AS_CSTRING(args[0])); return false; }
        *result = NUMBER_VAL(v);
        return true;
    }
    if (IS_BOOL(args[0])) {
        *result = NUMBER_VAL(AS_BOOL(args[0]) ? 1.0 : 0.0);
        return true;
    }
    fprintf(stderr, "JTS GO: float() argument must be a number, string, or boolean\n");
    return false;
}

static bool native_bool(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) { fprintf(stderr, "JTS GO: bool() expects 1 argument\n"); return false; }
    if (IS_VOID(args[0])) { *result = BOOL_VAL(false); return true; }
    if (IS_BOOL(args[0])) { *result = args[0]; return true; }
    if (IS_NUMBER(args[0])) { *result = BOOL_VAL(AS_NUMBER(args[0]) != 0.0); return true; }
    if (IS_STRING(args[0])) { *result = BOOL_VAL(AS_STRING(args[0])->length > 0); return true; }
    *result = BOOL_VAL(true);
    return true;
}

static bool native_find(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_STRING(args[1])) {
        fprintf(stderr, "JTS GO: find() expects (string, substring)\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    ObjString* sub = AS_STRING(args[1]);
    if (sub->length == 0) { *result = NUMBER_VAL(0); return true; }
    for (int i = 0; i <= str->length - sub->length; i++) {
        if (memcmp(&str->chars[i], sub->chars, sub->length) == 0) {
            *result = NUMBER_VAL((double)i);
            return true;
        }
    }
    *result = NUMBER_VAL(-1);
    return true;
}

static bool native_count(int arg_count, Value* args, Value* result) {
    if (arg_count != 2) {
        fprintf(stderr, "JTS GO: count() expects (string_or_list, substring_or_value)\n");
        return false;
    }
    if (IS_STRING(args[0]) && IS_STRING(args[1])) {
        ObjString* str = AS_STRING(args[0]);
        ObjString* sub = AS_STRING(args[1]);
        int count = 0;
        if (sub->length == 0) {
            *result = NUMBER_VAL((double)(str->length + 1));
            return true;
        }
        for (int i = 0; i <= str->length - sub->length; i++) {
            if (memcmp(&str->chars[i], sub->chars, sub->length) == 0) {
                count++;
                i += sub->length - 1;
            }
        }
        *result = NUMBER_VAL((double)count);
        return true;
    }
    if (IS_LIST(args[0])) {
        ObjList* list = AS_LIST(args[0]);
        int count = 0;
        for (int i = 0; i < list->count; i++) {
            if (values_equal(list->values[i], args[1])) count++;
        }
        *result = NUMBER_VAL((double)count);
        return true;
    }
    fprintf(stderr, "JTS GO: count() expects a string or list\n");
    return false;
}

static bool native_capitalize(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: capitalize() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    char* buf = ALLOCATE(char, str->length + 1);
    for (int i = 0; i < str->length; i++) {
        if (i == 0 && str->chars[i] >= 'a' && str->chars[i] <= 'z') {
            buf[i] = str->chars[i] - 32;
        } else if (i > 0 && str->chars[i] >= 'A' && str->chars[i] <= 'Z') {
            buf[i] = str->chars[i] + 32;
        } else {
            buf[i] = str->chars[i];
        }
    }
    buf[str->length] = '\0';
    *result = OBJ_VAL(take_string(buf, str->length));
    return true;
}

static bool native_title(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: title() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    char* buf = ALLOCATE(char, str->length + 1);
    bool new_word = true;
    for (int i = 0; i < str->length; i++) {
        char c = str->chars[i];
        if (c == ' ' || c == '\t' || c == '\n') {
            buf[i] = c;
            new_word = true;
        } else {
            if (new_word && c >= 'a' && c <= 'z') c -= 32;
            else if (!new_word && c >= 'A' && c <= 'Z') c += 32;
            buf[i] = c;
            new_word = false;
        }
    }
    buf[str->length] = '\0';
    *result = OBJ_VAL(take_string(buf, str->length));
    return true;
}

static bool native_swapcase(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: swapcase() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    char* buf = ALLOCATE(char, str->length + 1);
    for (int i = 0; i < str->length; i++) {
        char c = str->chars[i];
        if (c >= 'a' && c <= 'z') buf[i] = c - 32;
        else if (c >= 'A' && c <= 'Z') buf[i] = c + 32;
        else buf[i] = c;
    }
    buf[str->length] = '\0';
    *result = OBJ_VAL(take_string(buf, str->length));
    return true;
}

static bool native_is_digit(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: is_digit() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) { *result = BOOL_VAL(false); return true; }
    for (int i = 0; i < str->length; i++) {
        if (str->chars[i] < '0' || str->chars[i] > '9') { *result = BOOL_VAL(false); return true; }
    }
    *result = BOOL_VAL(true);
    return true;
}

static bool native_is_alpha(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: is_alpha() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) { *result = BOOL_VAL(false); return true; }
    for (int i = 0; i < str->length; i++) {
        char c = str->chars[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) { *result = BOOL_VAL(false); return true; }
    }
    *result = BOOL_VAL(true);
    return true;
}

static bool native_is_alnum(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: is_alnum() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) { *result = BOOL_VAL(false); return true; }
    for (int i = 0; i < str->length; i++) {
        char c = str->chars[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) { *result = BOOL_VAL(false); return true; }
    }
    *result = BOOL_VAL(true);
    return true;
}

static bool native_is_space(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: is_space() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    if (str->length == 0) { *result = BOOL_VAL(false); return true; }
    for (int i = 0; i < str->length; i++) {
        char c = str->chars[i];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') { *result = BOOL_VAL(false); return true; }
    }
    *result = BOOL_VAL(true);
    return true;
}

static bool native_is_upper(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: is_upper() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    bool has_cased = false;
    for (int i = 0; i < str->length; i++) {
        char c = str->chars[i];
        if (c >= 'a' && c <= 'z') { *result = BOOL_VAL(false); return true; }
        if (c >= 'A' && c <= 'Z') has_cased = true;
    }
    *result = BOOL_VAL(has_cased);
    return true;
}

static bool native_is_lower(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: is_lower() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    bool has_cased = false;
    for (int i = 0; i < str->length; i++) {
        char c = str->chars[i];
        if (c >= 'A' && c <= 'Z') { *result = BOOL_VAL(false); return true; }
        if (c >= 'a' && c <= 'z') has_cased = true;
    }
    *result = BOOL_VAL(has_cased);
    return true;
}

static bool native_zfill(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: zfill() expects (string, width)\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    int width = (int)AS_NUMBER(args[1]);
    if (width <= str->length) { *result = args[0]; return true; }
    char* buf = ALLOCATE(char, width + 1);
    memset(buf, '0', width);
    memcpy(buf + (width - str->length), str->chars, str->length);
    buf[width] = '\0';
    *result = OBJ_VAL(take_string(buf, width));
    return true;
}

static bool native_ljust(int arg_count, Value* args, Value* result) {
    if (arg_count < 2 || arg_count > 3 || !IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: ljust() expects (string, width[, fill])\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    int width = (int)AS_NUMBER(args[1]);
    char fill = ' ';
    if (arg_count == 3) {
        if (!IS_STRING(args[2]) || AS_STRING(args[2])->length == 0) { fprintf(stderr, "JTS GO: ljust() fill must be a non-empty string\n"); return false; }
        fill = AS_STRING(args[2])->chars[0];
    }
    if (width <= str->length) { *result = args[0]; return true; }
    char* buf = ALLOCATE(char, width + 1);
    memcpy(buf, str->chars, str->length);
    memset(buf + str->length, fill, width - str->length);
    buf[width] = '\0';
    *result = OBJ_VAL(take_string(buf, width));
    return true;
}

static bool native_rjust(int arg_count, Value* args, Value* result) {
    if (arg_count < 2 || arg_count > 3 || !IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: rjust() expects (string, width[, fill])\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    int width = (int)AS_NUMBER(args[1]);
    char fill = ' ';
    if (arg_count == 3) {
        if (!IS_STRING(args[2]) || AS_STRING(args[2])->length == 0) { fprintf(stderr, "JTS GO: rjust() fill must be a non-empty string\n"); return false; }
        fill = AS_STRING(args[2])->chars[0];
    }
    if (width <= str->length) { *result = args[0]; return true; }
    char* buf = ALLOCATE(char, width + 1);
    memset(buf, fill, width - str->length);
    memcpy(buf + (width - str->length), str->chars, str->length);
    buf[width] = '\0';
    *result = OBJ_VAL(take_string(buf, width));
    return true;
}

static bool native_center(int arg_count, Value* args, Value* result) {
    if (arg_count < 2 || arg_count > 3 || !IS_STRING(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: center() expects (string, width[, fill])\n");
        return false;
    }
    ObjString* str = AS_STRING(args[0]);
    int width = (int)AS_NUMBER(args[1]);
    char fill = ' ';
    if (arg_count == 3) {
        if (!IS_STRING(args[2]) || AS_STRING(args[2])->length == 0) { fprintf(stderr, "JTS GO: center() fill must be a non-empty string\n"); return false; }
        fill = AS_STRING(args[2])->chars[0];
    }
    if (width <= str->length) { *result = args[0]; return true; }
    int total_pad = width - str->length;
    int left = total_pad / 2;
    int right = total_pad - left;
    char* buf = ALLOCATE(char, width + 1);
    memset(buf, fill, width);
    memcpy(buf + left, str->chars, str->length);
    buf[width] = '\0';
    *result = OBJ_VAL(take_string(buf, width));
    return true;
}

static bool native_join(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_STRING(args[0]) || !IS_LIST(args[1])) {
        fprintf(stderr, "JTS GO: join() expects (separator, list)\n");
        return false;
    }
    ObjString* sep = AS_STRING(args[0]);
    ObjList* list = AS_LIST(args[1]);
    int total = 0;
    for (int i = 0; i < list->count; i++) {
        if (!IS_STRING(list->values[i])) {
            fprintf(stderr, "JTS GO: join() list elements must be strings\n");
            return false;
        }
        total += AS_STRING(list->values[i])->length;
        if (i > 0) total += sep->length;
    }
    char* buf = ALLOCATE(char, total + 1);
    int pos = 0;
    for (int i = 0; i < list->count; i++) {
        if (i > 0) {
            memcpy(buf + pos, sep->chars, sep->length);
            pos += sep->length;
        }
        ObjString* item = AS_STRING(list->values[i]);
        memcpy(buf + pos, item->chars, item->length);
        pos += item->length;
    }
    buf[pos] = '\0';
    *result = OBJ_VAL(take_string(buf, pos));
    return true;
}

static bool native_lstrip(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: lstrip() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    int start = 0;
    while (start < str->length && (str->chars[start] == ' ' || str->chars[start] == '\t' || str->chars[start] == '\n' || str->chars[start] == '\r')) start++;
    *result = OBJ_VAL(copy_string(&str->chars[start], str->length - start));
    return true;
}

static bool native_rstrip(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: rstrip() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    int end = str->length;
    while (end > 0 && (str->chars[end - 1] == ' ' || str->chars[end - 1] == '\t' || str->chars[end - 1] == '\n' || str->chars[end - 1] == '\r')) end--;
    *result = OBJ_VAL(copy_string(str->chars, end));
    return true;
}

static bool native_splitlines(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: splitlines() expects 1 string argument\n"); return false; }
    ObjString* str = AS_STRING(args[0]);
    ObjList* list = new_list();
    int start = 0;
    for (int i = 0; i < str->length; i++) {
        if (str->chars[i] == '\n') {
            int len = i - start;
            if (len > 0 && str->chars[i - 1] == '\r') len--;
            list_append(list, OBJ_VAL(copy_string(&str->chars[start], len)));
            start = i + 1;
        }
    }
    list_append(list, OBJ_VAL(copy_string(&str->chars[start], str->length - start)));
    *result = OBJ_VAL(list);
    return true;
}

static bool native_format(int arg_count, Value* args, Value* result) {
    if (arg_count < 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: format() expects (format_string, ...)\n");
        return false;
    }
    ObjString* fmt = AS_STRING(args[0]);
    char* buf = ALLOCATE(char, fmt->length * 8 + 1);
    int pos = 0;
    int arg_idx = 1;
    for (int i = 0; i < fmt->length; i++) {
        if (fmt->chars[i] == '{' && i + 1 < fmt->length && fmt->chars[i + 1] == '}') {
            if (arg_idx < arg_count) {
                ObjString* s = value_to_display_string(args[arg_idx++]);
                memcpy(buf + pos, s->chars, s->length);
                pos += s->length;
            }
            i++;
        } else if (fmt->chars[i] == '{') {
            int j = i + 1;
            while (j < fmt->length && fmt->chars[j] >= '0' && fmt->chars[j] <= '9') j++;
            if (j < fmt->length && fmt->chars[j] == '}') {
                int idx = atoi(&fmt->chars[i + 1]);
                if (idx + 1 < arg_count) {
                    ObjString* s = value_to_display_string(args[idx + 1]);
                    memcpy(buf + pos, s->chars, s->length);
                    pos += s->length;
                }
                i = j;
            } else {
                buf[pos++] = fmt->chars[i];
            }
        } else {
            buf[pos++] = fmt->chars[i];
        }
    }
    buf[pos] = '\0';
    *result = OBJ_VAL(take_string(buf, pos));
    return true;
}

static bool native_list_insert(int arg_count, Value* args, Value* result) {
    if (arg_count != 3 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
        fprintf(stderr, "JTS GO: insert() expects (list, index, value)\n");
        return false;
    }
    ObjList* list = AS_LIST(args[0]);
    int index = (int)AS_NUMBER(args[1]);
    if (index < 0) index = list->count + index;
    if (index < 0) index = 0;
    if (index > list->count) index = list->count;
    list_append(list, VOID_VAL);
    for (int i = list->count - 1; i > index; i--) {
        list->values[i] = list->values[i - 1];
    }
    list->values[index] = args[2];
    *result = args[0];
    return true;
}

static bool native_list_extend(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_LIST(args[0]) || !IS_LIST(args[1])) {
        fprintf(stderr, "JTS GO: extend() expects (list, other_list)\n");
        return false;
    }
    ObjList* list = AS_LIST(args[0]);
    ObjList* other = AS_LIST(args[1]);
    for (int i = 0; i < other->count; i++) list_append(list, other->values[i]);
    *result = args[0];
    return true;
}

static bool native_list_clear(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) { fprintf(stderr, "JTS GO: clear() expects 1 list or dict argument\n"); return false; }
    if (IS_LIST(args[0])) {
        AS_LIST(args[0])->count = 0;
    } else if (IS_DICT(args[0])) {
        for (int i = 0; i < AS_DICT(args[0])->entries.capacity; i++) {
            AS_DICT(args[0])->entries.entries[i].key = NULL;
        }
        AS_DICT(args[0])->entries.count = 0;
        AS_DICT(args[0])->entries.live = 0;
    } else {
        fprintf(stderr, "JTS GO: clear() argument must be a list or dict\n");
        return false;
    }
    *result = args[0];
    return true;
}

static bool native_list_index(int arg_count, Value* args, Value* result) {
    if (arg_count != 2) { fprintf(stderr, "JTS GO: index() expects (string_or_list, value)\n"); return false; }
    if (IS_STRING(args[0]) && IS_STRING(args[1])) {
        ObjString* str = AS_STRING(args[0]);
        ObjString* sub = AS_STRING(args[1]);
        if (sub->length == 0) { *result = NUMBER_VAL(0); return true; }
        for (int i = 0; i <= str->length - sub->length; i++) {
            if (memcmp(&str->chars[i], sub->chars, sub->length) == 0) {
                *result = NUMBER_VAL((double)i);
                return true;
            }
        }
        *result = NUMBER_VAL(-1);
        return true;
    }
    if (IS_LIST(args[0])) {
        ObjList* list = AS_LIST(args[0]);
        for (int i = 0; i < list->count; i++) {
            if (values_equal(list->values[i], args[1])) {
                *result = NUMBER_VAL((double)i);
                return true;
            }
        }
        *result = NUMBER_VAL(-1);
        return true;
    }
    fprintf(stderr, "JTS GO: index() expects a string or list\n");
    return false;
}

static bool native_list_reverse(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_LIST(args[0])) { fprintf(stderr, "JTS GO: reverse() expects 1 list argument\n"); return false; }
    ObjList* list = AS_LIST(args[0]);
    for (int i = 0, j = list->count - 1; i < j; i++, j--) {
        Value temp = list->values[i];
        list->values[i] = list->values[j];
        list->values[j] = temp;
    }
    *result = args[0];
    return true;
}

static bool native_list_copy(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_LIST(args[0])) { fprintf(stderr, "JTS GO: copy() expects 1 list argument\n"); return false; }
    ObjList* src = AS_LIST(args[0]);
    ObjList* copy = new_list();
    for (int i = 0; i < src->count; i++) list_append(copy, src->values[i]);
    *result = OBJ_VAL(copy);
    return true;
}

static bool native_list_convert(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: list() expects 1 argument (a string, set, tensor, or list)\n");
        return false;
    }
    ObjList* list = new_list();
    if (IS_STRING(args[0])) {
        ObjString* str = AS_STRING(args[0]);
        for (int i = 0; i < str->length; i++) {
            list_append(list, OBJ_VAL(copy_string(&str->chars[i], 1)));
        }
    } else if (IS_SET(args[0])) {
        ObjSet* src = AS_SET(args[0]);
        for (int i = 0; i < src->count; i++) list_append(list, src->values[i]);
    } else if (IS_TENSOR(args[0])) {
        ObjTensor* t = AS_TENSOR(args[0]);
        for (int i = 0; i < t->size; i++) list_append(list, NUMBER_VAL(t->data[i]));
    } else if (IS_LIST(args[0])) {
        ObjList* src = AS_LIST(args[0]);
        for (int i = 0; i < src->count; i++) list_append(list, src->values[i]);
    } else {
        fprintf(stderr, "JTS GO: list() expects a string, set, tensor, or list\n");
        return false;
    }
    *result = OBJ_VAL(list);
    return true;
}

static bool native_set(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: set() expects 1 argument (a list)\n");
        return false;
    }
    ObjSet* set = new_set();
    if (IS_LIST(args[0])) {
        ObjList* list = AS_LIST(args[0]);
        for (int i = 0; i < list->count; i++) set_add(set, list->values[i]);
    } else if (IS_SET(args[0])) {
        ObjSet* src = AS_SET(args[0]);
        for (int i = 0; i < src->count; i++) set_add(set, src->values[i]);
    } else if (IS_STRING(args[0])) {
        ObjString* str = AS_STRING(args[0]);
        for (int i = 0; i < str->length; i++) {
            set_add(set, OBJ_VAL(copy_string(&str->chars[i], 1)));
        }
    } else {
        fprintf(stderr, "JTS GO: set() expects a list, set, or string\n");
        return false;
    }
    *result = OBJ_VAL(set);
    return true;
}

static bool native_set_add(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_SET(args[0])) { fprintf(stderr, "JTS GO: set_add() expects (set, value)\n"); return false; }
    set_add(AS_SET(args[0]), args[1]);
    *result = args[0];
    return true;
}

static bool native_set_remove(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_SET(args[0])) { fprintf(stderr, "JTS GO: set_remove() expects (set, value)\n"); return false; }
    set_remove(AS_SET(args[0]), args[1]);
    *result = args[0];
    return true;
}

static bool native_set_contains(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_SET(args[0])) { fprintf(stderr, "JTS GO: set_contains() expects (set, value)\n"); return false; }
    *result = BOOL_VAL(set_contains(AS_SET(args[0]), args[1]));
    return true;
}

static bool native_set_union(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_SET(args[0]) || !IS_SET(args[1])) { fprintf(stderr, "JTS GO: set_union() expects (set, set)\n"); return false; }
    ObjSet* out = new_set();
    ObjSet* a = AS_SET(args[0]);
    ObjSet* b = AS_SET(args[1]);
    for (int i = 0; i < a->count; i++) set_add(out, a->values[i]);
    for (int i = 0; i < b->count; i++) set_add(out, b->values[i]);
    *result = OBJ_VAL(out);
    return true;
}

static bool native_set_intersection(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_SET(args[0]) || !IS_SET(args[1])) { fprintf(stderr, "JTS GO: set_intersection() expects (set, set)\n"); return false; }
    ObjSet* out = new_set();
    ObjSet* a = AS_SET(args[0]);
    ObjSet* b = AS_SET(args[1]);
    for (int i = 0; i < a->count; i++) {
        if (set_contains(b, a->values[i])) set_add(out, a->values[i]);
    }
    *result = OBJ_VAL(out);
    return true;
}

static bool native_set_difference(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_SET(args[0]) || !IS_SET(args[1])) { fprintf(stderr, "JTS GO: set_difference() expects (set, set)\n"); return false; }
    ObjSet* out = new_set();
    ObjSet* a = AS_SET(args[0]);
    ObjSet* b = AS_SET(args[1]);
    for (int i = 0; i < a->count; i++) {
        if (!set_contains(b, a->values[i])) set_add(out, a->values[i]);
    }
    *result = OBJ_VAL(out);
    return true;
}

static bool native_dict_keys(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_DICT(args[0])) { fprintf(stderr, "JTS GO: keys() expects 1 dict argument\n"); return false; }
    ObjDict* dict = AS_DICT(args[0]);
    ObjList* list = new_list();
    for (int i = 0; i < dict->entries.capacity; i++) {
        if (dict->entries.entries[i].key != NULL) list_append(list, OBJ_VAL(dict->entries.entries[i].key));
    }
    *result = OBJ_VAL(list);
    return true;
}

static bool native_dict_values(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_DICT(args[0])) { fprintf(stderr, "JTS GO: values() expects 1 dict argument\n"); return false; }
    ObjDict* dict = AS_DICT(args[0]);
    ObjList* list = new_list();
    for (int i = 0; i < dict->entries.capacity; i++) {
        if (dict->entries.entries[i].key != NULL) list_append(list, dict->entries.entries[i].value);
    }
    *result = OBJ_VAL(list);
    return true;
}

static bool native_dict_items(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_DICT(args[0])) { fprintf(stderr, "JTS GO: items() expects 1 dict argument\n"); return false; }
    ObjDict* dict = AS_DICT(args[0]);
    ObjList* list = new_list();
    for (int i = 0; i < dict->entries.capacity; i++) {
        if (dict->entries.entries[i].key != NULL) {
            ObjList* pair = new_list();
            list_append(pair, OBJ_VAL(dict->entries.entries[i].key));
            list_append(pair, dict->entries.entries[i].value);
            list_append(list, OBJ_VAL(pair));
        }
    }
    *result = OBJ_VAL(list);
    return true;
}

static bool native_dict_get(int arg_count, Value* args, Value* result) {
    if (arg_count < 2 || arg_count > 3 || !IS_DICT(args[0])) {
        fprintf(stderr, "JTS GO: get() expects (dict, key[, default])\n");
        return false;
    }
    ObjDict* dict = AS_DICT(args[0]);
    if (IS_STRING(args[1])) {
        ObjString* key = AS_STRING(args[1]);
        Value value;
        if (dict_get(dict, key, &value)) {
            *result = value;
            return true;
        }
    }
    if (arg_count == 3) { *result = args[2]; return true; }
    *result = VOID_VAL;
    return true;
}

static bool native_dict_pop(int arg_count, Value* args, Value* result) {
    if (arg_count < 2 || arg_count > 3 || !IS_DICT(args[0])) {
        fprintf(stderr, "JTS GO: pop() expects (dict, key[, default])\n");
        return false;
    }
    ObjDict* dict = AS_DICT(args[0]);
    if (IS_STRING(args[1])) {
        ObjString* key = AS_STRING(args[1]);
        Value value;
        if (dict_get(dict, key, &value)) {
            table_delete(&dict->entries, key);
            *result = value;
            return true;
        }
    }
    if (arg_count == 3) { *result = args[2]; return true; }
    *result = VOID_VAL;
    return true;
}

static bool native_dict_has(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_DICT(args[0])) { fprintf(stderr, "JTS GO: has() expects (dict, key)\n"); return false; }
    ObjDict* dict = AS_DICT(args[0]);
    if (IS_STRING(args[1])) {
        Value value;
        *result = BOOL_VAL(dict_get(dict, AS_STRING(args[1]), &value));
    } else {
        *result = BOOL_VAL(false);
    }
    return true;
}

static bool native_dict_update(int arg_count, Value* args, Value* result) {
    if (arg_count != 2 || !IS_DICT(args[0]) || !IS_DICT(args[1])) {
        fprintf(stderr, "JTS GO: update() expects (dict, other_dict)\n");
        return false;
    }
    ObjDict* dest = AS_DICT(args[0]);
    ObjDict* src = AS_DICT(args[1]);
    for (int i = 0; i < src->entries.capacity; i++) {
        if (src->entries.entries[i].key != NULL) {
            dict_set(dest, src->entries.entries[i].key, src->entries.entries[i].value);
        }
    }
    *result = args[0];
    return true;
}

static bool native_dict_clear(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_DICT(args[0])) { fprintf(stderr, "JTS GO: clear() expects 1 dict argument\n"); return false; }
    for (int i = 0; i < AS_DICT(args[0])->entries.capacity; i++) {
        AS_DICT(args[0])->entries.entries[i].key = NULL;
    }
    AS_DICT(args[0])->entries.count = 0;
    AS_DICT(args[0])->entries.live = 0;
    *result = args[0];
    return true;
}

static Value json_parse_value(const char** p);

static bool json_skip_ws(const char** p) {
    while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
    return **p != '\0';
}

static Value json_parse_string(const char** p) {
    (*p)++;
    char buf[4096];
    int pos = 0;
    while (**p && **p != '"') {
        if (**p == '\\') {
            (*p)++;
            char c = **p;
            switch (c) {
                case 'n': buf[pos++] = '\n'; break;
                case 't': buf[pos++] = '\t'; break;
                case 'r': buf[pos++] = '\r'; break;
                case '\\': buf[pos++] = '\\'; break;
                case '"': buf[pos++] = '"'; break;
                case '/': buf[pos++] = '/'; break;
                case 'b': buf[pos++] = '\b'; break;
                case 'f': buf[pos++] = '\f'; break;
                default: buf[pos++] = c; break;
            }
            (*p)++;
        } else {
            buf[pos++] = **p;
            (*p)++;
        }
    }
    if (**p) (*p)++;
    return OBJ_VAL(copy_string(buf, pos));
}

static Value json_parse_value(const char** p) {
    json_skip_ws(p);
    if (**p == '{') {
        (*p)++;
        ObjDict* dict = new_dict();
        json_skip_ws(p);
        if (**p == '}') { (*p)++; return OBJ_VAL(dict); }
        while (**p) {
            json_skip_ws(p);
            if (**p != '"') { (*p)++; break; }
            Value key = json_parse_string(p);
            json_skip_ws(p);
            if (**p == ':') (*p)++;
            Value val = json_parse_value(p);
            dict_set(dict, AS_STRING(key), val);
            json_skip_ws(p);
            if (**p == ',') { (*p)++; continue; }
            if (**p == '}') { (*p)++; break; }
            (*p)++;
        }
        return OBJ_VAL(dict);
    }
    if (**p == '[') {
        (*p)++;
        ObjList* list = new_list();
        json_skip_ws(p);
        if (**p == ']') { (*p)++; return OBJ_VAL(list); }
        while (**p) {
            list_append(list, json_parse_value(p));
            json_skip_ws(p);
            if (**p == ',') { (*p)++; continue; }
            if (**p == ']') { (*p)++; break; }
            (*p)++;
        }
        return OBJ_VAL(list);
    }
    if (**p == '"') return json_parse_string(p);
    if (**p == 't' && strncmp(*p, "true", 4) == 0) { *p += 4; return BOOL_VAL(true); }
    if (**p == 'f' && strncmp(*p, "false", 5) == 0) { *p += 5; return BOOL_VAL(false); }
    if (**p == 'n' && strncmp(*p, "null", 4) == 0) { *p += 4; return VOID_VAL; }
    if (**p == '-' || (**p >= '0' && **p <= '9')) {
        char* end;
        double v = strtod(*p, &end);
        if (end == *p) { (*p)++; return VOID_VAL; }
        *p = end;
        return NUMBER_VAL(v);
    }
    (*p)++;
    return VOID_VAL;
}

static bool native_json_parse(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: json_parse() expects 1 string argument\n");
        return false;
    }
    const char* p = AS_CSTRING(args[0]);
    *result = json_parse_value(&p);
    return true;
}

static void json_stringify_value(Value v, char* buf, int* pos, int depth) {
    if (depth > 64) return;
    if (IS_NUMBER(v)) {
        ObjString* s = AS_STRING(number_to_string(AS_NUMBER(v)));
        memcpy(buf + *pos, s->chars, s->length);
        *pos += s->length;
    } else if (IS_BOOL(v)) {
        if (AS_BOOL(v)) { memcpy(buf + *pos, "true", 4); *pos += 4; }
        else { memcpy(buf + *pos, "false", 5); *pos += 5; }
    } else if (IS_VOID(v)) {
        memcpy(buf + *pos, "null", 4);
        *pos += 4;
    } else if (IS_STRING(v)) {
        ObjString* s = AS_STRING(v);
        buf[(*pos)++] = '"';
        for (int i = 0; i < s->length; i++) {
            char c = s->chars[i];
            switch (c) {
                case '"': memcpy(buf + *pos, "\\\"", 2); *pos += 2; break;
                case '\\': memcpy(buf + *pos, "\\\\", 2); *pos += 2; break;
                case '\n': memcpy(buf + *pos, "\\n", 2); *pos += 2; break;
                case '\t': memcpy(buf + *pos, "\\t", 2); *pos += 2; break;
                case '\r': memcpy(buf + *pos, "\\r", 2); *pos += 2; break;
                default: buf[(*pos)++] = c; break;
            }
        }
        buf[(*pos)++] = '"';
    } else if (IS_LIST(v)) {
        ObjList* list = AS_LIST(v);
        buf[(*pos)++] = '[';
        for (int i = 0; i < list->count; i++) {
            if (i > 0) buf[(*pos)++] = ',';
            json_stringify_value(list->values[i], buf, pos, depth + 1);
        }
        buf[(*pos)++] = ']';
    } else if (IS_DICT(v)) {
        ObjDict* dict = AS_DICT(v);
        buf[(*pos)++] = '{';
        int written = 0;
        for (int i = 0; i < dict->entries.capacity; i++) {
            if (dict->entries.entries[i].key != NULL) {
                if (written > 0) buf[(*pos)++] = ',';
                json_stringify_value(OBJ_VAL(dict->entries.entries[i].key), buf, pos, depth + 1);
                buf[(*pos)++] = ':';
                json_stringify_value(dict->entries.entries[i].value, buf, pos, depth + 1);
                written++;
            }
        }
        buf[(*pos)++] = '}';
    } else {
        memcpy(buf + *pos, "null", 4);
        *pos += 4;
    }
}

static bool native_json_stringify(int arg_count, Value* args, Value* result) {
    if (arg_count != 1) {
        fprintf(stderr, "JTS GO: json_stringify() expects 1 argument\n");
        return false;
    }
    char buf[65536];
    int pos = 0;
    json_stringify_value(args[0], buf, &pos, 0);
    buf[pos] = '\0';
    *result = OBJ_VAL(copy_string(buf, pos));
    return true;
}

static bool native_now(int arg_count, Value* args, Value* result) {
    if (arg_count != 0) { fprintf(stderr, "JTS GO: now() expects 0 arguments\n"); return false; }
    *result = NUMBER_VAL((double)time(NULL));
    return true;
}

static bool native_sleep(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_NUMBER(args[0])) { fprintf(stderr, "JTS GO: sleep() expects 1 number argument\n"); return false; }
    double secs = AS_NUMBER(args[0]);
#ifdef _WIN32
    Sleep((DWORD)(secs * 1000));
#else
    struct timespec ts;
    ts.tv_sec = (time_t)secs;
    ts.tv_nsec = (long)((secs - (double)ts.tv_sec) * 1000000000);
    nanosleep(&ts, NULL);
#endif
    *result = VOID_VAL;
    return true;
}

static bool native_strftime(int arg_count, Value* args, Value* result) {
    if (arg_count < 1 || arg_count > 2 || !IS_STRING(args[0])) {
        fprintf(stderr, "JTS GO: strftime() expects (format[, timestamp])\n");
        return false;
    }
    time_t t = time(NULL);
    if (arg_count == 2) {
        if (!IS_NUMBER(args[1])) { fprintf(stderr, "JTS GO: strftime() timestamp must be a number\n"); return false; }
        t = (time_t)AS_NUMBER(args[1]);
    }
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[256];
    const char* fmt = AS_CSTRING(args[0]);
    if (strcmp(fmt, "%s") == 0) {
        snprintf(buf, sizeof(buf), "%lld", (long long)t);
        *result = OBJ_VAL(copy_string(buf, (int)strlen(buf)));
        return true;
    }
    strftime(buf, sizeof(buf), fmt, &tmv);
    *result = OBJ_VAL(copy_string(buf, (int)strlen(buf)));
    return true;
}

static bool native_env(int arg_count, Value* args, Value* result) {
    if (arg_count != 1 || !IS_STRING(args[0])) { fprintf(stderr, "JTS GO: env() expects 1 string argument\n"); return false; }
    const char* val = getenv(AS_CSTRING(args[0]));
    if (val == NULL) { *result = VOID_VAL; return true; }
    *result = OBJ_VAL(copy_string(val, (int)strlen(val)));
    return true;
}

static bool native_args(int arg_count, Value* args, Value* result) {
    if (arg_count != 0) { fprintf(stderr, "JTS GO: args() expects 0 arguments\n"); return false; }
    ObjList* list = new_list();
    for (int i = 1; i < jts_argc; i++) {
        list_append(list, OBJ_VAL(copy_string(jts_argv[i], (int)strlen(jts_argv[i]))));
    }
    *result = OBJ_VAL(list);
    return true;
}

static bool native_exit(int arg_count, Value* args, Value* result) {
    int code = 0;
    if (arg_count > 1 || (arg_count == 1 && !IS_NUMBER(args[0]))) {
        fprintf(stderr, "JTS GO: exit() expects 0 or 1 number argument\n");
        return false;
    }
    if (arg_count == 1) code = (int)AS_NUMBER(args[0]);
    exit(code);
    return true;
}

static bool native_cwd(int arg_count, Value* args, Value* result) {
    if (arg_count != 0) { fprintf(stderr, "JTS GO: cwd() expects 0 arguments\n"); return false; }
    char buf[4096];
    if (GET_CWD(buf, sizeof(buf)) == NULL) {
        fprintf(stderr, "JTS GO: cwd() failed\n");
        return false;
    }
    *result = OBJ_VAL(copy_string(buf, (int)strlen(buf)));
    return true;
}

typedef struct {
    const char* name;
    int arity;
    bool (*function)(int arg_count, Value* args, Value* result);
} NativeDef;

static NativeDef native_functions[] = {
    {"say", -1, native_print},
    {"ask", -1, native_input},
    {"len",    1, native_len},
    {"type",   1, native_type},
    {"append", 2, native_append},
    {"number", 1, native_number},
    {"str",    1, native_string},
    {"string", 1, native_string},
    {"math",   2, native_math},
    {"tensor", 1, native_tensor},
    {"matrix", -1, native_matrix},
    {"matmul", 2, native_matmul},
    {"sigmoid", 1, native_sigmoid},
    {"relu",  1, native_relu},
    {"mse",   2, native_mse},
    {"http_server", -1, native_http_server},
    {"http_route",  4, native_http_route},
    {"http_start",  1, native_http_start},
    {"http_request", -1, native_http_request},
    {"sqrt",   1, native_sqrt},
    {"file_exists", 1, native_file_exists},
    {"sin",    1, native_sin},
    {"cos",    1, native_cos},
    {"tan",    1, native_tan},
    {"log",    1, native_log},
    {"exp",    1, native_exp},
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
    {"sort", -1, native_sort},
    // Range
    {"range", -1, native_range},
    // Math
    {"abs", 1, native_abs},
    {"min", -1, native_min},
    {"max", -1, native_max},
    {"sum", 1, native_sum},
    {"pow", 2, native_pow},
    {"round", -1, native_round},
    {"floor", 1, native_floor},
    {"ceil", 1, native_ceil},
    {"rand", 0, native_rand},
    {"randint", 2, native_randint},
    {"seed", 1, native_seed},
    {"shuffle", 1, native_shuffle},
    // Conversions
    {"int", 1, native_int},
    {"float", 1, native_float},
    {"bool", 1, native_bool},
    // String methods
    {"find", 2, native_find},
    {"count", 2, native_count},
    {"capitalize", 1, native_capitalize},
    {"title", 1, native_title},
    {"swapcase", 1, native_swapcase},
    {"is_digit", 1, native_is_digit},
    {"is_alpha", 1, native_is_alpha},
    {"is_alnum", 1, native_is_alnum},
    {"is_space", 1, native_is_space},
    {"is_upper", 1, native_is_upper},
    {"is_lower", 1, native_is_lower},
    {"zfill", 2, native_zfill},
    {"ljust", -1, native_ljust},
    {"rjust", -1, native_rjust},
    {"center", -1, native_center},
    {"join", 2, native_join},
    {"lstrip", 1, native_lstrip},
    {"rstrip", 1, native_rstrip},
    {"splitlines", 1, native_splitlines},
    {"format", -1, native_format},
    // List methods
    {"insert", 3, native_list_insert},
    {"extend", 2, native_list_extend},
    {"clear", 1, native_list_clear},
    {"copy", 1, native_list_copy},
    {"reverse", 1, native_list_reverse},
    {"index", 2, native_list_index},
    // Set operations
    {"set", 1, native_set},
    {"list", 1, native_list_convert},
    {"set_add", 2, native_set_add},
    {"set_remove", 2, native_set_remove},
    {"set_contains", 2, native_set_contains},
    {"set_union", 2, native_set_union},
    {"set_intersection", 2, native_set_intersection},
    {"set_difference", 2, native_set_difference},
    // Dict methods
    {"keys", 1, native_dict_keys},
    {"values", 1, native_dict_values},
    {"items", 1, native_dict_items},
    {"get", -1, native_dict_get},
    {"has", 2, native_dict_has},
    {"update", 2, native_dict_update},
    // JSON
    {"json_parse", 1, native_json_parse},
    {"json_stringify", 1, native_json_stringify},
    // Time
    {"now", 0, native_now},
    {"sleep", 1, native_sleep},
    {"strftime", -1, native_strftime},
    // OS
    {"env", 1, native_env},
    {"args", 0, native_args},
    {"exit", -1, native_exit},
    {"cwd", 0, native_cwd},
    // File I/O
    {"read_file", 1, native_read_file},
    {"write_file", 2, native_write_file},
    // Import
    {"import_file", 1, native_import_file},
    {"bring_scroll", 1, native_bring_scroll},
    // Generator
    {"next", 1, native_next},
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
