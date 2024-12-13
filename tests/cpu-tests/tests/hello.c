#include <klib-macros.h>
#include <am.h>

char str1[] = "Hello World! from riscv32e-ysyxsoc\n";
char str2[] = "Hello World! from riscv32e-ysyxsoc\n";

int main(const char *args) {
    // const char *str = ;
    putstr(str1);
    putstr(str2);
    return 0;
}
