#include "xcommon.h"
#include "xmem.h"
#include "xscanner.h"
#include "xversion.h"
#include "xvm.h"
#include "xutils.h"

#define MAX_LINE_SIZE 1024
#define MAX_LINE_HISTORY 100

#ifdef _WIN32
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

// For Unix/Linux
#ifndef _WIN32
static struct termios orig_termios;

static void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static int read_key() {
    int c = getchar();

    if (c == '\x1b') {  // ESC sequence
        int seq[2];
        seq[0] = getchar();
        seq[1] = getchar();

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A':
                    return 1000;  // Up arrow
                case 'B':
                    return 1001;  // Down arrow
                case 'C':
                    return 1002;  // Right arrow
                case 'D':
                    return 1003;  // Left arrow
            }
        }
    }

    return c;
}
#endif

static void repl() {
    printf(COLOR_BOLD COLOR_BRIGHT_BLUE "Xen" COLOR_RESET " %s " COLOR_DIM
                                        "- Copyright (C) 2025 Jake Rieger\n" COLOR_RESET,
           VERSION_STRING_FULL);
    printf(COLOR_ITALIC "Type " COLOR_RESET COLOR_BOLD ".exit" COLOR_RESET COLOR_ITALIC
                        " to quit the REPL.\n\n" COLOR_RESET);

    xen_ring_buffer* line_history = xen_ring_buffer_create(MAX_LINE_HISTORY);

    char line[MAX_LINE_SIZE];
    int history_index = -1;

#ifndef _WIN32
    enable_raw_mode();
#endif

    for (;;) {
        printf("=> ");
        fflush(stdout);

        int pos = 0;
        memset(line, 0, MAX_LINE_SIZE);

        for (;;) {
#ifdef _WIN32
            int c = _getch();
            if (c == 224) {  // Arrow key prefix on Windows
                c = _getch();
                if (c == 72)
                    c = 1000;  // Up
                else if (c == 80)
                    c = 1001;  // Down
            }
#else
            int c = read_key();
#endif

            if (c == 1000) {  // Up arrow
                if (history_index < (int)xen_ring_buffer_count(line_history) - 1) {
                    history_index++;
                    const char* hist = xen_ring_buffer_peek_from_end(line_history, history_index);
                    if (hist) {
                        // Clear current line
                        printf("\r=> ");
                        for (int i = 0; i < pos; i++)
                            printf(" ");
                        printf("\r=> ");

                        strcpy(line, hist);
                        pos = strlen(line);
                        if (line[pos - 1] == '\n') {
                            line[pos - 1] = '\0';
                            pos--;
                        }
                        printf("%s", line);
                        fflush(stdout);
                    }
                }
            } else if (c == 1001) {  // Down arrow
                if (history_index > 0) {
                    history_index--;
                    const char* hist = xen_ring_buffer_peek_from_end(line_history, history_index);
                    if (hist) {
                        printf("\r=> ");
                        for (int i = 0; i < pos; i++)
                            printf(" ");
                        printf("\r=> ");

                        strcpy(line, hist);
                        pos = strlen(line);
                        if (line[pos - 1] == '\n') {
                            line[pos - 1] = '\0';
                            pos--;
                        }
                        printf("%s", line);
                        fflush(stdout);
                    }
                } else if (history_index == 0) {
                    history_index = -1;
                    printf("\r=> ");
                    for (int i = 0; i < pos; i++)
                        printf(" ");
                    printf("\r=> ");
                    memset(line, 0, MAX_LINE_SIZE);
                    pos = 0;
                    fflush(stdout);
                }
            } else if (c == '\n' || c == '\r') {
                line[pos]     = '\n';
                line[pos + 1] = '\0';
                printf("\n");
                break;
            } else if (c == 127 || c == 8) {  // Backspace
                if (pos > 0) {
                    pos--;
                    line[pos] = '\0';
                    printf("\b \b");
                    fflush(stdout);
                }
            } else if (c >= 32 && c < 127) {  // Printable characters
                if (pos < MAX_LINE_SIZE - 2) {
                    line[pos++] = c;
                    printf("%c", c);
                    fflush(stdout);
                }
            }
        }

        if (strcmp(line, ".exit\n") == 0) {
            break;
        }

        if (strlen(line) > 1) {  // Don't add empty lines
            xen_ring_buffer_push(line_history, line);
            history_index = -1;
        }

        xen_exec_result exec_result = xen_vm_exec(line, XEN_FALSE, NULL);
    }

    xen_ring_buffer_free(line_history);
}

