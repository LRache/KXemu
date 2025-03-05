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
        
        pattern = m.group(1)
        name = m.group(2)
        arg1 = m.group(3)
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
        name = name.replace(".", "_")
        group.append(InstPattern(pattern, name, arg1, t))
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


# def gen_code_avx(instList: List[InstPattern], Ins) -> str:
#     code = ""
#     code += "{\n"
#     code += __attribute__((aligned(64))) static constexpr uint32_t patterns = 


def gen_code(tables: List[Dict[int, List[InstPattern]]], format: str) -> str:
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
            if len(table[key]) >= 32:

                code += "{"
                code += f"__attribute__((aligned(64))) static constexpr uint32_t patterns[] = "
                code += "\n{"
                for inst in table[key]:
                    if inst.t == InstType.Only32:
                        code += "#ifdef KXEMU_ISA32\n"
                    elif inst.t == InstType.Only64:
                        code += "#ifdef KXEMU_ISA64\n"
                    
                    code += f"{inst.key},\n"
                    
                    if inst.t != InstType.Both:
                        code += "#endif\n"
                     
                code += "};\n"

                code += f"const __m256i __key=_mm256_set1_epi32(inst & {key});\n"
                code += f"void (RVCore::*__fun_0[])()"
                code += "{\n"
                for inst in table[key]:
                    if inst.t == InstType.Only32:
                        code += "#ifdef KXEMU_ISA32\n"
                    elif inst.t == InstType.Only64:
                        code += "#ifdef KXEMU_ISA64\n"
                    
                    code += f"&RVCore::decode_insttype_{inst.arg1},\n"
                    
                    if inst.t != InstType.Both:
                        code += "#endif\n"
                code += "};\n"
                
                code += f"void (RVCore::*__fun_1[])(const DecodeInfo &)  = "
                code += "{\n"
                for inst in table[key]:
                    if inst.t == InstType.Only32:
                        code += "#ifdef KXEMU_ISA32\n"
                    elif inst.t == InstType.Only64:
                        code += "#ifdef KXEMU_ISA64\n"
                        
                    code += f"&RVCore::do_{inst.arg0},\n"

                    if inst.t != InstType.Both:
                        code += "#endif\n"
                
                code += "};\n"
                
                code += "for(std::size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i += 8) "
                code += "{\n"
                code += "__m256i v_patterns = _mm256_load_si256((__m256i*)(patterns + i));\n"
                code += "__m256i cmp = _mm256_cmpeq_epi32(__key, v_patterns);\n"
                code += "uint32_t mask = _mm256_movemask_epi8(cmp);\n"
                code += "if (unlikely(mask != 0)) {\n"
                code += "int bitpos = __builtin_ctz(mask);\n"
                code += "int index = i + (bitpos / 4);\n"
                code += "(this->*__fun_0[index])();\n"
                code += "(this->*__fun_1[index])(this->gDecodeInfo);\n"
                code += "return __fun_1[index];\n"
                code += "}\n}\n"

                code += "}\n"
            else:
                code += "switch (inst & " + hex(key) + ") {\n"
                for inst in table[key]:
                    s = format.replace("{name}", inst.arg0)
                    s = s.replace("{arg1}", inst.arg1)
                    if inst.t == InstType.Only32:
                        code += f"    __INST32(case {hex(inst.key)}: {s})\n"
                    elif inst.t == InstType.Only64:
                        code += f"    __INST64(case {hex(inst.key)}: {s})\n"
                    else:
                        code += f"    case {hex(inst.key)}: {s}\n"
                code += "}\n"

    code += """
return nullptr;

#undef __INST32
#undef __INST64
"""

    return code
