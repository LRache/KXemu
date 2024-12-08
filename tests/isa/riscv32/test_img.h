// This file is just used to temp test

#ifndef __TEST_IMG_H__
#define __TEST_IMG_H__

#include <stdint.h>

#define NOP 0x00000013
#define __EBREAK 0x00100073
#define GOOD_TRAP 0x00000513, __EBREAK,

static uint32_t test_img_ebreak[] = {
    __EBREAK
};

static uint32_t test_img_illegal_inst[] = {
    0,
    __EBREAK
};

static uint32_t test_img_athrimatic[] =  {
    0x00a00093, // 00 addi x1, x0, 10
    0x00008133, // 04 add x2, x1, x0
    0x00000413, // 08 addi x8,  x0, 15   
    0x01b00593, // 0c addi x11,  x0, 27    
    0x00b50633, // add  x12, x10, x11  
    0x40b50633, // sub  x12, x10, x11  
    0x00b57633, // and  x12, x10, x11 
    0x00b56633, // or   x12, x10, x11   
    0x00b54633, // xor  x12, x10, x11   
    0x00300693, // addi x13,  x0,  3    
    0x00d00713, // addi x14,  x0, 13    
    0x00d717b3, // sll  x15, x14, x13   
    0x00d757b3, // srl  x15, x14, x13   
    0x40d757b3, // sra  x15, x14, x13   
    0x00d727b3, // slt  x15, x14, x13
    0x06300413, // addi x8, x0, 99
    0x08247613, // andi x12, x8, 130
    0x00f00613, // slti x12, x0, 15     
    0x00f00693, // addi x13, x0, 15     
    0x00f00713, // andi x14, x0, 15     
    0x00f00793, // ori  x15, x0, 15     
    0x00f00813, // addi x16, x0, 15    
    0x00100893, // slli x17, x1, 1      
    0x00100913, // srli x18, x1, 1     
    0x40100913, // srai x18, x1, 1
    
    GOOD_TRAP 
};                                       

static uint32_t test_img_branch[] {
    0x00f00513, // 00 addi x10, x0, 15
    0x01b00593, // 04 addi x11, x0, 27
    0x00b50633, // 08 add  x12, x10, x11
    0x00f52693, // 0c slti x13, x10, 15
    0x02000793, // 10 addi x15, x0, 32
    
    0x00b70063, // 14 beq  x14, x11, 16 (offset = 4 instructions)
    0x00a00713, // 18 addi x14, x0, 10 (will be skipped if beq is taken)
    0x00000713, // 1c addi x14, x0, 0  (will be skipped if beq is taken)
    0x00e00713, // 20 addi x14, x0, 14 (will be executed if beq is not taken)

    0x00a78063, // 24 bne  x15, x10, 16 (offset = 4 instructions)
    0x00100793, // 28 addi x15, x0, 1 (will be skipped if bne is taken)
    0x00000793, // 1c addi x15, x0, 0 (will be skipped if bne is taken)
    0x00f00793, // 20 addi x15, x0, 15 (will be executed if bne is not taken)

    GOOD_TRAP
};

static uint32_t test_img_upper[] = {
    0x00123517, // auipc x10, 291
    0x01edf597, // auipc x11, 7903
    0x00123537, // lui x10, 291
    0x01edf5b7, // lui x11, 7903

    GOOD_TRAP
};

static uint32_t test_img_jump0[] = {
    0x00000517, // auipc x10, 0
    0x00c500e7, // jalr x1, 12(x10)
    0x00000000, // invalid
    GOOD_TRAP
};

static uint32_t test_img_jump1[] = {
    0x00f00513, // 00 addi x10, x0, 15
    0x01b00593, // 04 addi x11, x0, 27
    0x00b50633, // 08 add  x12, x10, x11
    0x00f52693, // 0c addi x13, x10, 15
    0x02000793, // 10 addi x15, x0, 32
    0xff5ff0ef, // 14 jal ra, -12
    GOOD_TRAP
};

