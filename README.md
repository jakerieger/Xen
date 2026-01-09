# Xen

<p align="center">
    <img src="./docs/FileIcon.png"/>
</p>

<p align="center">
    <a href="#getting-started">Getting Started</a> | <a href="https://xenlanguage.github.io/documentation.html">Documentation</a> | <a href="#examples">Examples</a>
</p>

---

Xen is a loosely-typed, imperative scripting language written in C. Its syntax closely resembles C-like languages and the interpreter itself is a mere 120 Kb in size. Its primary purpose is to serve as a learning project for myself and a fun side project to work on in my free time. **It is not designed with the intent of being a serious, production-ready language.**

<p align="center">
    <img src="docs/demo.gif"/>
</p>

## Getting Started

If you want to try Xen out, you can download one of the pre-compiled binaries from our [releases](https://github.com/jakerieger/Xen/releases/latest).
Xen currently has releases for Windows (*x64*), Linux (*x64*), and macOS (*x64/ARM64*).

If you prefer to build from source, you need to be on a system that supports makefiles.

### 1. Clone the repository

```
$ git clone https://github.com/XenLanguage/Xen
```

### 2. Run make

```
$ cd Xen
$ make
```

If you want to specify a platform or target, you can do so like this:

```
$ export BUILD_TYPE=release
$ export TARGET_PLATFORM=windows
$ make
```

That's pretty much it. Builds are located in `build/`.

## Cross-building

Since Xen is developed by one guy (me) and since I only have one PC, builds for other platforms are generated using cross-compilation with toolchains like [MinGW](https://www.mingw-w64.org/) and [osxcross](https://github.com/tpoechtrager/osxcross). If you have these toolchains and they're in your path, you can build for all platforms with:

```
$ make all-platforms
```

> [!IMPORTANT]
> **Building from source on platforms other than Linux is not tested.**

## Editor Support

While an LSP is still in development, there are syntax highlighting support extensions for both VS Code and Neovim:

- [Download (VS Code)](https://github.com/XenLanguage/Xen-VSCode) (Install with `Developer > Install Extension from Directory`)
- [Download (Neovim)](https://github.com/XenLanguage/Xen-Neovim)

## Examples

Example code can be found in the [examples](examples) directory. The [syntax.xen](examples/syntax.xen) file showcases the entire syntax of Xen.

## License

**Xen** is licensed under the [ISC license](LICENSE).
