# KDB文档 Documentation

## KDB简介 Introduction

KDB是对KXemu的包装，用于用户控制KXemu中的CPU，并且连接CPU和其他外设。

## KDB启动参数 Startup Parameters

编译运行KXemu，会自动启动kdb命令行。kdb接受的命令行参数：

When KXemu is compiled and executed, the KDB command-line interface is automatically launched. KDB accepts the following command-line arguments:

- `-s --source [filename]` 在启动的时候自动运行脚本，这可以用于大多数的时候运行镜像文件。可以指定多个源文件，kdb会依次运行它们。Automatically executes a script at startup, often used for running image files. Multiple source files can be specified, and KDB will execute them in order.

- `-e --elf [filename]` 指定elf文件，在命令中使用`load elf`来将这个elf加载到内存的指定位置。Specifies an ELF file that can be loaded into memory using the load elf command.

## KDB命令

### 运行

- `step [n]` 运行当前选中的核心`n`步，遇到核心出现错误或者break时终止。`n`默认为`1`。默认会打印运行的指令和反汇编代码。Executes the selected core for n steps, stopping on errors or breaks. Defaults to n=1. The executed instruction and its disassembly are printed by default.

- `run` 运行当前核心，直到核心出现错误或者break时终止。Runs the current core(or halt in the riscv) until an error or break occurs.

### 日志

- `log on [DEBUG|INFO|WARN|PANIC]` 打开一项日志等级的输出，默认所有等级的输出都是开启的。Enables a specific log level output. By default, all log levels are enabled.

- `log off [DEBUG|INFO|WARN|PANIC]` 关闭一项日志等级的输出。Disables a specific log level output.

### 加载

加载指令用于加载在命令行参数中指定的文件，如ELF文件。

Load commands are used to load files specified in the command-line arguments, such as ELF files.

- `load elf` 加载命令行中由参数`--elf`或`-e`指定的`elf`路径，可以在批处理下设置好内存映射后延迟加载ELF文件。Loads the ELF file path specified by the --elf or -e argument, allowing deferred loading after memory mapping in batch processing.

### 内存

- `mem create [name] [addr] [size]` 创建一个名为`name`的、映射到`addr`位置的，长度为`size`的内存映射区域。Creates a memory-mapped region named name at address addr with a size of size.

- `mem img [addr] [filename]` 将本地文件加载到内存`addr`的位置，`addr`处映射的内存必须是一个用于存储的区域。Loads a local file into memory at address addr. The memory at addr must be a storage region.

- `x </> addr` 与gdb类似，打印内存中的数据。Similar to GDB, prints data from memory at the specified address.

### 串口

- `uart add <base> [port]` KXemu可以模拟一个UART16650设备，这条指令可以为CPU添加一个内存映射在`base`处的uart串口，如果指定端口，可以通过TCP套接字连接至串口，向端口`port`写入可以发送数据，同时可以从`port+1`端口接收KXemu的串口输出。如果不指定端口，则串口输出会自动输出至标准输出。在`./utils/uart`目录下有一个示例。KXemu can simulate a UART16650 device. This command maps a UART serial port to the CPU at the base address. If a port is specified, TCP sockets can be used to connect to the serial port, allowing data transmission to port and receiving KXemu's serial output from port+1. If no port is specified, output is directed to standard output. An example is available in the `./utils/uart` directory.

- `uart puts [str]` 将`str`字符串的内容加入到串口的FIFO中。Adds the content of the str string to the UART's FIFO.

### Debug

- `info <name>` 打印名字为`name`的寄存器。Prints the value of the register named name.

- `info gpr` 打印当前核的所有通用寄存器。Prints all general-purpose registers of the current core(or halt).

### 其他

- `source [filename]` 运行存储在本地的kdb命令文件。Executes a local KDB command file.
