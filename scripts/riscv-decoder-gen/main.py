import argparse

from gen import *

def main():
    parser = argparse.ArgumentParser(description="Generate instruction decode switch-case table from .instpat file")
    parser.add_argument("-i", "--input", type=str, required=True, help="Input .instpat file")
    parser.add_argument("-o", "--output", type=str, required=True)

    args = parser.parse_args()
    inputFile = args.input
    outputFile = args.output
    
    groups = read_file(inputFile)
    tables = build_decode_table(groups)
    code = gen_code(tables)
    
    with open(outputFile, 'w') as f:
        f.write(code)

if __name__ == "__main__":
    main()
