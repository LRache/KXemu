// Minimal static user program used to verify that KernelX reaches userspace
// under KXemu. It relies only on write(2) and exit(2).
#include <unistd.h>

int main(void) {
    static const char message[] = "Hello from KernelX userspace on KXemu!\n";
    write(1, message, sizeof(message) - 1);
    return 0;
}
