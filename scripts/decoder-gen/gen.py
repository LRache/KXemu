import re
from typing import List, Dict
from instpat import *

INST_PATTERN = re.compile(r'INSTPAT\s*\(\s*"([^"]+)"\s*,\s*([^,\s]+)\s*(?:,\s*(\d+))?\s*\)\s*;')

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
        t = m.group(3)

        if t is None:
            t = InstType.Both
        else:
            if t == "32":
                t = InstType.Only32
            elif t == "64":
                t = InstType.Only64
            else:
                raise ValueError(f"Invalid type '{t}'")
        group.append(InstPattern(pattern, name, t))
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


def gen_code(tables: List[Dict[int, List[InstPattern]]], functionPrefix: str, instType: str = "uint32_t") -> str:
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
            code += "switch (inst & " + hex(key) + ") {\n"
            for inst in table[key]:
                if inst.t == InstType.Only32:
                    code += f"    __INST32(case {hex(inst.key)}: {functionPrefix}{inst.name}(); return true;)\n"
                elif inst.t == InstType.Only64:
                    code += f"    __INST64(case {hex(inst.key)}: {functionPrefix}{inst.name}(); return true;)\n"
                else:
                    code += f"    case {hex(inst.key)}: {functionPrefix}{inst.name}(); return true;\n"
            code += "}\n"

    code += """
return false;

#undef __INST32
#undef __INST64
"""

    return code
