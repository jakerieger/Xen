#include "xcommon.h"
#include "xscanner.h"
#include "xversion.h"
#include "xvm.h"
#include "xutils.h"

#define MAX_LINE_SIZE 1024

static void repl() {
    printf(COLOR_BOLD COLOR_BRIGHT_BLUE "Xen" COLOR_RESET " %s " COLOR_DIM
                                        "- Copyright (C) 2025 Jake Rieger\n" COLOR_RESET,
           VERSION_STRING_FULL);
    printf(COLOR_ITALIC "Type " COLOR_RESET COLOR_BOLD ".exit" COLOR_RESET COLOR_ITALIC
                        " to quit the REPL.\n\n" COLOR_RESET);

    char line[MAX_LINE_SIZE];

    for (;;) {
        printf("=> ");
        if (!fgets(line, MAX_LINE_SIZE, stdin)) {
            printf("\n");
            break;
        }

        if (strcmp(line, ".exit\n") == 0) {
            break;
        }

        xen_exec_result exec_result = xen_vm_exec(line, XEN_FALSE, NULL);
        // We won't panic in the REPL
    }
}

static void print_help() {
    printf(COLOR_BOLD COLOR_BRIGHT_BLUE "Xen" COLOR_RESET COLOR_DIM " - Copyright (C) 2025 Jake Rieger\n" COLOR_RESET);
    printf(COLOR_DIM "version " COLOR_RESET COLOR_BOLD VERSION_STRING_FULL COLOR_RESET "\n\n");
    printf("USAGE\n");
    printf("  xl  " COLOR_DIM "-or-" COLOR_RESET "  xl <filename>\n\n");
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
