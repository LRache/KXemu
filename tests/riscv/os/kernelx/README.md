# KernelX on KXemu

在 `kxemu-system-riscv64` 上启动 [KernelX](https://github.com/LRache/KernelX)
的 riscv64 版本。KernelX 源码是 `tests/os/KernelX` 子模块，跟踪 `dev` 分支。

启动路径与 QEMU virt 一致：

- OpenSBI `fw_jump` 运行在 M-mode，加载地址 `0x80000000`
- KernelX `Image` 由 OpenSBI 跳到 `0x80200000`（S-mode）
- 设备树加载到 `0x82200000`（OpenSBI fw_jump 的固定 FDT 地址），经 `a1` 传给内核
- 根文件系统是 virtio-mmio 块设备 `0x10001000`（PLIC IRQ 1）上的 ext4 镜像
- 串口是 `0x10000000` 上的 ns16550a（PLIC IRQ 10）

## 依赖

- KernelX 子模块及其 lwext4 子模块：

  ```sh
  git submodule update --init --recursive tests/os/KernelX
  ```

- Rust nightly，并安装 `riscv64gc-unknown-none-elf` target 和 `rust-src` 组件
- clang / lld（KernelX 的 C 库与 vDSO 使用 clang 交叉编译）
- `llvm-objcopy`（或带版本号的 `llvm-objcopy-21`）
- `dtc`、`mke2fs`、`cpio`
- riscv64 musl 交叉编译器（编译根文件系统里的 init/hello），默认查找
  `riscv64-linux-musl-gcc`，找不到时回退到
  `/opt/cross-tools/riscv64-linux-musl/bin/riscv64-linux-musl-gcc`；
  可用 `MUSL_CC=` 覆盖
- riscv64 GNU 交叉编译器（编译 OpenSBI），默认查找
  `riscv64-unknown-linux-gnu-gcc`，找不到时回退到
  `/opt/cross-tools/riscv64-unknown-linux-gnu/bin/riscv64-unknown-linux-gnu-`；
  可用 `CROSS_COMPILE=` 覆盖

## 构建和运行

```sh
cd tests/riscv/os/kernelx
make run
```

KDB 启动后输入 `run`。正常启动时最后会看到：

```text
[   0.164204] [INFO] Welcome to use KernelX!
Hello from KernelX userspace on KXemu!
[   0.214317] [PANIC] Init process exited with status Normal(0), system will halt.
```

`make` 会依次完成：

1. 用 `tests/os/KernelX` 构建 `build/riscv64/Image` 和 `vmkernelx`
2. 构建一份关闭 semihosting 的 OpenSBI `fw_jump`（见下）
3. 生成 `build/kernelx-riscv64.dtb`
4. 用 musl 编译 `usertests/init` 和一个 hello 程序，用 `mke2fs -d` 打包成
   `build/rootfs.ext4`

`make clean` 会删除本目录的 `build/`，并清理 KernelX 的构建产物。

## 多核

`SMP` 控制 hart 数，默认 4，范围 1–16（KXemu 的 PLIC 有 32 个 context，每 hart
占两个 M/S context）。`gen-dts.py` 会按 `SMP` 生成同样数量的 CPU 节点、
PLIC S-mode context 和 CLINT 中断项，KDB 也会用 `--core $(SMP)` 启动同样数量的
核心：

```sh
make run            # 4 核
make run SMP=2
make run SMP=1
```

KernelX 从设备树读取 hart 数，并通过 SBI HSM 的 `hart_start` 唤醒其余 hart。
多核下 OpenSBI 的 boot hart 是通过抽签决定的，不一定是 hart 0；因此 DTB 固定
加载到 `0x82200000`（OpenSBI `fw_jump` 的 `FW_JUMP_FDT_OFFSET` 为 `0x2200000`），
这样无论哪个 hart 中签，内核都能拿到有效设备树。KXemu 的 KDB `set reg a1` 只
作用于当前 core（core 0），不能依赖它给 boot hart 传 FDT 地址。

## 为什么单独构建 OpenSBI

KXemu 当前配置里 `CONFIG_HALT_WHEN_BREAK=y`，任何 `ebreak` 都会让 hart 停机。
OpenSBI 的 generic 平台默认打开 `CONFIG_SERIAL_SEMIHOSTING`，启动时会执行一条
`ebreak` 探测 semihosting 是否可用，于是直接触发 KXemu 的停机。本目录的
Makefile 因此用同一份 OpenSBI 源码单独生成 `.config`，把
`CONFIG_SERIAL_SEMIHOSTING` 关掉后再编译 `fw_jump.bin`。

## 文件

- `Makefile`：KernelX / OpenSBI / DTB / rootfs 的构建入口
- `gen-dts.py`：按 `SMP` 生成 KXemu virt 设备树（`/chosen/bootargs` 指定
  `root=`、`init=` 和 `initargs=`）
- `kernelx-riscv64.kdb`：KDB 加载脚本
- `initramfs/hello.c`：运行在用户态的测试程序
