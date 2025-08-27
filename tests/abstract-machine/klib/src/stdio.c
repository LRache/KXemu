#include "am.h"
#include "klib.h"
#include <stdarg.h>

typedef struct FmtBuffer {
    char *buffer;
    size_t cnt;
    void (*write)(struct FmtBuffer *buffer, char c);
} FmtBuffer;

static void out_buffer_init(FmtBuffer *buffer, char *out);
static void putch_buffer_init(FmtBuffer *buffer);

static void write_buffer_out   (FmtBuffer *buffer, char c);
static void write_buffer_putch (FmtBuffer *buffer, char c);

typedef void (*fmt_fun_t)(FmtBuffer *buffer, va_list*, char*);

static void __fmt_s   (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_d   (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_ld  (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_lld (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_u   (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_lu  (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_llu (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_x   (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_lx  (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_llx (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_X   (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_p   (FmtBuffer *buffer, va_list *ap, char *arg);
static void __fmt_llu (FmtBuffer *buffer, va_list *ap, char *arg);


typedef struct {
    char fmt[10];
    int length;
    fmt_fun_t fun;
} FmtEntry;

static FmtEntry fmtTable[] = {
  {"s"   , 1, __fmt_s},
  {"d"   , 1, __fmt_d},
  {"ld"  , 2, __fmt_ld},
  {"lld" , 3, __fmt_lld},
  {"u"   , 1, __fmt_u},
  {"lu"  , 2, __fmt_lu},
  {"llu" , 3, __fmt_llu},
  {"x"   , 1, __fmt_x},
  {"lx"  , 2, __fmt_lx},
  {"llx" , 3, __fmt_llx},
  {"X"   , 1, __fmt_X},
  {"p"   , 1, __fmt_p},
};

#define FMT_TABLE_LEN (sizeof(fmtTable) / sizeof(fmtTable[0]))

static int __vsprintf(FmtBuffer *buffer, const char *fmt, va_list ap);

static void out_buffer_init(FmtBuffer *buffer, char *out) {
    buffer->buffer = out;
    buffer->cnt = 0;
    buffer->write = write_buffer_out;
}

static void putch_buffer_init(FmtBuffer *buffer) {
    buffer->buffer = NULL;
    buffer->cnt = 0;
    buffer->write = write_buffer_putch;
}

static void write_buffer_out(FmtBuffer *buffer, char c) {
    buffer->buffer[buffer->cnt++] = c;
}

static void write_buffer_putch(FmtBuffer *buffer, char c) {
    putchar(c);
}

static void __fmt_s(FmtBuffer *buffer, va_list *ap, char *arg) {
    char *s = va_arg(*ap, char*);
    while (*s) buffer->write(buffer, *(s++));
}

static void __fmt_d(FmtBuffer *buffer, va_list *ap, char *arg) {
    int d = va_arg(*ap, int);
    int leftAlign = 0;
    int minWidth = 0;
    int zeroFill = 0;
    if (*arg != 0) {
        if (*arg == '0') {
            zeroFill = 1;
            arg++;
        }
        if (*arg == '-') {
            leftAlign = 1;
            arg++;
        } 
        while (*arg) {
            minWidth = minWidth * 10 + *arg - '0';
            arg++;
        }
    } 
    int sign = d < 0;
    if (sign) d = -d;
    
    char stack[13] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
    } else {
        while (d) {
            *(t++) = d % 10 + '0';
            d = d / 10;
        }
    }

    int width = t - stack;
    char *h = stack;
    if (width < minWidth) {
        if (leftAlign) {
        while (t > h) buffer->write(buffer, *(--t));
        while (width < minWidth) {
            buffer->write(buffer, ' ');
            width ++;
        }
        } else {
        char fill = zeroFill ? '0' : ' ';
        while (width < minWidth) {
            buffer->write(buffer, fill);
            width++;
        }
        while (t > h) buffer->write(buffer, *(--t));
        }
    } else {
        if (sign) buffer->write(buffer, '-');
        while (t > h) buffer->write(buffer, *(--t));
    }
}

static void __fmt_ld(FmtBuffer *buffer, va_list *ap, char *arg) {
    long d = va_arg(*ap, long);
    int leftAlign = 0;
    int minWidth = 0;
    int zeroFill = 0;
    if (*arg != 0) {
        if (*arg == '0') {
            zeroFill = 1;
            arg++;
        }
        if (*arg == '-') {
            leftAlign = 1;
            arg++;
        } 
        while (*arg) {
            minWidth = minWidth * 10 + *arg - '0';
            arg++;
        }
    } 
    int sign = d < 0;
    if (sign) d = -d;
    
    char stack[21] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
    } else {
        while (d) {
            *(t++) = d % 10 + '0';
            d = d / 10;
        }
    }

    int width = t - stack;
    char *h = stack;
    if (width < minWidth) {
        if (leftAlign) {
        while (t > h) buffer->write(buffer, *(--t));
        while (width < minWidth) {
            buffer->write(buffer, ' ');
            width ++;
        }
        } else {
        char fill = zeroFill ? '0' : ' ';
        while (width < minWidth) {
            buffer->write(buffer, fill);
            width++;
        }
        while (t > h) buffer->write(buffer, *(--t));
        }
    } else {
        if (sign) buffer->write(buffer, '-');
        while (t > h) buffer->write(buffer, *(--t));
    }
}

static void __fmt_lld(FmtBuffer *buffer, va_list *ap, char *arg) {
    long long d = va_arg(*ap, long long);
    int leftAlign = 0;
    int minWidth = 0;
    int zeroFill = 0;
    if (*arg != 0) {
        if (*arg == '0') {
        zeroFill = 1;
        arg++;
        }
        if (*arg == '-') {
        leftAlign = 1;
        arg++;
        } 
        while (*arg) {
        minWidth = minWidth * 10 + *arg - '0';
        arg++;
        }
    } 
    int sign = d < 0;
    if (sign) d = -d;
    
    char stack[21] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
    } else {
        while (d) {
            *(t++) = d % 10 + '0';
            d = d / 10;
        }
    }

    int width = t - stack;
    char *h = stack;
    if (width < minWidth) {
        if (leftAlign) {
        while (t > h) buffer->write(buffer, *(--t));
        while (width < minWidth) {
            buffer->write(buffer, ' ');
            width ++;
        }
        } else {
        char fill = zeroFill ? '0' : ' ';
        while (width < minWidth) {
            buffer->write(buffer, fill);
            width++;
        }
        while (t > h) buffer->write(buffer, *(--t));
        }
    } else {
        if (sign) buffer->write(buffer, '-');
        while (t > h) buffer->write(buffer, *(--t));
    }
}

static void __fmt_u(FmtBuffer *buffer, va_list *ap, char *arg) {
    unsigned int d = va_arg(*ap, unsigned int);
    int leftAlign = 0;
    int minWidth = 0;
    int zeroFill = 0;
    if (*arg != 0) {
        if (*arg == '0') {
            zeroFill = 1;
            arg++;
        }
        if (*arg == '-') {
            leftAlign = 1;
            arg++;
        } 
        while (*arg) {
            minWidth = minWidth * 10 + *arg - '0';
            arg++;
        }
    } 
    
    char stack[13] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
    } else {
        while (d) {
            *(t++) = d % 10 + '0';
            d = d / 10;
        }
    }

    int width = t - stack;
    char *h = stack;
    if (width < minWidth) {
        if (leftAlign) {
            while (t > h) buffer->write(buffer, *(--t));
            while (width < minWidth) {
                buffer->write(buffer, ' ');
                width ++;
            }
        } else {
            char fill = zeroFill ? '0' : ' ';
            while (width < minWidth) {
                buffer->write(buffer, fill);
                width++;
            }
            while (t > h) buffer->write(buffer, *(--t));
        }
    } else {
        while (t > h) buffer->write(buffer, *(--t));
    }
}

static void __fmt_lu(FmtBuffer *buffer, va_list *ap, char *arg) {
    unsigned long d = va_arg(*ap, unsigned long);
    int leftAlign = 0;
    int minWidth = 0;
    int zeroFill = 0;
    if (*arg != 0) {
        if (*arg == '0') {
            zeroFill = 1;
            arg++;
        }
        if (*arg == '-') {
            leftAlign = 1;
            arg++;
        } 
        while (*arg) {
            minWidth = minWidth * 10 + *arg - '0';
            arg++;
        }
    } 
    
    char stack[21] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
    } else {
        while (d) {
            *(t++) = d % 10 + '0';
            d = d / 10;
        }
    }

    int width = t - stack;
    char *h = stack;
    if (width < minWidth) {
        if (leftAlign) {
            while (t > h) buffer->write(buffer, *(--t));
            while (width < minWidth) {
                buffer->write(buffer, ' ');
                width ++;
            }
        } else {
            char fill = zeroFill ? '0' : ' ';
            while (width < minWidth) {
                buffer->write(buffer, fill);
                width++;
            }
            while (t > h) buffer->write(buffer, *(--t));
        }
    } else {
        while (t > h) buffer->write(buffer, *(--t));
    }
}

static void __fmt_llu(FmtBuffer *buffer, va_list *ap, char *arg) {
    unsigned long long d = va_arg(*ap, unsigned long long);
    int leftAlign = 0;
    int minWidth = 0;
    int zeroFill = 0;
    if (*arg != 0) {
        if (*arg == '0') {
            zeroFill = 1;
            arg++;
        }
        if (*arg == '-') {
            leftAlign = 1;
            arg++;
        } 
        while (*arg) {
            minWidth = minWidth * 10 + *arg - '0';
            arg++;
        }
    } 
    
    char stack[21] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
    } else {
        while (d) {
            *(t++) = d % 10 + '0';
            d = d / 10;
        }
    }

    int width = t - stack;
    char *h = stack;
    if (width < minWidth) {
        if (leftAlign) {
            while (t > h) buffer->write(buffer, *(--t));
            while (width < minWidth) {
                buffer->write(buffer, ' ');
                width ++;
            }
        } else {
            char fill = zeroFill ? '0' : ' ';
            while (width < minWidth) {
                buffer->write(buffer, fill);
                width++;
            }
            while (t > h) buffer->write(buffer, *(--t));
        }
    } else {
        while (t > h) buffer->write(buffer, *(--t));
    }
}

static void __fmt_hex(FmtBuffer *buffer, va_list *ap, char *arg, char base, int width, char fill) {
    unsigned int d = va_arg(*ap, unsigned int);
    
    char stack[9] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
        width--;
    } else {
        while (d) {
            int x = d % 16;
            if (x > 9) *(t++) = x - 10 + base;
            else *(t++) = x + '0';
            d = d / 16;
            width--;
        }
    }

    for (int i = width; i > 0; i--) buffer->write(buffer, fill);
    char *h = stack;
    while (t > h) buffer->write(buffer, *(--t)); 
}

static void __fmt_hex_l(FmtBuffer *buffer, va_list *ap, char *arg, char base, int width, char fill) {
    unsigned long d = va_arg(*ap, unsigned long);
    
    char stack[17] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
        width--;
    } else {
        while (d) {
            int x = d % 16;
            if (x > 9) *(t++) = x - 10 + base;
            else *(t++) = x + '0';
            d = d / 16;
            width--;
        }
    }

    for (int i = width; i > 0; i--) buffer->write(buffer, fill);
    char *h = stack;
    while (t > h) buffer->write(buffer, *(--t)); 
}

static void __fmt_hex_ll(FmtBuffer *buffer, va_list *ap, char *arg, char base, int width, char fill) {
    unsigned long long d = va_arg(*ap, unsigned long long);
    
    char stack[17] = {};
    char *t = stack;
    if (d == 0) {
        *(t++) = '0';
        width--;
    } else {
        while (d) {
            int x = d % 16;
            if (x > 9) *(t++) = x - 10 + base;
            else *(t++) = x + '0';
            d = d / 16;
            width--;
        }
    }

    for (int i = width; i > 0; i--) buffer->write(buffer, fill);
    char *h = stack;
    while (t > h) buffer->write(buffer, *(--t)); 
}

static void __fmt_x(FmtBuffer *buffer, va_list *ap, char *arg) {
    __fmt_hex(buffer, ap, arg, 'a', -1, 0);
}

static void __fmt_lx(FmtBuffer *buffer, va_list *ap, char *arg) {
    __fmt_hex_l(buffer, ap, arg, 'a', -1, 0);
}

static void __fmt_llx(FmtBuffer *buffer, va_list *ap, char *arg) {
    __fmt_hex_ll(buffer, ap, arg, 'a', -1, 0);
}

static void __fmt_X(FmtBuffer *buffer, va_list *ap, char *arg) {
    __fmt_hex(buffer, ap, arg, 'A', -1, 0);
}

static void __fmt_p(FmtBuffer *buffer, va_list *ap, char *arg) {
    buffer->write(buffer, '0');
    buffer->write(buffer, 'x');
    __fmt_hex(buffer, ap, arg, 'a', 8, '0');
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    FmtBuffer buffer;
    putch_buffer_init(&buffer);
    int r = __vsprintf(&buffer, fmt, ap);
    va_end(ap);
    return r;
}

int vsprintf(char *str, const char *format, va_list ap) {
    FmtBuffer buffer;
    out_buffer_init(&buffer, str);
    return __vsprintf(&buffer, format, ap);
}

static int __vsprintf(FmtBuffer *buffer, const char *fmt, va_list ap) {
    char *p = (char*) fmt;
    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == '%') {
                buffer->write(buffer, '%');
                continue;
            }

            int arglen = 0;
            char arg[10] = {};
            for (arglen = 0; (!isalpha(*p)) && *p; arglen++, p++) arg[arglen] = *p;
            
            for (int i = 0; i < FMT_TABLE_LEN; i++) {
                if (strncmp(p, fmtTable[i].fmt, fmtTable[i].length) == 0) {
                    fmtTable[i].fun(buffer, &ap, arg);
                    p += fmtTable[i].length;
                    break;
                } 
            }
        } else {
            buffer->write(buffer, *(p++));
        }
    }
    buffer->write(buffer, 0);
    return buffer->cnt;
    }

int sprintf(char *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int result = vsprintf(out, fmt, ap);
    va_end(ap);
    return result;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int result = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return result;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
    // TODO: implement this function
    return 0;
}

int puts(const char *s) {
    while (*s) putchar(*(s++));
    putchar('\n');
    return 0;
}
