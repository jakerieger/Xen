#include "xbuiltin_net.h"
#include "xbuiltin_common.h"
#include "../xutils.h"

//==============================================================//
//                          Globals                             //
//==============================================================//

static xen_obj_class* g_tcp_stream_class = NULL;

//==============================================================//
//                  Platform Initialization                     //
//==============================================================//

#ifdef PLATFORM_WINDOWS
static bool winsock_initialized = XEN_FALSE;

static bool init_winsock() {
    if (winsock_initialized)
        return XEN_TRUE;

    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        xen_runtime_error("[Net] WSAStartup failed with error: %d", result);
        return XEN_FALSE;
    }
    winsock_initialized = XEN_TRUE;
    return XEN_TRUE;
}

static void cleanup_winsock() {
    if (winsock_initialized) {
        WSACleanup();
        winsock_initialized = XEN_FALSE;
    }
}
#endif

//==============================================================//
//                  Platform-Agnostic Helpers                   //
//==============================================================//

static const char* get_socket_error() {
#ifdef PLATFORM_WINDOWS
    static char error_buffer[256];
    int error_code = WSAGetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   error_code,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   error_buffer,
                   sizeof(error_buffer),
                   NULL);
    // Remove trailing newline
    size_t len = strlen(error_buffer);
    if (len > 0 && error_buffer[len - 1] == '\n') {
        error_buffer[len - 1] = '\0';
        if (len > 1 && error_buffer[len - 2] == '\r') {
            error_buffer[len - 2] = '\0';
        }
    }
    return error_buffer;
#else
    return strerror(errno);
#endif
}

static ssize_t socket_read(socket_t fd, void* buffer, size_t size) {
#ifdef PLATFORM_WINDOWS
    return recv(fd, (char*)buffer, (int)size, 0);
#else
    return read(fd, buffer, size);
#endif
}

static ssize_t socket_write(socket_t fd, const void* buffer, size_t size) {
#ifdef PLATFORM_WINDOWS
    return send(fd, (const char*)buffer, (int)size, 0);
#else
    return write(fd, buffer, size);
#endif
}

//==============================================================//
//                      TcpStream Class                         //
//==============================================================//

// Native initializer: init(fd, remote_addr, remote_port)
static xen_value tcp_stream_init(i32 argc, xen_value* args) {
    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);

    if (argc < 2 || !VAL_IS_NUMBER(args[1])) {
        xen_runtime_error("TcpStream requires a socket file descriptor");
        return NULL_VAL;
    }

    socket_t fd = (socket_t)VAL_AS_NUMBER(args[1]);

    // Set properties (fd=0, remote_addr=1, remote_port=2)
    self->fields[0] = NUMBER_VAL((double)fd);
    self->fields[1] = argc > 2 ? args[2] : NULL_VAL;       // remote_addr
    self->fields[2] = argc > 3 ? args[3] : NUMBER_VAL(0);  // remote_port

    return OBJ_VAL(self);
}

// Method: read(max_bytes = 4096) -> returns string or null on error/EOF
static xen_value tcp_stream_read(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NULL_VAL;
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t fd            = (socket_t)VAL_AS_NUMBER(self->fields[0]);

    if (fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpStream] Socket is closed");
        return NULL_VAL;
    }

    i32 max_bytes = 4096;
    if (argc > 1 && VAL_IS_NUMBER(args[1])) {
        max_bytes = (i32)VAL_AS_NUMBER(args[1]);
        if (max_bytes < 1)
            max_bytes = 1;
        if (max_bytes > 65536)
            max_bytes = 65536;  // Cap at 64KB
    }

    char* buffer = (char*)malloc(max_bytes);
    if (!buffer) {
        xen_runtime_error("[TcpStream] Failed to allocate buffer");
        return NULL_VAL;
    }

    ssize_t bytes_read = socket_read(fd, buffer, max_bytes);

    if (bytes_read < 0) {
        free(buffer);
        xen_runtime_error("[TcpStream] read() failed: %s", get_socket_error());
        return NULL_VAL;
    }

    if (bytes_read == 0) {
        // EOF - connection closed by peer
        free(buffer);
        return NULL_VAL;
    }

    xen_obj_str* result = xen_obj_str_copy(buffer, (i32)bytes_read);
    free(buffer);

    return OBJ_VAL(result);
}

