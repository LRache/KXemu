// This file is just for test

#include <cstdint>

static const uint32_t test_img_ebreak[] = {
    0x00100073, // ebreak
};

static const uint32_t test_img_addi[] = {
    // 0x00100093, // addi x1, x0, 1
    0xffe00093, // addi x1, x0, -2
    0x00100073, // ebreak
};
