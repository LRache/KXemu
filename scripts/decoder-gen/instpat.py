from enum import Enum

class InstType(Enum):
    Only32 = 0
    Only64 = 1
    Both = 2

class InstPattern():
    def __init__(self, pattern: str, name: str, t: InstType):
        self.name = name
        self.t = t

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