// Method: write(data) -> returns number of bytes written or -1 on error
static xen_value tcp_stream_write(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_INSTANCE(args[0])) {
        return NUMBER_VAL(-1);
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t fd            = (socket_t)VAL_AS_NUMBER(self->fields[0]);

    if (fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpStream] Socket is closed");
        return NUMBER_VAL(-1);
    }

    if (!OBJ_IS_STRING(args[1])) {
        xen_runtime_error("[TcpStream] write() requires a string argument");
        return NUMBER_VAL(-1);
    }

    xen_obj_str* data = OBJ_AS_STRING(args[1]);
    i32 decoded_size;
    const char* data_decoded = decode_string_literal(data->str, data->length, &decoded_size);
    ssize_t bytes_written    = socket_write(fd, data_decoded, decoded_size);

    if (bytes_written < 0) {
        xen_runtime_error("[TcpStream] write() failed: %s", get_socket_error());
        return NUMBER_VAL(-1);
    }

    return NUMBER_VAL((double)bytes_written);
}

// Method: send(data) -> alias for write(), returns number of bytes sent
static xen_value tcp_stream_send(i32 argc, xen_value* args) {
    if (argc < 2 || !OBJ_IS_INSTANCE(args[0])) {
        return NUMBER_VAL(-1);
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t fd            = (socket_t)VAL_AS_NUMBER(self->fields[0]);

    if (fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpStream] Socket is closed");
        return NUMBER_VAL(-1);
    }

    if (!OBJ_IS_STRING(args[1])) {
        xen_runtime_error("[TcpStream] send() requires a string argument");
        return NUMBER_VAL(-1);
    }

    xen_obj_str* data = OBJ_AS_STRING(args[1]);
    i32 decoded_size;
    const char* data_decoded = decode_string_literal(data->str, data->length, &decoded_size);
#ifdef PLATFORM_WINDOWS
    ssize_t bytes_sent = send(fd, data_decoded, decoded_size, 0);
#else
    ssize_t bytes_sent = send(fd, data_decoded, decoded_size, 0);
#endif

    if (bytes_sent < 0) {
        xen_runtime_error("[TcpStream] send() failed: %s", get_socket_error());
        return NUMBER_VAL(-1);
    }

    return NUMBER_VAL((double)bytes_sent);
}

// Method: recv(max_bytes = 4096) -> alias for read()
static xen_value tcp_stream_recv(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NULL_VAL;
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t fd            = (socket_t)VAL_AS_NUMBER(self->fields[0]);

    if (fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpStream] Socket is closed");
        return NULL_VAL;
    }

    i32 max_bytes = 4096;
    if (argc > 1 && VAL_IS_NUMBER(args[1])) {
        max_bytes = (i32)VAL_AS_NUMBER(args[1]);
        if (max_bytes < 1)
            max_bytes = 1;
        if (max_bytes > 65536)
            max_bytes = 65536;
    }

    char* buffer = (char*)malloc(max_bytes);
    if (!buffer) {
        xen_runtime_error("[TcpStream] Failed to allocate buffer");
        return NULL_VAL;
    }

#ifdef PLATFORM_WINDOWS
    ssize_t bytes_recv = recv(fd, buffer, max_bytes, 0);
#else
    ssize_t bytes_recv = recv(fd, buffer, max_bytes, 0);