static uint32_t test_img_mem1[] = {
    0x020000b7, // lui x1, 0x02000
    0x0000a103, // lw x2, 0(x1)
    0x0040a183, // lw x3, 4(x1)
    
    0xa0000537, // 00 lui x10, 0x0f000
    0x810285b7, // 04 lui x11, 0x01020
    0x30458593, // 08 addi x11, x11, 0x304
    0x00b52023, // 0c sw x11, 0(x10)
    0x00050603, // 10 lb  x11, 0(x10)
    0x00150583, // 14 lb  x11, 1(x10)
    0x00250583, // 18 lb  x11, 2(x10)
    0x00350583, // 1c lb  x11, 3(x10)
    0x00055603, // 20 lhu x12, 0(x10)
    0x00255603, // 24 lhu x12, 2(x10)
    0x00051683, // 28 lh  x13, 0(x10)
    0x00251683, // 2c lh  x13, 2(x10)
    0x00052683, // 30 lw  x13, 0(x10)

    0x00c00193, // 34 addi x3, x0, 12
    0x00350223, // 38 sb x3, 4(x10)
    0x00452203, // 3c lw x4, 4(x10)
    0x003502a3, // 40 sb x3, 5(x10)
    0x00452203, // 44 lw x4, 4(x10)
    0x00350323, // 48 sb x3, 6(x10)
    0x00452203, // 4c lw x4, 4(x10)
    0x003503a3, // 50 sb x3, 7(x10)
    0x00452203, // 54 lw x4, 4(x10)

    0x7bc00293, // 58 addi x5, x0, 0x7bc
    0x00551423, // 5c sh x5, 8(x10)
    0x00852303, // 60 lw x6, 8(x10)
    0x00551523, // 64 sh x5, 10(x10)
    0x00852303, // 68 lw x6, 8(x10)

    GOOD_TRAP
};

static uint32_t test_img_mem2[] = {
    0x00000517, // 00 auipc x10, 0
    0x7ff50513, // 04 addi x10, x10, 2047
    0x00150513, // 08 addi x10, x10, 1
    0x00a00133, // 0c add x2, x0, x10
    0x02100793, // 10 addi x15, x0, 33
    0x00f12423, // 1c sw x15, 8(x2)
    0x00810803, // 20 lb x16, 8(x2)
    0x00814883, // 24 lbu x17, 8(x2)
    0x00811903, // 28 lh x18, 8(x2)
    0x00815983, // 2c lhu x19, 8(x2)
    0x00812a03, // 30 lw x20, 8(x2)
    
    GOOD_TRAP
};

static uint32_t test_img_mrom[] = {
    0x00000517, // 00 auipc x10, 0
    0x01750513, // 04 addi x10, x10, 0x17
    0x00054583, // 08 lbu x11, 0(x10)
    GOOD_TRAP  // 0c 10
    0x01020304  // 14        
};

static uint32_t test_img_sram[] = {
    0x0f0000b7, // lui x1, 0x0f000
    0x01b00113, // addi x2, x0, 27
    0x0020a023, // sw x2, 0(x1)
    0x0000a183, // lw x3, 0(x1)

    GOOD_TRAP
};

static uint32_t test_img_uart[] = {
    0x100007b7, // lui a5,0x10000
    0x0037c683, // lbu a3,3(a5)

    GOOD_TRAP
};

static uint32_t test_img_flash[] ={
    0x30000537, // lui x10, 0x30000
    0x00054583, // lbu x11, 0(x10)
    0x00154583, // lbu x11, 1(x10)
    0x00254583, // lbu x11, 2(x10)
    0x00354583, // lbu x11, 3(x10)

    GOOD_TRAP
};

static uint32_t test_img_spi[] ={
    0x10001537, // lui x10, 0x10001
    0x00a00593, // li x11, 10
    0x00b52023, // sw x11, 0(x10)

    GOOD_TRAP
};

static uint32_t test_img_dummy[] = {
    0x00000413,
    0x00009117,
    0xffc10113,
    0x00c000ef,
    0x00000513,
    0x00008067,
    0xff410113,
    0x00000517,
    0x01450513,
    0x00112423,
    0xfe9ff0ef,
    __EBREAK
};

static uint32_t test_img_csrrw[] = {
    // 0x7ff50513, // 00 addi x10, x10, 2047
    // 0x342515f3, // 04 csrrw x11, mcause, x10
    0x300515f3, // 08 csrrw x11, mstatus, x10
    0x30001573, // 0c csrrw x10, mstatus, x0
    0x30001373, // 10 csrrw x6, mstatus, x0
    0x30002373, // 14 csrrs x6, mstatus, x0
    GOOD_TRAP
};

static uint32_t test_img_fun[] = {
    0xef002117, // auipc sp,0xef002
    0xffc10113, // addi	sp,sp,-5 # f001fff
    0xff010113, // addi	sp,sp,-16
    0x00010513, // mv	a0,sp
    0x0aa00793, // li	a5,170
    0x00f11023, // sh	a5,0(sp)
    0x00012503, // lw   a0,0(sp)
    // 0x00054503, // lbu	a0,0(a0)

    GOOD_TRAP
};

static uint32_t test_img_ecall[] = {
    0x00000517, // 00 auipc x10, 0
    0x01850513, // 04 addi x10, x10, 24
    0x30551073, // 08 csrrw x0, mtvec, x10
    0xfff00793, // 0c addi x15, x0, -1
    0x00000073, // 10 ecall
    0x00000000, // 14
    0x30200073, // mret
    0x34201673, // 18 csrrw x12, mcause, x0

    GOOD_TRAP
};

