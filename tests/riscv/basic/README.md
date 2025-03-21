# Test for RISCV

## deleg

这个测试用于测试riscv的trap代理。代码将U模式下的`ecall`内陷代理给了S模式，U模式下通过`ecall`进入S模式，再在S模式下通过`sret`返回到U模式并退出。正确的输出应该如下：

```text
Enter to S-mode
Call from U-mode
Enter to U-mode
U-mode ECALL
```

## smode

这个测试用于测试KXemu能否进入到S模式。代码将`mstatus`寄存器的`MPP`字段设置为了`S`，并通过`mret`进入到`S`模式，在模式下再次使用`ecall`指令进入到M模式后返回，最后退出程序。正确的输出应该如下：

```text
Enter to S-mode
Call from U-mode
Enter to U-mode
U-mode ECALL
```


## mtimer

这个测试用于测试KXemu的M模式下的定时功能，即设置mtime和mtimecmp寄存器。正确的输出应该每隔约1秒输出一行字符串。

## vm32 vm64

这两个测试用于测试虚拟内存的实现。M模式设置好页表后，返回到U模式，U模式在分页上运行，并尝试读写数据，每次读写结束返回至M模式验证读写结果正确性。
