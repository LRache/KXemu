# KXemu

## NEWS

2025.5.29 **成功在kxemu-system-riscv64上启动linux-5.15** ，但是还未尝试挂载init root fs。

## 关于KXemu About KXEMU

KXemu(Kexie Emulator)是一款以学习为目的开发的不同指令集的CPU模拟器，类似于大名鼎鼎的QEMU和NJUPA的NEMU。它可以用作学习计算机体系结构和操作系统，或者用于DIFFTEST。

KXemu(Kexie Emulator) is an educational CPU simulator for various ISAs, similar to the renowned QEMU and NJUPA's NEMU. It can be used for learning computer architecture and operating systems or for DIFFTEST.

我们是一群来自杭州电子科技大学的学生，非常喜欢计算机体系结构与操作系统。我们欢迎并希望有喜欢这些内容的同学加入我们，一起开发和维护KXemu这个项目。

We are a group of students from Hangzhou Dianzi University who are passionate about computer architecture and operating systems. We warmly welcome anyone with similar interests to join us in developing and maintaining the KXemu project.

KXemu现在的主要功能包括了：

The main features of KXemu include

- 一个简单的调试器KDB A simple debugger KDB

    - 单步调试 Single step

    - 查看内存和寄存器 Show memory and registers' value

    - 断点 Breakpoints

    - 加载ELF文件和符号 Load ELF files and load symbols from ELF

- CPU核心模拟 CPU core emulation

    - [riscv32](./docs/isa/riscv32.md)

    - [riscv64](./docs/isa/riscv64.md)

    - [loongarch32](./docs/isa/loongarch32.md)

- 设备 Devices

    - 内存 Memory

    - [uart16650](./docs/device/uart16650.md)

    - [Virtio Block](./docs/device/virtio-blk.md)

    - [MMIO总线 MMIO bus](./docs/device/mmio-bus.md)

## 使用方法 Usage

查看文档 See docs

- [编译和运行 Compile and Run](./docs/run.md)

- [KDB命令 KDB Commands](./docs/kdb.md)

- [测试 Tests](./docs/tests.md)

## 文件结构 File Structure

- `include` KXemu所需要的头文件。Header files required by KXemu.

- `src` KXemu的源代码。The source code of KXemu.

- `scripts` 编写KXemu需要的脚本以及KDB运行脚本。Scripts for KXemu development and KDB execution.

- `tests` 包含了KXemu的测试，大多数测试可以使用`make ISA=[isa] NAME=`(除了cpu-tests)或者`make ISA=[isa] ALL=`(cpu-tests)的方法运行。Contains tests for KXemu. Most tests can be run using `make ISA=[isa] NAME=` (except for CPU tests) or `make ISA=[isa] ALL=` (for CPU tests).

- `utils` 辅助KXemu的工具，但是没有这些工具KXemu同样可以正常运行。Tools to assist KXemu, though KXemu can run normally without them.

## 贡献

因为这是一个开源的学习项目，而我们也只是学生，所以我们非常期待你能够对我们的代码提出建议或者完善一些功能。你可以直接fork我们的仓库，在修改后提出PR，我们会在1到2天内回复。如果你是杭州电子科技大学的学生，你可以直接联系杭州电子科技大学 **计算机科学与技术协会(计科协)**，我们可以共同探讨。我们创立这个项目的初衷，一是想继续学习，二是在NEMU上直接修改过于麻烦。

As this is an open-source learning project and we are still students, we highly value your suggestions or contributions to improve the codebase. You can fork our repository and submit a PR after making changes; we will respond within 1-2 days. If you are a student at Hangzhou Dianzi University, feel free to contact the **HDU-CSKX**, and we can explore ideas together. The purpose of this project is to continue learning and address the challenges of directly modifying NEMU.

邮箱：ziyou@hdu.edu.cn (预计会在1周之内回复，如果你是校内的同学直接找计科协的系统组应该会更快。)

Email: ziyou@hdu.edu.cn (We will respond within one week. If you are a student at Hangzhou Dianzi University, contacting the Systems Group of the Computer Science Association directly might be faster.)

## 声明 Disclaimer

如果你需要将本项目用于比赛或者商业盈利性活动，那么你**必须在显著位置**标明本项目的主要贡献者(如果我们更新了)和开源链接。

If you wish to use this project for competitions or commercial profit-driven activities, you **must prominently credit** the main contributors of this project (if updated) and include the open-source link.