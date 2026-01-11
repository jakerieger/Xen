/*
 * This is a proof of concept for producing native binaries from Xen code.
 *   - It is not intended to be used for production purposes -
 * The initial goal is to simply wrap pre-exisiting Xen code with the Xen VM
 * and have it run that code by default. This would effectively produce
 * a "native" binary, although its not exactly Xen -> Native machine code.
 *
 * [!] Only intended to work on Linux for now
 *
 * TODO: Move interpreter/VM to library that can be shared by xen/xenc
 */

#include "../xen/xcommon.h"
#include "../xen/xutils.h"
#include "../xen/xvm.h"
#include "xenb.h"

#ifdef __linux
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <errno.h>
    #include <unistd.h>
#else
    #error "Unsupported platform"
#endif

#define MAX_CODE_SIZE XEN_MB(1)
#define BYTES_PER_LINE 12

/*
 * The general approach is as follows:
 * 1. Accept the source to compile as an argument
 * 2. Compile source to bytecode
 * 2. Generate a simple C runner project and embed bytecode
 * 3. Compile this runner project to produce the final application binary
 */

static bool dir_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void generate_runner_project(const char* name, const char* source_path) {
    // Needed files:
    // - runner_dir
    //   - Makefile
    //   - runner.c
    //   - bytecode.h (compiled bytecode)
    //
    // Compile this generated project for the final executable

    char project_name[256] = {'\0'};
    snprintf(project_name, 256, "%s-build", name);

    // Create runner project directory
    if (!dir_exists(project_name)) {
        if (mkdir(project_name, 0755) == -1) {
            fprintf(stderr, "error: failed to create project directory\n");
            return;
        }
    } else {
        if (rmdir(project_name) == -1) {
            fprintf(stderr, "error: failed to remove existing project directory\n");
            return;
        }
        if (mkdir(project_name, 0755) == -1) {
            fprintf(stderr, "error: failed to create project directory\n");
            return;
        }
    }

    char makefile_path[256] = {'\0'};
    char runner_path[256]   = {'\0'};
    char bytecode_path[256] = {'\0'};
    snprintf(makefile_path, 256, "%s/Makefile", project_name);
    snprintf(runner_path, 256, "%s/runner.c", project_name);
    snprintf(bytecode_path, 256, "%s/bytecode.h", project_name);

    size_t bytecode_size;
    u8 bytecode[MAX_CODE_SIZE];
    xenb_compile(source_path, bytecode, &bytecode_size);

    // Convert bytecode into C header file
    FILE* output = fopen(bytecode_path, "w");
    if (!output) {
        perror("Failed to open output file");
        return;
    }

    // Write header
    fprintf(output, "#ifndef BYTECODE_H\n");
    fprintf(output, "#define BYTECODE_H\n\n");
    fprintf(output, "#include <stdint.h>\n\n");
    fprintf(output, "#define BYTECODE_SIZE %ld\n", bytecode_size);
    fprintf(output, "const uint8_t k_bytecode[BYTECODE_SIZE] = {\n");

    // Write bytecode array
    for (size_t i = 0; i < bytecode_size; i++) {
        if (i % BYTES_PER_LINE == 0) {
            fprintf(output, "    ");
        }

        fprintf(output, "0x%02X", bytecode[i]);

        if (i < (size_t)bytecode_size - 1) {
            fprintf(output, ",");
            if ((i + 1) % BYTES_PER_LINE == 0) {
                fprintf(output, "\n");
            } else {
                fprintf(output, " ");
            }
        }
    }

    fprintf(output, "\n};\n\n");
    fprintf(output, "#endif // BYTECODE_H\n");

    fclose(output);
}

int main(i32 argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "error: no input file provided\n");
        return 1;
    }

    xen_vm_config config;
    config.mem_size_permanent  = XEN_MB(64);
    config.mem_size_generation = XEN_MB(64);
    config.mem_size_temporary  = XEN_MB(4);
    config.stack_size          = XEN_KB(1);

    xen_vm_init(config);

    const char* source_path = argv[1];

    size_t bytecode_size;
    u8 bytecode[MAX_CODE_SIZE];
    xenb_compile(source_path, bytecode, &bytecode_size);

    xen_obj_func* decoded = xen_decode_bytecode(bytecode, bytecode_size);

    // printf("Compiling: %s\n", source_path);

    // // Generate C runner project
    // const char* project_name = "test";
    // generate_runner_project(project_name, source_path);

    xen_vm_shutdown();

    return 0;
}