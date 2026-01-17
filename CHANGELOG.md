# Changelog

## Table of Contents
- [v0.5.5](#v055-january-17-2026)
- [v0.5.4](#v054-january-11-2026)
- [v0.5.3](#v053-january-8-2026)
- [v0.5.2](#v052-january-7-2026)
- [v0.5.1](#v051-january-7-2026)
- [v0.5.0](#v050-january-5-2026)
- [v0.4.1](#v041-january-4-2026)
- [v0.4.0](#v040-january-3-2026)
- [v0.3.0](#v030-january-2-2026)
- [v0.2.0](#v020-january-2-2026)
- [v0.1.0](#v010-january-1-2026)

## v0.5.5 (January 17, 2026)

Xen v0.5.5 brings some new additions to the language.

### Added
- `env` namespace with `.args` (array) and `.argc` (number) members.
- `as` keyword for casting primitive types
- `is` keyword for type-checking values at run time
- `Error` type

## v0.5.4 (January 11, 2026)

Xen v0.5.4 brings lots of bug fixes and reliability tweaks, as well as some new additions to the language.

### Added
- `os.sleep()` function for sleeping the current process
- `UInt8Array` type for storing byte sequences
- `dictionary` namespace and constructor with `Dictionary()`
- `.len()` to dictionaries

### Changed
- Renamed `dict` namespace to `dictionary`
- Made all constructors capitalized: `number("10")` -> `Number("10")`

### Fixed
- String literals get properly decoded now so `io.println("\n")` will print an actual newline instead of a literal "\n" (as well as any other functions that accept string input)
- Memory leaks from certain objects not getting properly destroyed
- Bug in token scanner when matching tokens

## v0.5.3 (January 8, 2026)

### Added
- `net` namespace with `TcpListener` and `TcpStream` classes:
```js
include net;
var l = new net.TcpListener(8080);
```

### Changed
- Refactored builtin code and moved builtin namespace implementations to dedicated source files in [src/builtin](src/builtin)

### Fixed
- Build system bugs

## v0.5.2 (January 7, 2026)

Xen v0.5.2 brings support to both x64 and ARM64 macOS systems, as well as some small bug fixes and improvements.

## v0.5.1 (January 7, 2026)

### Added
- New methods to the `os` namespace: `mkdir`, `rmdir`, `rm`, and `exists`.
- Arrow functions are valid inside of class bodies

### Fixed
- Class `init` bug where classes that had initializers would return `null` on construction.
- General improvements to code

## v0.5.0 (January 5, 2026)

### Added
- Classes ([#2](https://github.com/jakerieger/Xen/issues/2))
- Dictionaries ([#3](https://github.com/jakerieger/Xen/issues/3))
- A new `os` namespace with platform utilities such as `os.exit()` and `os.exec()`
- Additional methods to the `io` namespace (`io.clear`, `io.pause`)
- String character indexing with `[]`
- Line history to REPL ([#9](https://github.com/jakerieger/Xen/issues/9))

### Changed
- Moved `readtxt` and `readline` from `io` namespace to `os` namespace

### Fixed
- Incorrect formatting when printing numbers ([#8](https://github.com/jakerieger/Xen/issues/8))

### Known Bugs
- Segfault when executing a line of code immediately following a runtime error. This needs to be investigated further but I've narrowed down a 
method for reproducing it consistently.

## v0.4.1 (January 4, 2026)

### Changed
- Arrow functions no longer require a semicolon `;` at the end of the function anymore:
```js
// OLD
fn xadd(x) => ++x;
//             needed here as well
//                      v
some_func(fn(x) => x / 2;, 100);

// NEW
fn xadd(x) => ++x
some_func(fn(x) => x / 2, 100);
```

## v0.4.0 (January 3, 2026)

Xen 0.4 adds constructors, methods and properties, lambdas, and constant variable declarations. It also makes some breaking changes to the `io` namespace and adds syntactic sugar for single-line implicit return functions.
The bulk of Xen's features have been implemented (*or at least the foundation laid*), so major updates won't be as frequent going forward. The goal
now is to shift towards improving what's already been implemented and ensuring everything is robust before continuing to build more features.

### Added
- Object methods and properties (i.e, `my_string.len` or `my_array.push(69)`).
- Anonymous functions (lambdas).
- Arrow syntax `=>` for single-line implicit return functions.
- Additional methods to the `io` namespace for file reading (`readtxt` and `readlines`).
- Constructors for numbers, strings, booleans, and arrays.
- Constant variables with `const var`.

### Changed
- Array literals have a max size of 256 elements. Arrays can be dynamically resized to any size (up to your memory limit of course).
- `io.readline` is now `io.input` and accepts an optional string argument for prefix text.

### Removed
Nothing.

### Fixed
- Large numbers would print in scientific notation instead of displaying all digits. This was caused by the use of `%g` instead of `%f` in the corresponding `printf` call (see [xvalue.c](https://github.com/jakerieger/Xen/blob/master/src/xvalue.c#L94)).

### Known Bugs
- When indexing arrays, the parser doesn't recognize operations performed directly on the indexed object. For example, `my_arr[0]++` does not work. Neither does `my_arr[0]()` in an array of function elements.


## v0.3.0 (January 2, 2026)

Xen 0.3 brings arrays to the table. At the moment they can only store up to 256 elements per array, but this is likely to change in the near future. Eight bits
are just easier to work with when implementing and testing new features. Iteration can be achieved in a multitude of different ways, offering flexibility to
the programmer. The `array` module has also been added that includes a lot of useful array operations such as `len`, `first`, `last`, `index_of`, `join`, and
many more.

### Added
- Array objects (`xen_obj_array`)
- `array` module


### Changed
- `io.println` doesn't add a space between each argument when printing anymore
- `typeof` remains in the global namespace; everything else has been moved to specialized namespaces.

### Removed
- Bytecode emission has been disabled for now


### Fixed
- Operator precedence incorrectly set to `PREC_NONE` for some tokens in the parse rules table. Settting these to `PREC_CALL` fixed the problem

- Xen object equality check was missing certain cases


## v0.2.0 (January 2, 2026)

This is the update that brings the bulk of Xen's features to life. Support has been added for:

- Control flow (`if`/`else`, `for`/`while`) blocks
- `and` and `or` keywords
- Namespaces and object properties
- Package imports (`include` keyword)
- Recursion
- Compound operators (`+=`, `/=`, `%=`, etc.)
- Iteration via C-style or range-based loops (`for(def; cond; iter)` or `for(var in min..max)`)
- Pre- and post-fix operators for incrementing and decrementing values (`++x`, `x++`, `--x`, `x--`)

A lot of work has also been spent developing Xen's standard library, which now boasts four modules: `io`, `math`, `string`, and `datetime`. Documentation for these will be coming in the near future. These can be
imported and used in Xen scripts like so:

```
include io;

io.println("Hello, Xen!");
```


## v0.1.0 (January 1, 2026)

The core foundation of the language has been implemented. Minor bug testing has taken place and most of the breaking bugs have been
worked out. This puts Xen in a very exciting position with many new updates on their way.

Current language features:

- Variable declarations (global *and* local)
- Functions
- Native functions (`print`, `type`)
- readline-style REPL input editing with multi-line support
- Bytecode emission

### Bytecode emission

Work has begun on the bytecode emission format for Xen (called XLB). This format allows Xen source code to be compiled to an 
executable format the Xen VM recognizes and can execute. This feature is not functional yet but a large portion of the format itself
has already been implemented.
