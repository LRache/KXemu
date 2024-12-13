#ifdef MAINARGS
    const char *mainargs = "" MAINARGS "";
#else
    const char *mainargs = "";
#endif

extern int main(const char *mainargs);

__attribute__((noreturn))
extern void halt(int code);

void __run_main() {
    int r = main(mainargs);
    halt(r);
}
