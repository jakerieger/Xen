#include "xbuiltin_os.h"
#include "xbuiltin_common.h"
#include "../xutils.h"

#include "../object/xobj_string.h"
#include "../object/xobj_array.h"
#include "../object/xobj_u8array.h"
#include "../object/xobj_namespace.h"
#include "../object/xobj_native_function.h"

#include <sys/stat.h>
#include <errno.h>
#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #include <windows.h>
    #define mkdir(path, mode) _mkdir(path)
    #define rmdir(path) _rmdir(path)
    #define unlink(path) _unlink(path)
#else
    #include <sys/types.h>
    #include <dirent.h>
    #include <unistd.h>
#endif

static xen_value os_readtxt(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("filename", 0, TYPEID_STRING);
    xen_value val = argv[0];

    if (val.type == VAL_OBJECT && OBJ_IS_STRING(val)) {
        FILE* fp = fopen(OBJ_AS_CSTRING(val), "r");
        if (!fp) {
            xen_runtime_error("failed to open file: %s", OBJ_AS_CSTRING(val));
            return NULL_VAL;
        }

        fseek(fp, 0, SEEK_END);
        i32 fp_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        i32 input_size = fp_size + 1;
        char* buffer   = (char*)malloc(input_size);

        size_t read_bytes  = fread(buffer, 1, input_size, fp);
        buffer[read_bytes] = '\0';
        fclose(fp);

        return OBJ_VAL(xen_obj_str_take(buffer, input_size));
    } else {
        xen_runtime_error("filename argument must be of type string (got '%s')", xen_value_type_to_str(val));
        return NULL_VAL;
    }
}

static xen_value os_readlines(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("filename", 0, TYPEID_STRING);
    xen_value val = argv[0];

    if (val.type == VAL_OBJECT && OBJ_IS_STRING(val)) {
        FILE* fp = fopen(OBJ_AS_CSTRING(val), "r");
        if (!fp) {
            xen_runtime_error("failed to open file: %s", OBJ_AS_CSTRING(val));
            return NULL_VAL;
        }

        xen_obj_array* arr = xen_obj_array_new();

        char* line;
        while ((line = xen_read_line(fp)) != NULL) {
            xen_obj_array_push(arr, OBJ_VAL(xen_obj_str_copy(line, strlen(line))));
            free(line);
        }

        fclose(fp);

        return OBJ_VAL(arr);
    } else {
        xen_runtime_error("filename argument must be of type string (got '%s')", xen_value_type_to_str(val));
        return NULL_VAL;
    }
}

static xen_value os_readbytes(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("filename", 0, TYPEID_STRING);
    xen_value val = argv[0];

    if (val.type == VAL_OBJECT && OBJ_IS_STRING(val)) {
        FILE* fp = fopen(OBJ_AS_CSTRING(val), "rb");
        if (!fp) {
            xen_runtime_error("failed to open file: %s", OBJ_AS_CSTRING(val));
            return NULL_VAL;
        }

        // Get file size
        if (fseek(fp, 0, SEEK_END) != 0) {
            fclose(fp);
            return NULL_VAL;
        }

        long size = ftell(fp);
        if (size < 0) {
            fclose(fp);
            return NULL_VAL;
        }
        rewind(fp);

        u8* buffer = malloc((size_t)size);
        if (!buffer) {
            fclose(fp);
            return NULL_VAL;
        }

        size_t read = fread(buffer, 1, (size_t)size, fp);
        fclose(fp);

        if (read != (size_t)size) {
            free(buffer);
            return NULL_VAL;
        }

        xen_obj_u8array* arr = xen_obj_u8array_new_with_capacity(size);
        arr->count           = size;
        memcpy(arr->values, buffer, size);
        free(buffer);

        return OBJ_VAL(arr);
    } else {
        xen_runtime_error("filename argument must be of type string (got '%s')", xen_value_type_to_str(val));
        return NULL_VAL;
    }
}

