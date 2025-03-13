#include "cpu/riscv/core.h"
#include "cpu/riscv/cache-def.h"

using namespace kxemu::cpu;

#ifdef CONFIG_ICache

void RVCore::icache_push(do_inst_t do_inst, unsigned int instLen) {
    word_t set = ICACHE_SET(this->pc);
    this->icache[set].valid = true;
    this->icache[set].tag = ICACHE_TAG(this->pc);
    this->icache[set].do_inst = do_inst;
    this->icache[set].instLen = instLen;
    this->icache[set].decodeInfo = this->gDecodeInfo;
}

bool RVCore::icache_decode_and_exec() {
    word_t set = ICACHE_SET(pc);
    const ICacheBlock &block = this->icache[set];
    if (block.tag == ICACHE_TAG(pc) && block.valid) {
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

void RVCore::icache_push(do_inst_t do_inst, unsigned int instLen) {}

bool RVCore::icache_decode_and_exec() {
    NOT_IMPLEMENTED();
    return false;
}

void RVCore::icache_fence() {}

#endif