static void print_help() {
    printf(COLOR_BOLD COLOR_BRIGHT_BLUE "Xen" COLOR_RESET COLOR_DIM " - Copyright (C) 2025 Jake Rieger\n" COLOR_RESET);
    printf(COLOR_DIM "version " COLOR_RESET COLOR_BOLD VERSION_STRING_FULL COLOR_RESET "\n\n");
    printf("USAGE\n");
    printf("  xen  " COLOR_DIM "-or-" COLOR_RESET "  xen <filename>\n\n");
    printf("ARGUMENTS\n");
    printf("  -h, --help  Show this help page\n");
    printf("\n");
}

static void print_version() {
    printf("version  : " VERSION_STRING "\n");
    printf("platform : " PLATFORM_STRING "\n");
}

static void print_vm_config(const xen_vm_config* config) {
    size_t p_scaled;
    size_t g_scaled;
    size_t t_scaled;
    size_t stack_scaled;

    const char* p_oom     = xen_bytes_order_of_magnitude(config->mem_size_permanent, &p_scaled);
    const char* g_oom     = xen_bytes_order_of_magnitude(config->mem_size_generation, &g_scaled);
    const char* t_oom     = xen_bytes_order_of_magnitude(config->mem_size_temporary, &t_scaled);
    const char* stack_oom = xen_bytes_order_of_magnitude(config->stack_size, &stack_scaled);

    printf("=== VM Configuration ===\n");
    printf("Memory (Perm) : %lu %s\n", p_scaled, p_oom);
    printf("Memory (Gen)  : %lu %s\n", g_scaled, g_oom);
    printf("Memory (Temp) : %lu %s\n", t_scaled, t_oom);
    printf("Stack Size    : %lu %s\n", stack_scaled, stack_oom);
}

static int execute_file(const char* filename, bool emit_bytecode, const char* bytecode_filename) {
    char* source = xen_read_file(filename);
    if (!source) {
        return XEN_FAIL;
    }

    xen_exec_result exec_result = xen_vm_exec(source, emit_bytecode, bytecode_filename);
    free(source);

    if (exec_result != EXEC_OK) {
        if (exec_result == EXEC_COMPILE_ERROR) {
            xen_panic(XEN_ERR_EXEC_COMPILE, "failed to compile input");
        }
    }

    return XEN_OK;
}

static int execute_bytecode(const char* filename) {
    goto not_implemented;

    xen_chunk chunk = xen_read_bytecode(filename);

not_implemented:
    printf("Bytecode execution is not currently supported\n");
    return XEN_OK;
}

int main(int argc, char* argv[]) {
    xen_vm_config config;
    config.mem_size_permanent  = XEN_MB(64);
    config.mem_size_generation = XEN_MB(64);
    config.mem_size_temporary  = XEN_MB(4);
    config.stack_size          = XEN_KB(1);

    xen_vm_init(config);

    if (argc < 2) {
        repl();
        return XEN_OK;
    }

    char* arg1 = argv[1];

    if (strcmp(arg1, "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return XEN_OK;
    }

    if (strcmp(arg1, "--version") == 0 || strcmp(arg1, "-v") == 0) {
        print_version();
        return XEN_OK;
    }

    if (strcmp(arg1, "--vm-config") == 0) {
        print_vm_config(&config);
        return XEN_OK;
    }

    bool emit_bytecode      = XEN_FALSE;
    char* bytecode_filename = NULL;
    if (argc > 2) {
        if (strcmp(argv[2], "--emit-bytecode") == 0) {
            if (argc > 3) {
                emit_bytecode     = XEN_TRUE;
                bytecode_filename = argv[3];
            } else {
                xen_panic(XEN_ERR_INVALID_ARGS, "missing bytecode filename argument");
            }
        }
    }

    // check supplied file extension so we know whether this is source or bytecode
    const char* ext = strrchr(arg1, '.');
    if (!ext) {
        xen_panic(XEN_ERR_INVALID_ARGS, "supplied filename is invalid (no extension)");
    }

    if (strcmp(ext, ".xen") == 0) {
        execute_file(arg1, emit_bytecode, bytecode_filename);
    } else if (strcmp(ext, ".xnb") == 0) {
        execute_bytecode(arg1);
    }

    xen_vm_shutdown();

    return XEN_OK;
}
