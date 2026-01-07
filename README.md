![](docs/Banner.png)

<p align="center">
    <a href="https://jakerieger.github.io/Xen/documentation.html">Documentation</a> | <a href="#getting-started">Getting Started</a> | <a href="#examples">Examples</a>
</p>

---

Xen is a loosely-typed, imperative scripting language written in C. Its syntax closesly resembles C-like languages and the bytecode compiler and VM is a mere 70Kb in total. Its primary purpose is to serve as a learning project for myself and a fun side project to work on in my free time. **It is not designed with the intent of being a serious, production-ready language.**

```js
include io;

class Car {
    year;
    make;
    model;

    init(year, make, model) {
        this.year = year;
        this.make = make;
        this.model = model;
    }

    fn start_engine() {
        if (this.year > 2000) { io.println("brrrrrrrrr"); }
        else { io.println("chuggachuggachugga"); }
    }

    fn show_year() => this.year
};

const var maserati = new Car(2019, "Maserati", "Levante");
io.println(maserati); // <Car : instance>

io.println(maserati.model); // Levante
maserati.start_engine(); // brrrrrrrrr
io.println(maserati.show_year()); // 2019

```

## Getting Started

If you want to try the current version of Xen for yourself, you can download [precompiled binaries](https://github.com/jakerieger/Xen/releases/latest) or download and compile the source code yourself. You can also checkout [Xen's "Get Started" guide](https://jakerieger.github.io/Xen/get-started.html) for a quick guide on getting up and running with programming in Xen.

## Examples

Example code can be found in the [examples](examples) directory. Until documentation is completed, the [xbuiltin.c](src/xbuiltin.c) file contains
all of the definitions for built in functions (like `os.println`);

## License

**Xen** is licensed under the [ISC license](LICENSE).