#endif

    if (bytes_recv < 0) {
        free(buffer);
        xen_runtime_error("[TcpStream] recv() failed: %s", get_socket_error());
        return NULL_VAL;
    }

    if (bytes_recv == 0) {
        free(buffer);
        return NULL_VAL;
    }

    xen_obj_str* result = xen_obj_str_copy(buffer, (i32)bytes_recv);
    free(buffer);

    return OBJ_VAL(result);
}

// Method: close()
static xen_value tcp_stream_close(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NULL_VAL;
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t fd            = (socket_t)VAL_AS_NUMBER(self->fields[0]);

    if (fd != INVALID_SOCKET_FD) {
        if (CLOSE_SOCKET(fd) < 0) {
            xen_runtime_error("[TcpStream] close() failed: %s", get_socket_error());
        }
        self->fields[0] = NUMBER_VAL((double)INVALID_SOCKET_FD);
    }

    return NULL_VAL;
}

// Method: shutdown(how = 2) -> 0=read, 1=write, 2=both
static xen_value tcp_stream_shutdown(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return BOOL_VAL(XEN_FALSE);
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t fd            = (socket_t)VAL_AS_NUMBER(self->fields[0]);

    if (fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpStream] Socket is closed");
        return BOOL_VAL(XEN_FALSE);
    }

    int how = SHUTDOWN_BOTH;  // Default: shutdown both read and write
    if (argc > 1 && VAL_IS_NUMBER(args[1])) {
        i32 how_arg = (i32)VAL_AS_NUMBER(args[1]);
        if (how_arg == 0)
            how = SHUTDOWN_READ;
        else if (how_arg == 1)
            how = SHUTDOWN_WRITE;
        else
            how = SHUTDOWN_BOTH;
    }

    if (shutdown(fd, how) < 0) {
        xen_runtime_error("[TcpStream] shutdown() failed: %s", get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    return BOOL_VAL(XEN_TRUE);
}

static xen_obj_class* create_tcp_stream_class() {
    xen_obj_str* name    = xen_obj_str_copy("TcpStream", 9);
    xen_obj_class* class = xen_obj_class_new(name);

    // Properties: _fd, remote_addr, remote_port
    xen_obj_str* fd_name = xen_obj_str_copy("_fd", 3);
    xen_obj_class_add_property(class, fd_name, NUMBER_VAL((double)INVALID_SOCKET_FD), XEN_TRUE);

    xen_obj_str* addr_name = xen_obj_str_copy("remote_addr", 11);
    xen_obj_class_add_property(class, addr_name, NULL_VAL, XEN_FALSE);

    xen_obj_str* port_name = xen_obj_str_copy("remote_port", 11);
    xen_obj_class_add_property(class, port_name, NUMBER_VAL(0), XEN_FALSE);

    xen_obj_class_set_native_init(class, tcp_stream_init);

    xen_obj_class_add_native_method(class, "read", tcp_stream_read, XEN_FALSE);
    xen_obj_class_add_native_method(class, "write", tcp_stream_write, XEN_FALSE);
    xen_obj_class_add_native_method(class, "send", tcp_stream_send, XEN_FALSE);
    xen_obj_class_add_native_method(class, "recv", tcp_stream_recv, XEN_FALSE);
    xen_obj_class_add_native_method(class, "close", tcp_stream_close, XEN_FALSE);
    xen_obj_class_add_native_method(class, "shutdown", tcp_stream_shutdown, XEN_FALSE);

    return class;
}

//==============================================================//
//                     TcpListener Class                        //
//==============================================================//

// Native initializer: init(port)
static xen_value tcp_listener_init(i32 argc, xen_value* args) {
    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);

#ifdef PLATFORM_WINDOWS
    if (!init_winsock()) {
        return NULL_VAL;
    }
#endif

    if (argc < 2) {
        xen_runtime_error("TcpListener requires a port number");
        return NULL_VAL;
    }

    if (!VAL_IS_NUMBER(args[1])) {
        xen_runtime_error("port must be a number");
        return NULL_VAL;
    }

    i32 port = (i32)VAL_AS_NUMBER(args[1]);

    // Set properties (port is index 0, _socket is index 1)
    self->fields[0] = NUMBER_VAL(port);
    self->fields[1] = NUMBER_VAL((double)INVALID_SOCKET_FD);

    return OBJ_VAL(self);
}