static xen_value os_exit(i32 argc, array(xen_value) argv) {
    i32 exit_code = 0;
    if (argc > 0 && VAL_IS_NUMBER(argv[0]))
        exit_code = (i32)VAL_AS_NUMBER(argv[0]);

    printf("Xen was terminated with exit code %d\n", exit_code);
    exit(exit_code);

    return NULL_VAL;
}

// Signature:
// os.exec( cmd, argv[] ) -> exit_code
// TODO: This uses system and is quite unsafe, need to refactor to use fork instead
static xen_value os_exec(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("cmd", 0, TYPEID_STRING);
    xen_obj_str* cmd = OBJ_AS_STRING(argv[0]);

    i32 args_count = 0;
    if (argc == 2 && OBJ_IS_ARRAY(argv[1])) {
        args_count = OBJ_AS_ARRAY(argv[1])->array.count;
    }

    char cmd_buffer[XEN_STRBUF_SIZE] = {'\0'};
    size_t offset                    = 0;
    memcpy(cmd_buffer, cmd->str, cmd->length);
    cmd_buffer[cmd->length + 1] = ' ';  // add a space after the command
    offset += cmd->length + 1;

    if (args_count > 0) {
        xen_obj_array* args_arr = OBJ_AS_ARRAY(argv[1]);
        for (i32 i = 0; i < args_count; i++) {
            xen_value arg = args_arr->array.values[i];
            if (OBJ_IS_STRING(arg)) {
                const char* arg_str = OBJ_AS_CSTRING(arg);
                memcpy(cmd_buffer + offset, arg_str, strlen(arg_str));
                cmd_buffer[strlen(arg_str) + 1] = ' ';
                offset += strlen(arg_str) + 1;
            }
        }
    }

    i32 exit_code = system(cmd_buffer);
    return NUMBER_VAL(exit_code);
}

static i32 remove_directory(const char* path, bool recursive) {
    if (!recursive) {
        return rmdir(path);
    }

    // Recursive removal
#ifdef _WIN32
    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    char search_path[MAX_PATH];
    char file_path[MAX_PATH];

    snprintf(search_path, MAX_PATH, "%s\\*", path);
    hFind = FindFirstFile(search_path, &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        return rmdir(path);
    }

    do {
        if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
            snprintf(file_path, MAX_PATH, "%s\\%s", path, find_data.cFileName);

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                remove_directory(file_path, XEN_TRUE);  // Recursively remove subdirectories
            } else {
                unlink(file_path);
            }
        }
    } while (FindNextFile(hFind, &find_data));

    FindClose(hFind);
    return rmdir(path);
#else
    DIR* dir;
    struct dirent* entry;
    char file_path[1024];
    struct stat statbuf;

    dir = opendir(path);
    if (!dir) {
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

        if (stat(file_path, &statbuf) == -1) {
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            remove_directory(file_path, XEN_TRUE);  // Recursively remove subdirectories
        } else {
            unlink(file_path);
        }
    }

    closedir(dir);
    return rmdir(path);
#endif
}

static bool path_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static bool is_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return XEN_FALSE;
    }
    return S_ISDIR(st.st_mode);
}

static bool is_file(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return XEN_FALSE;
    }
    return S_ISREG(st.st_mode);
}

// Signature:
// * dir: string
// * should_overwrite: bool
// fn mkdir(dir, should_overwrite) -> success: true, fail: false
static xen_value os_mkdir(int argc, array(xen_value) argv) {
    REQUIRE_ARG("dir", 0, TYPEID_STRING);
    const char* dir = OBJ_AS_CSTRING(argv[0]);

    bool should_overwrite = XEN_FALSE;
    if (argc > 1 && xen_typeid_get(argv[1]) == TYPEID_BOOL) {
        should_overwrite = VAL_AS_BOOL(argv[1]);
    }

    struct stat st;
    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            if (should_overwrite) {
                if (remove_directory(dir, XEN_TRUE) != 0) {
                    xen_runtime_error("failed to remove directory '%s'", dir);
                    return BOOL_VAL(XEN_FALSE);
                }
            } else {
                return BOOL_VAL(XEN_FALSE);  // directory exists but no overwrite
            }
        }
    }

