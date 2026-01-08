#include "xbuiltin_net.h"
#include "xbuiltin_common.h"

#ifdef __linux
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#else
    #error                                                                                                             \
      "The net namespace is currently only available for Linux. Disable it with XEN_NET_DISABLE to compile Xen for Windows or macOS."
#endif

//==============================================================//
//                     TCPListener Class                        //
//==============================================================//

// Native initializer: init(port)
// args[0] = instance (this), args[1] = port
static xen_value tcp_listener_init(i32 argc, xen_value* args) {
    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);

    if (argc < 2) {
        xen_runtime_error("TCPListener requires a port number");
        return NULL_VAL;
    }

    if (!VAL_IS_NUMBER(args[1])) {
        xen_runtime_error("port must be a number");
        return NULL_VAL;
    }

    i32 port = (i32)VAL_AS_NUMBER(args[1]);

    // Set properties (assuming port is index 0, _socket is index 1)
    self->fields[0] = NUMBER_VAL(port);
    self->fields[1] = NUMBER_VAL(-1);  // socket fd, -1 = not bound

    return OBJ_VAL(self);  // Return self (standard init behavior)
}

// Method: bind()
static xen_value tcp_listener_bind(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return BOOL_VAL(XEN_FALSE);
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);
    i32 port               = (i32)VAL_AS_NUMBER(self->fields[0]);

    // TODO: Real socket binding
    printf("[TCPListener] Binding to port %d\n", port);
    self->fields[1] = NUMBER_VAL(42);  // Fake socket fd

    return BOOL_VAL(XEN_TRUE);
}

// Method: listen(backlog = 5)
static xen_value tcp_listener_listen(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return BOOL_VAL(XEN_FALSE);
    }

    i32 backlog = 5;
    if (argc > 1 && VAL_IS_NUMBER(args[1])) {
        backlog = (i32)VAL_AS_NUMBER(args[1]);
    }

    printf("[TCPListener] Listening with backlog %d\n", backlog);
    return BOOL_VAL(XEN_TRUE);
}

// Method: accept() -> would return TcpStream in real impl
static xen_value tcp_listener_accept(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NULL_VAL;
    }

    printf("[TCPListener] Waiting for connection...\n");
    // Real implementation would block and return a TcpStream instance
    return NULL_VAL;
}

// Method: close()
static xen_value tcp_listener_close(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NULL_VAL;
    }

    xen_obj_instance* self = OBJ_AS_INSTANCE(args[0]);

    i32 fd = (i32)VAL_AS_NUMBER(self->fields[1]);
    if (fd >= 0) {
        // TODO: Real close(fd)
        printf("[TCPListener] Closed socket %d\n", fd);
        self->fields[1] = NUMBER_VAL(-1);
    }

    return NULL_VAL;
}

// Property getter: port (if you want .port as a property-style access)
static xen_value tcp_listener_get_port(i32 argc, xen_value* args) {
    if (argc < 1 || !OBJ_IS_INSTANCE(args[0])) {
        return NUMBER_VAL(0);
    }
    return OBJ_AS_INSTANCE(args[0])->fields[0];
}

static xen_obj_class* create_tcp_listener_class() {
    xen_obj_str* name    = xen_obj_str_copy("TCPListener", 11);
    xen_obj_class* class = xen_obj_class_new(name);

    xen_obj_str* port_name = xen_obj_str_copy("port", 4);
    xen_obj_class_add_property(class, port_name, NUMBER_VAL(0), XEN_FALSE);

    xen_obj_str* socket_name = xen_obj_str_copy("_socket", 7);
    xen_obj_class_add_property(class, socket_name, NUMBER_VAL(-1), XEN_TRUE);

    xen_obj_class_set_native_init(class, tcp_listener_init);

    xen_obj_class_add_native_method(class, "bind", tcp_listener_bind, XEN_FALSE);
    xen_obj_class_add_native_method(class, "listen", tcp_listener_listen, XEN_FALSE);
    xen_obj_class_add_native_method(class, "accept", tcp_listener_accept, XEN_FALSE);
    xen_obj_class_add_native_method(class, "close", tcp_listener_close, XEN_FALSE);

    return class;
}

xen_obj_namespace* xen_builtin_net() {
    xen_obj_namespace* net = xen_obj_namespace_new("net");

    xen_obj_namespace_set(net, "TCPListener", OBJ_VAL(create_tcp_listener_class()));

    return net;
}
