# 编译和运行

## 构建依赖

- Git 子模块：clone 时使用 `git clone --recurse-submodules`，或手动执行
  `git submodule update --init`（KDB 依赖的 readline 以子模块方式 vendored
  在 `utils/readline`，无需安装 `libreadline-dev`）。
- Linux：需要 `libncurses-dev`（readline 链接所需 termcap 函数在 ncurses 中）。
- macOS：无需额外安装，Xcode Command Line Tools 自带 ncurses。

KXemu 支持两种配置方式。两条路径最终都会把同一组 `CONFIG_*` 参数交给
根目录的 `build.mk`，并由 CMake 在构建目录生成对应的配置头文件。

## 使用 menuconfig

```sh
make menuconfig
make
make run
```

`menuconfig` 的结果保存在 `configs/.config`。顶层 `Makefile` 会读取生成的
配置，并把它传给 `build.mk`。

## 直接指定构建参数

不修改 `configs/.config`，可以直接调用 `build.mk`：

```sh
make -f build.mk CONFIG_ISA=riscv64
make -f build.mk CONFIG_ISA=riscv64 CONFIG_DEBUG=y CONFIG_TLB=y
make -f build.mk run CONFIG_ISA=loongarch64 CONFIG_CC=clang++
```

未指定的参数使用 `scripts/build-config.mk` 中与 Kconfig 一致的默认值。
命令行参数优先于默认值。常用参数包括 `CONFIG_ISA`、`CONFIG_CC`、
`CONFIG_DEBUG`、`CONFIG_ENABLE_ICACHE`、`CONFIG_TLB` 和
`CONFIG_TARGET_SHARE`。