#ifdef _WIN32
    int result = mkdir(dir, 0755);
#else
    int result = mkdir(dir, 0755);
#endif

    if (result == 0) {
        return BOOL_VAL(XEN_TRUE);
    } else {
        return BOOL_VAL(XEN_FALSE);
    }
}

// Signature:
// * dir: string
// * recursive: bool
// fn rmdir(dir, recursive) -> success: true, fail: false
static xen_value os_rmdir(int argc, array(xen_value) argv) {
    REQUIRE_ARG("dir", 0, TYPEID_STRING);
    const char* dir = OBJ_AS_CSTRING(argv[0]);

    bool recursive = XEN_FALSE;
    if (argc > 1 && xen_typeid_get(argv[1]) == TYPEID_BOOL) {
        recursive = VAL_AS_BOOL(argv[1]);
    }

    i32 result = remove_directory(dir, recursive);
    if (result != 0) {
        xen_runtime_error("failed to remove directory '%s' (not empty)", dir);
        return BOOL_VAL(XEN_FALSE);
    }

    return BOOL_VAL(XEN_TRUE);
}

// Signature:
// * filename: string
// fn rm(filename) -> success: true, fail: false
static xen_value os_rm(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("filename", 0, TYPEID_STRING);
    const char* filename = OBJ_AS_CSTRING(argv[0]);

    if (!path_exists(filename)) {
        xen_runtime_error("file does not exist: %s", filename);
        return BOOL_VAL(XEN_FALSE);
    }

    if (!is_file(filename)) {
        xen_runtime_error("path is not a file: %s", filename);
        return BOOL_VAL(XEN_FALSE);
    }

    if (unlink(filename) == 0) {
        return BOOL_VAL(XEN_TRUE);
    } else {
        xen_runtime_error("failed to delete file: %s", filename);
        return BOOL_VAL(XEN_FALSE);
    }
}

// Signature:
// * path: string (file or directory)
// fn exists(dir, recursive) -> bool
static xen_value os_exists(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("path", 0, TYPEID_STRING);
    return BOOL_VAL(path_exists(OBJ_AS_CSTRING(argv[0])));
}

// Signature:
// * duration: number (in seconds) :
// fn sleep(duration) -> error: null | number (duration slept)
static xen_value os_sleep(i32 argc, array(xen_value) argv) {
    REQUIRE_ARG("duration", 0, TYPEID_NUMBER);
    u32 duration = (u32)VAL_AS_NUMBER(argv[0]);
    sleep(duration);
    return NUMBER_VAL(duration);
}

xen_obj_namespace* xen_builtin_os() {
    xen_obj_namespace* os = xen_obj_namespace_new("os");
    xen_obj_namespace_set(os, "readtxt", OBJ_VAL(xen_obj_native_func_new(os_readtxt, "readtxt")));
    xen_obj_namespace_set(os, "readlines", OBJ_VAL(xen_obj_native_func_new(os_readlines, "readlines")));
    xen_obj_namespace_set(os, "readbytes", OBJ_VAL(xen_obj_native_func_new(os_readbytes, "readbytes")));
    xen_obj_namespace_set(os, "exit", OBJ_VAL(xen_obj_native_func_new(os_exit, "exit")));
    xen_obj_namespace_set(os, "exec", OBJ_VAL(xen_obj_native_func_new(os_exec, "exec")));
    xen_obj_namespace_set(os, "mkdir", OBJ_VAL(xen_obj_native_func_new(os_mkdir, "mkdir")));
    xen_obj_namespace_set(os, "rmdir", OBJ_VAL(xen_obj_native_func_new(os_rmdir, "rmdir")));
    xen_obj_namespace_set(os, "rm", OBJ_VAL(xen_obj_native_func_new(os_rm, "rm")));
    xen_obj_namespace_set(os, "exists", OBJ_VAL(xen_obj_native_func_new(os_exists, "exists")));
    xen_obj_namespace_set(os, "sleep", OBJ_VAL(xen_obj_native_func_new(os_sleep, "sleep")));
    return os;
}