// Method: bind()
static xen_value tcp_listener_bind(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return BOOL_VAL(XEN_FALSE);
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    i32 port               = (i32)VAL_AS_NUMBER(self->fields[0]);
    socket_t socket_fd     = (socket_t)VAL_AS_NUMBER(self->fields[1]);

    if (socket_fd != INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpListener] Socket is already bound");
        return BOOL_VAL(XEN_FALSE);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpListener] Failed to create socket: %s", get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    // Set SO_REUSEADDR to allow quick rebinding
    int opt = 1;
#ifdef PLATFORM_WINDOWS
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#else
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        CLOSE_SOCKET(socket_fd);
        xen_runtime_error("[TcpListener] setsockopt() failed: %s", get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons((u16)port);

    if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        CLOSE_SOCKET(socket_fd);
        xen_runtime_error("[TcpListener] bind() failed on port %d: %s", port, get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    printf("[TcpListener] Binding to port %d\n", port);
    self->fields[1] = NUMBER_VAL((double)socket_fd);

    return BOOL_VAL(XEN_TRUE);
}

// Method: listen(backlog = 5)
static xen_value tcp_listener_listen(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return BOOL_VAL(XEN_FALSE);
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t socket_fd     = (socket_t)VAL_AS_NUMBER(self->fields[1]);

    if (socket_fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpListener] Socket not bound - call bind() first");
        return BOOL_VAL(XEN_FALSE);
    }

    i32 backlog = 5;
    if (argc > 1 && VAL_IS_NUMBER(args[1])) {
        backlog = (i32)VAL_AS_NUMBER(args[1]);
        if (backlog < 1)
            backlog = 1;
    }

    if (listen(socket_fd, backlog) < 0) {
        xen_runtime_error("[TcpListener] listen() failed: %s", get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    printf("[TcpListener] Listening with backlog %d\n", backlog);
    return BOOL_VAL(XEN_TRUE);
}

static xen_value tcp_listener_bind_and_listen(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NULL_VAL;
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    i32 port               = (i32)VAL_AS_NUMBER(self->fields[0]);
    socket_t socket_fd     = (socket_t)VAL_AS_NUMBER(self->fields[1]);

    if (socket_fd != INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpListener] Socket is already bound");
        return BOOL_VAL(XEN_FALSE);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpListener] Failed to create socket: %s", get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    // Set SO_REUSEADDR to allow quick rebinding
    int opt = 1;
#ifdef PLATFORM_WINDOWS
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#else
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        CLOSE_SOCKET(socket_fd);
        xen_runtime_error("[TcpListener] setsockopt() failed: %s", get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons((u16)port);

    if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        CLOSE_SOCKET(socket_fd);
        xen_runtime_error("[TcpListener] bind() failed on port %d: %s", port, get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    printf("[TcpListener] Binding to port %d\n", port);
    self->fields[1] = NUMBER_VAL((double)socket_fd);

    if (socket_fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpListener] Socket not bound - call bind() first");
        return BOOL_VAL(XEN_FALSE);
    }

    i32 backlog = 5;
    if (argc > 1 && VAL_IS_NUMBER(args[1])) {
        backlog = (i32)VAL_AS_NUMBER(args[1]);
        if (backlog < 1)
            backlog = 1;
    }

    if (listen(socket_fd, backlog) < 0) {
        xen_runtime_error("[TcpListener] listen() failed: %s", get_socket_error());
        return BOOL_VAL(XEN_FALSE);
    }

    printf("[TcpListener] Listening with backlog %d\n", backlog);
    return BOOL_VAL(XEN_TRUE);
}

// Method: accept() -> returns TcpStream instance
static xen_value tcp_listener_accept(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NULL_VAL;
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t socket_fd     = (socket_t)VAL_AS_NUMBER(self->fields[1]);

    if (socket_fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpListener] Socket not bound - call bind() and listen() first");
        return NULL_VAL;
    }

    struct sockaddr_in client_addr;
#ifdef PLATFORM_WINDOWS
    int addr_len = sizeof(client_addr);
#else
    socklen_t addr_len = sizeof(client_addr);
#endif

    socket_t client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &addr_len);

    if (client_fd == INVALID_SOCKET_FD) {
        xen_runtime_error("[TcpListener] accept() failed: %s", get_socket_error());
        return NULL_VAL;
    }

    // Get client address as string
    char addr_str[INET_ADDRSTRLEN];
#ifdef PLATFORM_WINDOWS
    inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, INET_ADDRSTRLEN);
