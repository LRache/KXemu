# KDB文档

## KDB简介

KDB是对KXemu的包装，用于用户控制KXemu中的CPU，并且连接CPU和其他外设。

## KDB启动

编译运行KXemu，会自动启动kdb命令行。kdb接受的命令行参数：

- `-s --source [filename]` 在启动的时候自动运行脚本，这可以用于大多数的时候运行镜像文件。可以指定多个源文件，kdb会依次运行它们。

- `-e --elf [filename]` 在运行完所有的启动脚本后，将elf文件加载到内存的指定位置。

## KDB命令

### 运行

- `step [n]` 运行当前选中的核心`n`步，遇到核心出现错误或者break时终止。`n`默认为`1`。默认会打印运行的指令和反汇编代码。

- `run` 运行当前核心，直到核心出现错误或者break是终止。

### 日志

- `log on [DEBUG|INFO|WARN|PANIC]` 打开一项日志等级的输出，默认所有等级的输出都是开启的。

- `log off [DEBUG|INFO|WARN|PANIC]` 关闭一项日志等级的输出。

### 加载

加载指令用于加载在命令行参数中指定的文件，如ELF文件。

- `load elf` 加载命令行中由参数`--elf`或`-e`指定的`elf`路径，可以在批处理下设置好内存映射后延迟加载ELF文件。

### 内存

- `mem create [name] [addr] [size] [type]` 创建一个名为`name`的、映射到`addr`位置的，长度为`size`的内存映射区域。`type`默认为`storage`，可以为以下值：

    + `storage` 表示用于存储的内存。

    + `uart` 表示串口。

    + `vga` 表示VGA视频输出。

- `mem img [addr] [filename]` 将本地文件加载到内存`addr`的位置，`addr`处映射的内存必须是一个用于存储的区域。

- `mem elf [filename]` 读取本地的elf格式文件，加载到内存中。

### Debug

- `info gpr` 打印当前核的所有通用寄存器。

### 其他

- `source [filename]` 运行存储在本地的kdb命令文件。

- `statistic` 输出统计信息，包括了运行的指令数、模拟速度等等。
