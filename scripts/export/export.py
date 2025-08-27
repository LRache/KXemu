import os
import re

config = {}

with open('configs/.config', 'r') as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        if line[0] == '#':
            continue
        key, value = line.split('=')
        if value[0] == '"' and value[-1] == '"':
            value = value[1:-1]
        config[key] = value

isaName = config['CONFIG_ISA']
baseISAName = config['CONFIG_BASE_ISA']

mkdir = lambda path: os.makedirs(path, exist_ok=True)

PATTERN = re.compile(r'#include\s*"((?!\./)[^"]+\.h)"')
def progress_file(src: str, dest: str):
    with open(src, 'r') as f:
        content = f.read()
    modified_content = PATTERN.sub(r'#include "kxemu/\1"', content)
    with open(dest, 'w') as f:
        f.write(modified_content)

def progress_dir(src: str, dest: str):
    mkdir(dest)
    for name in os.listdir(src):
        if os.path.isdir(src + '/' + name):
            progress_dir(src + '/' + name, dest + '/' + name)
        else:
            progress_file(src + '/' + name, dest + '/' + name)

destDir = 'export/' + isaName + '/include/kxemu/cpu'
srcDir = 'include/cpu'
mkdir(destDir)
for name in ("cpu.h", "core.h", "word.h"):
    progress_file(srcDir + '/' + name, destDir + '/' + name)
progress_dir(srcDir + '/' + baseISAName, destDir + '/' + baseISAName)

destDir = 'export/' + isaName + '/include/kxemu/device'
srcDir = 'include/device'
progress_dir(srcDir, destDir)

destDir = 'export/' + isaName + '/include/kxemu/config'
srcDir = 'include/config'
progress_dir(srcDir, destDir)

destDir = 'export/' + isaName + '/include/kxemu/utils'
srcDir = 'include/utils'
progress_dir(srcDir, destDir)

destDir = 'export/' + isaName + '/include/kxemu'
srcDir = 'include'
mkdir(destDir)
for name in ("macro.h", "log.h", "debug.h"):
    progress_file(srcDir + '/' + name, destDir + '/' + name)
