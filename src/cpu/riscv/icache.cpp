#include "cpu/riscv/addr.h"
#include "cpu/riscv/core.h"
#include "cpu/word.h"

using namespace kxemu::cpu;

#ifdef CONFIG_ENABLE_ICACHE

void RVCore::icache_push(do_inst_t do_inst, unsigned int instLen, const DecodeInfo &decodeInfo) {
    addr_t addr = this->pc;
    word_t set = addr.icache_set();
    this->icache[set].valid = true;
    this->icache[set].tag = addr.icache_tag();
    this->icache[set].do_inst = do_inst;
    this->icache[set].instLen = instLen;
    this->icache[set].decodeInfo = decodeInfo;
}

bool RVCore::icache_decode_and_exec() {
    addr_t addr = this->pc;
    const ICacheBlock &block = this->icache[addr.icache_set()];
    if (block.tag == addr.icache_tag() && block.valid) {
        this->npc = this->pc + block.instLen;

        (this->*block.do_inst)(block.decodeInfo);
        
        return true;
    } else {
        return false;
    }
}

void RVCore::icache_fence() {
    for (unsigned int i = 0; i < sizeof(this->icache) / sizeof(this->icache[0]); i++) {
        this->icache[i].valid = false;
    }
}

#else

void RVCore::icache_push(do_inst_t, unsigned int, const DecodeInfo &) {}

bool RVCore::icache_decode_and_exec() {
    return false;
}

void RVCore::icache_fence() {}

#endif