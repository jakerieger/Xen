# Changelog

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