#else
    inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, INET_ADDRSTRLEN);
#endif
    xen_obj_str* remote_addr = xen_obj_str_copy(addr_str, strlen(addr_str));
    u16 remote_port          = ntohs(client_addr.sin_port);

    // Create TcpStream instance
    xen_obj_class* tcp_stream_class = g_tcp_stream_class;
    xen_obj_instance* stream        = xen_obj_instance_new(tcp_stream_class);

    // Initialize the stream with fd, remote_addr, remote_port
    stream->fields[0] = NUMBER_VAL((double)client_fd);
    stream->fields[1] = OBJ_VAL(remote_addr);
    stream->fields[2] = NUMBER_VAL(remote_port);

    return OBJ_VAL(stream);
}

// Method: close()
static xen_value tcp_listener_close(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NULL_VAL;
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    socket_t fd            = (socket_t)VAL_AS_NUMBER(self->fields[1]);

    if (fd != INVALID_SOCKET_FD) {
        if (CLOSE_SOCKET(fd) < 0) {
            xen_runtime_error("[TcpListener] close() failed: %s", get_socket_error());
        }
        self->fields[1] = NUMBER_VAL((double)INVALID_SOCKET_FD);
    }

    return NULL_VAL;
}

static xen_obj_class* create_tcp_listener_class() {
    xen_obj_str* name    = xen_obj_str_copy("TcpListener", 11);
    xen_obj_class* class = xen_obj_class_new(name);

    xen_obj_str* port_name = xen_obj_str_copy("port", 4);
    xen_obj_class_add_property(class, port_name, NUMBER_VAL(0), XEN_FALSE);

    xen_obj_str* socket_name = xen_obj_str_copy("_socket", 7);
    xen_obj_class_add_property(class, socket_name, NUMBER_VAL((double)INVALID_SOCKET_FD), XEN_TRUE);

    xen_obj_class_set_native_init(class, tcp_listener_init);

    xen_obj_class_add_native_method(class, "bind", tcp_listener_bind, XEN_FALSE);
    xen_obj_class_add_native_method(class, "listen", tcp_listener_listen, XEN_FALSE);
    xen_obj_class_add_native_method(class, "bind_and_listen", tcp_listener_bind_and_listen, XEN_FALSE);
    xen_obj_class_add_native_method(class, "accept", tcp_listener_accept, XEN_FALSE);
    xen_obj_class_add_native_method(class, "close", tcp_listener_close, XEN_FALSE);

    return class;
}

//==============================================================//
//                    Register Namespace                        //
//==============================================================//

xen_obj_namespace* xen_builtin_net() {
    xen_obj_namespace* net = xen_obj_namespace_new("net");

    g_tcp_stream_class = create_tcp_stream_class();

    xen_obj_namespace_set(net, "TcpListener", OBJ_VAL(create_tcp_listener_class()));
    xen_obj_namespace_set(net, "TcpStream", OBJ_VAL(create_tcp_stream_class()));

    return net;
}