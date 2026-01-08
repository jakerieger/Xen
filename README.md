![](docs/Banner.png)

<p align="center">
    <a href="https://jakerieger.github.io/Xen/documentation.html">Documentation</a> | <a href="#getting-started">Getting Started</a> | <a href="#examples">Examples</a>
</p>

---

Xen is a loosely-typed, imperative scripting language written in C. Its syntax closesly resembles C-like languages and the bytecode compiler and VM is a mere 70Kb in total. Its primary purpose is to serve as a learning project for myself and a fun side project to work on in my free time. **It is not designed with the intent of being a serious, production-ready language.**

```js
include io;
include net;

const var PORT = 8080;

fn run() {
    const var l = new net.TcpListener(PORT);
    l.bind_and_listen();

    for (;;) {
        var conn = l.accept();
        var msg = conn.read();
        if (msg != null) {
            io.println("=== Client (", conn.remote_addr, ") ===");
            io.println(msg);
            const var resp = "HTTP/1.1 200 OK\nContent-Length: 5\n\nHello";
            conn.send(resp);
        }
        conn.close();
    }

    l.close();
}

run();

```

## Getting Started

If you want to try Xen out, you can download a pre-compiled binaries from our [releases](https://github.com/jakerieger/Xen/releases/latest).

If you prefer to build from source the process is pretty straight-forward:

### 1. Clone the repository

```
$ git clone https://github.com/jakerieger/Xen
$ cd Xen
```

### 2. Run the configuration script

```
$ ./generate_build.sh
```

### 3. Build

```
$ ./build.sh <platform> # i.e. linux-debug
```

The final binary will be located in the `build` directory.

## Examples

Example code can be found in the [examples](examples) directory. Until documentation is completed, the [xbuiltin.c](src/xbuiltin.c) file contains
all of the definitions for built in functions (like `os.println`);

## License

**Xen** is licensed under the [ISC license](LICENSE).
