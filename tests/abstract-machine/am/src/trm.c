#ifdef MAINARGS
    const char *mainargs = "" MAINARGS "";
#else
    const char *mainargs = "";
#endif

extern int main(const char *mainargs);

extern void __trap(int code);

__attribute__((noreturn))
void halt(int code) {
    __trap(code);
    while(1);
}

void __run_main() {
    int r = main(mainargs);
    halt(r);
}

int putchar(int c) {
    return 0;
}


