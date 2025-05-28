#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: $0 linux5.15"
    exit 1
fi

if [ $1 == "linux5.15" ]; then
    wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.132.tar.xz
    tar -xf linux-5.15.132.tar.xz
else
    echo "Usage: $0 linux5.15"
    exit 1
fi
