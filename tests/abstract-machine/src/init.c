#ifdef MAINARGS
    const char *mainargs = "" MAINARGS "";
#else
    const char *mainargs = "";
#endif

extern int main(const char *mainargs);

int __init() {
    int r = main(mainargs);
    return r;
}