static uint32_t test_img_psram[] = {
    0x800000b7, // 00 lui x1, 0x80000
    0x12300113, // 04 li x2, 0x123
    // 0x00208223, // sb x2, 4(x1)
    // 0x00209223, // sh x2, 4(x1)
    0x00208223, // 08 sb x2, 4(x1)
    0x0040a183, // 0c lw x3, 4(x1)

    GOOD_TRAP
};

static uint32_t test_img_sdram[] = {
    0xa00000b7, // 00 lui x1, 0xa0000
    // 0xa20001b7, // 04 lui x3, 0xa2000
    // 0xffc18193, // 08 addi x3, x3, -4
    0x12345137, // 0c lui x2, 0x12345
    0x67810113, // 10 addi x2, x2, 0x678
    0x0020a023, // 14 sw x2, 0(x1)
    // 0x0020a023, // 14 sw x2, 0(x1)
    // 0x0021a023, // 18 sw x2, 0(x3)
    0x0000a183, // 1c lw x2, 0(x1)

    GOOD_TRAP
};

static uint32_t test_img_gpio[] = {
    0x100020b7, // 00 lui x1, 0x10002
    // 0x00100113, // 04 addi x2, x0, 1
    0x0020a023, // 04 sw x2, 0(x1)
    GOOD_TRAP
};

static uint32_t test_img_dead_loop[] = {
    // 0x00100093, // addi x1, x0, 1
    // 0x00200113, // addi x2, x0, 2
    // 0x00300193, // addi x3, x0, 3
    // 0x00400213, // addi x4, x0, 4
    0x30000537, // 00 lui x10, 0x30000
    0x00052083, // 04 lw x1, 0(x10)
    0x00052103, // 08 lbu x11, 1(x10)
    0x00052183, // 0c lbu x11, 2(x10)
    0x00052203, // 10 lbu x11, 3(x10)
    0xff1ff06f, // 14 j -16
    GOOD_TRAP
};

static uint32_t test_img_pipeline_no_hazard_ivd[] = {
    0,
    0,
    0,
    0,
    0,
    __EBREAK
};

static uint32_t test_img_no_hazard_addi[] = {
    0x00100093, // 00 addi  x1, x0,  1
    0x00200113, // 04 addi  x2, x0,  2
    0x00300193, // 08 addi  x3, x0,  3
    0x00400213, // 0c addi  x4, x0,  4
    0x00500293, // 10 addi  x5, x0,  5
    0x00600313, // 14 addi  x6, x0,  6
    0x00700393, // 18 addi  x7, x0,  7
    0x00800413, // 1c addi  x8, x0,  8
    0x00900493, // 20 addi  x9, x0,  9
    0x00a00513, // 24 addi x10, x0, 10
    0x00b00593, // 28 addi x11, x0, 11
    0x00c00613, // 2c addi x12, x0, 12
    0x00d00693, // 30 addi x13, x0, 13
    0x00e00713, // 34 addi x14, x0, 14
    0x00f00793, // 38 addi x15, x0, 15
    GOOD_TRAP
};

static uint32_t test_img_simple_data_hazard[] = {
    0x00100093, // addi x1, x0, 1
    0x00208093, // addi x1, x1, 2
    __EBREAK
};

static uint32_t test_img_data_hazard[] = {
    0x00100093, // addi x1, x0, 1
    0x00208093, // addi x1, x1, 2
    0x00308093,
    0x00408093,
    0x00508093,
    0x00608093,
    0x00708093,
    0x00808093,
    0x00908093,
    0x00a08093,
    __EBREAK
};

static uint32_t test_img_simple_mem[] = {
    0xa0000537, // 00 lui x10, 0xa0000
    // 0x123455b7, // 04 lui x11, 0x12345
    // 0x67858593, // 08 addi x11, x11, 0x678
    // 0x00b52023, // 0c sw x11, 0(x10)
    0x00050603, // 10 lb x12, 0(x10)
    // 0x00100093, // 14 addi x1, x0, 1
    // 0x00200093, // addi x1, x0, 2
    GOOD_TRAP
};

static uint32_t test_img_simple_mem2[] = {
    0xa0000537, // 00 lui x10, 0xa0000
    0x7bc00293, // 04 addi x5, x0, 0x7bc
    0x00852283, // 08 lw x5, 8(x10)
    // 0x00852303, // 60 lw x6, 8(x10)
    // 0x00551523, // 64 sh x5, 10(x10)
    // 0x00852303, // 68 lw x6, 8(x10)
    GOOD_TRAP
};

