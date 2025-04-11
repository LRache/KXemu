include ./scripts/config.mk

all: kxemu

clean:
	$(info + CLEAN ./build)
	@ rm -rf ./build
	$(info + CLEAN ./export)
	@ rm -rf ./export

count:
	$(info Counting lines in src and include directories...)
	@ find ./src ./include ./scripts -name '*.c' -or -name "*.cpp" -or -name "*.h" -or -name "*.py" -or -name "*.instpat" -or -name "*.cmake" | xargs cat | sed '/^\s*$$/d' | wc -l

kxemu:
	@ mkdir -p ./build
	@ cd ./build && cmake .. -DTARGET_APP=ON -DTARGET_LIB=OFF && cmake --build .

export:
	@ mkdir -p ./build
	@ mkdir -p ./export
	@ cd ./build && cmake .. -DTARGET_LIB=ON -DTARGET_APP=OFF && cmake --build .
	$(info + Exporting Headers $(ISA))
	@ python3 ./scripts/export/export.py

.PHONY: kxemu export run clean all
.DEFAULT_GOAL := all
