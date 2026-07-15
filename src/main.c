#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "vm/vm.h"
#include "compiler/compiler.h"
#include "core/chunk.h"
#include "core/memory.h"
#include "io/fileio.h"

static ToolType detect_tool(const char* program_name) {
    const char* name = program_name;
    const char* p;

    // Find the last path separator
    p = strrchr(name, '/');
    if (p) name = p + 1;
    p = strrchr(name, '\\');
    if (p) name = p + 1;

    // Remove .exe extension on Windows
    size_t len = strlen(name);
    if (len > 4 && strcmp(name + len - 4, ".exe") == 0) {
        len -= 4;
    }

    if (len == 4 && strncmp(name, "jtsc", 4) == 0) return TOOL_JTSC;
    if (len == 5 && strncmp(name, "jtsvm", 5) == 0) return TOOL_JTSVM;
    if (len == 3 && strncmp(name, "jts", 3) == 0) return TOOL_JTS;
    return TOOL_JTS; // Default
}

static void print_version(void) {
    printf("JTS GO version %d.%d.%d\n",
           JTS_VERSION_MAJOR, JTS_VERSION_MINOR, JTS_VERSION_PATCH);
}

static void print_usage(const char* program_name) {
    printf("Usage:\n");
    printf("  jtsc <file.jts>              Compile to bytecode (.jbc)\n");
    printf("  jtsvm <file.jbc>             Run bytecode file\n");
    printf("  jts <file.jts>               Compile and run\n");
    printf("  jts                          Interactive REPL\n");
    printf("\nOptions:\n");
    printf("  --version, -v                Show version\n");
    printf("  --update, -u                 Update JTS GO to latest version\n");
    printf("  --help, -h                   Show this help\n");
}

static int run_update(void) {
    printf("Updating JTS GO...\n");
    int result = system("npm update -g @jaytechsolutions/jts-go");
    if (result == 0) {
        printf("Update complete! Run 'jts --version' to verify.\n");
    } else {
        fprintf(stderr, "Update failed. Make sure npm is installed and try again.\n");
        fprintf(stderr, "You can also update manually:\n");
        fprintf(stderr, "  npm install -g @jaytechsolutions/jts-go@latest\n");
    }
    return result;
}

static void repl(void) {
    print_version();
    printf("Type 'exit' or 'quit' to leave.\n\n");

    init_vm();

    char line[2048];
    for (;;) {
        printf("jts> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[--len] = '\0';
        }

        if (len == 0) continue;
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;

        InterpretResult result = interpret(line);
        if (result == INTERPRET_COMPILE_ERROR) {
            printf("Compile error.\n");
        } else if (result == INTERPRET_RUNTIME_ERROR) {
            printf("Runtime error.\n");
        }
    }

    free_vm();
}

static void run_file(const char* path, ToolType tool) {
    // Extract the base name without extension for .jbc path
    char jbc_path[1024];
    strncpy(jbc_path, path, sizeof(jbc_path) - 1);
    jbc_path[sizeof(jbc_path) - 1] = '\0';

    // Replace .jts with .jbc
    size_t path_len = strlen(jbc_path);
    if (path_len > 4 && strcmp(jbc_path + path_len - 4, ".jts") == 0) {
        strcpy(jbc_path + path_len - 4, ".jbc");
    }

    if (tool == TOOL_JTSC) {
        // Compile only
        char* source = read_file(path);
        init_vm();

        Chunk chunk;
        init_chunk(&chunk);

        if (compile(source, &chunk)) {
            if (save_bytecode(&chunk, jbc_path)) {
                printf("Compiled '%s' -> '%s'\n", path, jbc_path);
            } else {
                fprintf(stderr, "Failed to write bytecode to '%s'\n", jbc_path);
            }
        } else {
            fprintf(stderr, "Compilation failed.\n");
        }

        free_chunk(&chunk);
        free_vm();
        free_file(source);

    } else if (tool == TOOL_JTSVM) {
        // Check if it's a .jbc file
        size_t path_len = strlen(path);
        if (path_len > 4 && strcmp(path + path_len - 4, ".jbc") == 0) {
            // Load and run bytecode directly
            Chunk chunk;
            init_chunk(&chunk);
            init_vm();

            if (load_bytecode(&chunk, path)) {
                ObjFunction* function = new_function();
                function->chunk = chunk;
                push(OBJ_VAL(function));
                vm_call(function, 0);
                InterpretResult result = vm_exec();
                free_vm();
                free_chunk(&chunk);
                if (result == INTERPRET_RUNTIME_ERROR) {
                    fprintf(stderr, "Runtime error.\n");
                    exit(70);
                }
            } else {
                fprintf(stderr, "Failed to load bytecode file '%s'\n", path);
                free_vm();
                free_chunk(&chunk);
                exit(65);
            }
        } else {
            // Treat as source file, compile and run
            char* source = read_file(path);
            init_vm();
            InterpretResult result = interpret(source);
            free_vm();
            free_file(source);

            if (result == INTERPRET_COMPILE_ERROR) {
                fprintf(stderr, "Compilation failed.\n");
                exit(65);
            } else if (result == INTERPRET_RUNTIME_ERROR) {
                fprintf(stderr, "Runtime error.\n");
                exit(70);
            }
        }

    } else {
        // TOOL_JTS: compile and run
        char* source = read_file(path);

        init_vm();
        InterpretResult result = interpret(source);
        if (result == INTERPRET_COMPILE_ERROR) {
            fprintf(stderr, "Compilation failed.\n");
            free_file(source);
            exit(65);
        } else if (result == INTERPRET_RUNTIME_ERROR) {
            fprintf(stderr, "Runtime error.\n");
            free_file(source);
            exit(70);
        }

        free_vm();
        free_file(source);
    }
}

int main(int argc, const char* argv[]) {
    ToolType tool = detect_tool(argv[0]);

    if (argc == 1) {
        repl();
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            print_version();
            return 0;
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--update") == 0 || strcmp(argv[i], "-u") == 0) {
            return run_update();
        }
        run_file(argv[i], tool);
    }

    return 0;
}
