import re
from typing import List, Dict
from instpat import *

INST_PATTERN = re.compile(
    r'INSTPAT\s*\(\s*"([^"]+)"\s*,\s*([^,\s]+)\s*,\s*([^,\s]+)\s*(?:,\s*(\d+))?\s*\)\s*;'
)

def read_file(filename: str) -> List[List[InstPattern]]:
    f = open(filename, 'r')

    groups = []
    group = []

    for line in f:
        line = line.strip()
        if not line:
            continue

        if line == "---":
            groups.append(group)
            group = []
            continue

        m = INST_PATTERN.match(line)
        if not m:
            raise ValueError(f"Invalid line '{line}'")
        
        pattern     = m.group(1)
        instName    = m.group(2)
        decoderName = m.group(3)
        t = m.group(4)

        if t is None:
            t = InstType.Both
        else:
            if t == "32":
                t = InstType.Only32
            elif t == "64":
                t = InstType.Only64
            else:
                raise ValueError(f"Invalid type '{t}'")
        instName = instName.replace(".", "_")
        group.append(InstPattern(pattern, instName, decoderName, t))
    groups.append(group)

    f.close()
    return groups


def build_decode_table(groups: List[List[InstPattern]]) -> List[Dict[int, List[InstPattern]]]:
    cases = []
    for group in groups:
        case = {}
        for inst in group:
            mask = inst.mask
            if mask not in case:
                case[mask] = []
            case[mask].append(inst)
        cases.append(case)
    return cases


def gen_code_normal(instList: List[InstPattern], key) -> str:
    def helper(inst: InstPattern):
        if inst.instType == InstType.Only32:
            return f"        __INST32(case {hex(inst.key)}: this->decode_insttype_{inst.decoderName}(); this->do_{inst.instName}(this->gDecodeInfo); return &RVCore::do_{inst.instName};)\n"
        elif inst.instType == InstType.Only64:
            return f"        __INST64(case {hex(inst.key)}: this->decode_insttype_{inst.decoderName}(); this->do_{inst.instName}(this->gDecodeInfo); return &RVCore::do_{inst.instName};)\n"
        else:
            return f"        case {hex(inst.key)}: this->decode_insttype_{inst.decoderName}(); this->do_{inst.instName}(this->gDecodeInfo); return &RVCore::do_{inst.instName};\n"
    code  = "{\n"
    code += "    switch (inst & " + hex(key) + ") {\n"
    code += "".join(map(helper, instList))
    code += "    }\n"
    code += "}\n"

    return code


def gen_code_avx(instList: List[InstPattern], key) -> str: 
    inst32: List[InstPattern]= []
    inst64: List[InstPattern] = []
    instBoth: List[InstPattern] = []
    for inst in instList:
        if inst.instType == InstType.Only32:
            inst32.append(inst)
        elif inst.instType == InstType.Only64:
            inst64.append(inst)
        else:
            instBoth.append(inst)

    code  = "{\n"

    code += "    alignas(64) static constexpr uint32_t patterns[] = { \n"
    code += "".join(map(lambda inst: f"        {hex(inst.key)}, \n", instBoth))
    code += "        #ifdef KXEMU_ISA32\n"
    code += "".join(map(lambda inst: f"        {hex(inst.key)}, \n", inst32))
    code += "        #endif\n"
    code += "        #ifdef KXEMU_ISA64\n"
    code += "".join(map(lambda inst: f"        {hex(inst.key)}, \n", inst64))
    code += "        #endif\n"
    code += "        };\n"

    code += "    void (RVCore::*decoder_array[])() = { \n"
    code += "".join(map(lambda inst: f"        &RVCore::decode_insttype_{inst.decoderName}, \n", instBoth))
    code += "        #ifdef KXEMU_ISA32\n"
    code += "".join(map(lambda inst: f"        &RVCore::decode_insttype_{inst.decoderName}, \n", inst32))
    code += "        #endif\n"
    code += "        #ifdef KXEMU_ISA64\n"
    code += "".join(map(lambda inst: f"        &RVCore::decode_insttype_{inst.decoderName}, \n", inst64))
    code += "        #endif\n"
    code += "   };\n"

    code += "    void (RVCore::*do_inst_array[])(const DecodeInfo &){ \n"
    code += "".join(map(lambda inst: f"        &RVCore::do_{inst.instName}, \n", instBoth))
    code += "        #ifdef KXEMU_ISA32\n"
    code += "".join(map(lambda inst: f"        &RVCore::do_{inst.instName}, \n", inst32))
    code += "        #endif\n"
    code += "        #ifdef KXEMU_ISA64\n"
    code += "".join(map(lambda inst: f"        &RVCore::do_{inst.instName}, \n", inst64))
    code += "        #endif\n"
    code += "    };\n"

    code += f"    const __m256i vkey = _mm256_set1_epi32(inst & {key});\n"

    code += "    for(std::size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i += 8) {\n"
    code += "        __m256i vpatterns = _mm256_load_si256((__m256i*)(patterns + i));\n"
    code += "        __m256i cmp = _mm256_cmpeq_epi32(vkey, vpatterns);\n"
    code += "        uint32_t mask = _mm256_movemask_epi8(cmp);\n"
    code += "        \n"
    code += "        if (unlikely(mask != 0)) {\n"
    code += "            int bitpos = __builtin_ctz(mask);\n"
    code += "            int index = i + (bitpos / 4);\n"
    code += "            (this->*decoder_array[index])();\n"
    code += "            (this->*do_inst_array[index])(this->gDecodeInfo);\n"
    code += "            return do_inst_array[index];\n"
    code += "        }\n"
    code += "    }\n"
    code += "}\n"
                
    return code


def gen_code(tables: List[Dict[int, List[InstPattern]]]) -> str:
    code = """#ifdef KXEMU_ISA32
    #define __INST32(x) x
    #define __INST64(x) 
#else
    #define __INST32(x) 
    #define __INST64(x) x
#endif
"""
    for table in tables:
        for key in table:
            # if len(table[key]) >= 32:
            #     code += gen_code_avx(table[key], key)
            #     code += "\n"
                
            # else:
                code += gen_code_normal(table[key], key)
                code += "\n"

    code += """
return nullptr;

#undef __INST32
#undef __INST64
"""

    return code
