from enum import Enum
from typing import List

class InstType(Enum):
    Only32 = 0
    Only64 = 1
    Both = 2

class InstPattern():
    def __init__(self, pattern: str, instName: str, instType: InstType):
        self.instName = instName
        self.instType = instType

        mask = 0
        length = 0
        key = 0
        for c in pattern:
            if c == ' ':
                continue
            
            length += 1
            mask <<= 1
            key <<= 1
            if c == '0':
                mask |= 1
            elif c == '1':
                mask |= 1
                key |= 1
            elif c in '?xX':
                pass
            else:
                raise ValueError(f"Invalid character '{c}' in pattern '{pattern}'")
        
        self.mask = mask
        self.length = length
        self.key = key

decodeTable = {}
instGroup: List[List[InstPattern]]
__currentGroup = 0

def new_decoder_table(name: str):
    global instGroup
    decodeTable[name] = []
    instGroup = decodeTable[name]

def INSTPAT(pattern: str, instName: str, decoderName: str, instType: int = 0):
    if len(instGroup) == 0:
        instGroup.append(list())
    instType = {32: InstType.Only32, 64: InstType.Only64, 0: InstType.Both}
    instGroup[__currentGroup].append(InstPattern(pattern, instName, instType))

def new_group():
    __currentGroup += 1
    instGroup.append(list())

def get_decode_table(name: str):
    return decodeTable[name]