static uint32_t test_img_simple_mem3[] = {
    // 0xa0000237, // 00 lui x4, 0xa0000
    // 0x7bc00293, // 04 addi x5, x0, 0x7bc
    // 0x00521423, // 0c sh x5, 8(x4)
    // // 0x13,
    // // 0x13,
    // 0x00822303, // 10 lw x6, 8(x4)
    // 0x00521523, // 10 sh x5, 10(x4)
    // 0x00822303, // 14 lw x6, 8(x4)

    0xa0000537, // 00 lui x10, 0xa0000
    0x7bc00293, // 04 addi x5, x0, 0x7bc

    0x00551423, // 08 sh x5, 8(x10)
    0x00852303, // 0c lw x6, 8(x10)
    0x00551523, // 10 sh x5, 10(x10)
    0x00852383, // 14 lw x7, 8(x10)

    GOOD_TRAP
};

static uint32_t test_img_control_hazard1[] = {
    0x0080016f, // 00 jal x2, 8
    0x00000000, // invalid

    GOOD_TRAP
    0x00000000, // invalid

    0x00100093, // 08 addi x1, x0, 1
    0x00008067, // jalr x0, 0(x1)
};

static uint32_t test_img_control_hazard2[] = {
    0x00000093, // 00 li x1, 0
    0x01008193, // 04 addi x3, x1, 16
    0x00408093, // 14 addi x1, x1, 4
    0xfe309ee3, // 18 bne x1, x3, -4
    GOOD_TRAP
};

static uint32_t test_img_control_hazard3[] = {
    0xa00000b7, // 00 lui x1, 0xa0000
    0x01008193, // 04 addi x3, x1, 16
    0x12345137, // 08 lui x2, 0x12345
    0x2a610113, // 0c addi x2, x2, 678
    0x0020a023, // 10 sw x2, 0(x1)
    0x00408093, // 14 addi x1, x1, 4
    0xfe309ce3, // 18 bne x1, x3, -8
    GOOD_TRAP
};

static uint32_t test_img_control_hazard4[] = {
    0xa0000537, // 00 lui x10, 0xa0000
    0x00052583, // 04 lw x11, 0(x10)
    0x00208093, // addi x1, x1, 2
    0x004000ef, // 08 jal x1, 4
    0x00052583, // 0c lw x11, 0(x10)
    0x00208093, // addi x1, x1, 2
    0x0040016f, // 10 jal x2, 4
    0x00052583, // 14 lw x11, 0(x10)
    0x00208093, // addi x1, x1, 2
    0x004001ef, // 14 jal x3, 4
    0x00052583, // 18 lw x11, 0(x10)
    0x00208093, // addi x1, x1, 2
    GOOD_TRAP
};

static uint32_t test_img_bootloader[] = {
    0x70001717, // 00 auipc a4, 0x70001
    0xfe070713, // 04 addi a4, a4, -32
    0x00000797, // 08 auipc a5, 0x0
    0x03878793, // 0c addi a5, a5, 56
    0x02f70663, // 10 beq a4, a5, 3000005c
    0x0bc00613, // 14 li a2, 188
    0x00265613, // 18 srli a2, a2, 0x2
    0x02060063, // 1c beqz a2, 3000005c
    0x00261613, // 20 slli a2, a2, 0x2
    0x00c78633, // 24 add a2, a5, a2
    0x0007a683, // 28 lw a3, 0(a5)
    0x00478793, // 2c addi a5, a5, 4
    0x00470713, // 30 addi a4, a4, 4
    0xfed72e23, // 34 sw a3, -4(a4)
    0xfec798e3, // 38 bne a5, a2, 0x28 (-16)
    GOOD_TRAP   // 3c 40
};

static uint32_t test_img_temp[] = {
    0x00000793, // 00 li a5, 0
    0x00800613, // 04 li a2, 8
    0x00478793, // 08 addi a5, a5, 4
    NOP,        // 0c
    NOP,        // 10
    0xfec79ae3, // 14 bne a5, a2, 0x28 (-12)
    GOOD_TRAP   // 18 1c
};

static uint32_t test_img_ivd_load[] = {
    0x500000b7, // lui x1, 0x50000
    0x0000a103, // lw x2, 0(x1)
    GOOD_TRAP
};

static uint32_t test_img_clint[] = {
    0x00c000ef, // 00 jal x1, 12
    GOOD_TRAP   // 04 08
    0x020007b7, // 0c lui x15, 0x02000
    0x0007a603, // 10 lw x12, 0(x1)
    NOP,        // 14
    NOP,        // 18
    0x00008067, // 1c jalr x0, 0(x1)
};

static uint32_t test_img_btb[] = {
    0x00200093, // 00 addi x1, x0, 2
    0x00000113, // 04 addi x2, x0, 0
    0x00110113, // 08 addi x2, x2, 1
    0xfe209ee3, // 0c bne  x1, x2, -4
    GOOD_TRAP
};

#endif
