import argparse
import os

parser = argparse.ArgumentParser(description="Count lines of code.")
parser.add_argument("-d", "--dir", type=str, required=True, action="append", help="Input file")

args = parser.parse_args()
directories = args.dir

def is_python_file(filename):
    return filename.endswith(".py")

def is_cpp_file(filename):
    return filename.endswith(".cpp") or filename.endswith(".h")

def is_makefile(filename):
    return filename == "Makefile" or filename == "makefile" or filename.endswith(".mk")

filetypes = {
    "python": is_python_file,
    "cpp": is_cpp_file,
    "Makefile": is_makefile
}

fileCounter = {}

def count_lines_file(filename):
    counter = {
        "code": 0,
        "blank": 0,
        "comment": 0,
        "type": ""
    }

    t: int
    if is_python_file(filename):
        counter["type"] = "python"
        t = 0
    elif is_cpp_file(filename):
        counter["type"] = "cpp"
        t = 1
    elif is_makefile(filename):
        counter["type"] = "Makefile"
        t = 2
    else:
        counter["type"] = "other"
        t = 3

    with open(filename) as f:
        for line in f:
            line = line.strip()
            if line == "":
                counter["blank"] += 1
            
            if t == 0:
                if line.startswith("#"):
                    counter["comment"] += 1
                else:
                    counter["code"] += 1
            elif t == 1:
                if line.startswith("//"):
                    counter["comment"] += 1
                elif line.startswith("/*") or line.startswith("*"):
                    counter["comment"] += 1
                else:
                    counter["code"] += 1
            elif t == 2:
                if line.startswith("#"):
                    counter["comment"] += 1
                else:
                    counter["code"] += 1
            else:
                counter["code"] += 1

    fileCounter[filename] = counter


def count_lines_dir(d):
    for p in os.listdir(d):
        if os.path.isdir(d + "/" + p):
            count_lines_dir(d + "/" + p)
        else:
            count_lines_file(d + "/" + p)

for d in directories:
    count_lines_dir(d)

for filename, counter in fileCounter.items():
    print(f"{filename} {counter['type']} {counter['code']} {counter['blank']} {counter['comment']}")
