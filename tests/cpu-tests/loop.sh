#!/bin/bash
for i in {1..100}
do
    make run ISA=riscv64 ALL=leap-year
done