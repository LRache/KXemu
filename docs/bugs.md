# BUGS Report

## RISC-V

1. riscv64 linux5.15测试下，运行`busybox sleep 1`会无输出，同时linux内核会在几次时钟中断之后，不再设置stimecmp寄存器。
