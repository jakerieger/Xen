# Changelog

## Table of Contents
- [v0.3.0](#v030-january-2-2026)
- [v0.2.0](#v020-january-2-2026)
- [v0.1.0](#v010-january-1-2026)

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