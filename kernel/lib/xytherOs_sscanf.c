#include <stdarg.h>
#include <core/defs.h>
#include <lib/ctype.h>
#include <string.h>
#include <xytherOs_math.h>

#define FLAG_SKIP      0x01  /* * assignment suppression */
#define FLAG_WIDTH     0x02  /* field width specified */
#define FLAG_LONG      0x04  /* l length modifier */
#define FLAG_SHORT     0x08  /* h length modifier */
#define FLAG_LONG_LONG 0x10  /* ll length modifier */

int xytherOs_vsscanf(const char *str, const char *fmt, va_list ap) {
    const char *p = fmt;
    const char *s = str;
    int count = 0;
    int flags;
    int width;
    int base;
    __unused int neg;

    while (*p && *s) {
        /* Skip whitespace in format */
        while (isspace(*p)) {
            p++;
        }

        if (*p != '%') {
            /* Match literal character */
            while (isspace(*s))
                s++;
            if (*s != *p) {
                return count;
            }
            p++;
            s++;
            continue;
        }

        p++;  /* skip '%' */

        /* Parse flags */
        flags = 0;
        width = 0;
        
        if (*p == '*') {
            flags |= FLAG_SKIP;
            p++;
        }

        /* Parse width */
        if (isdigit(*p)) {
            flags |= FLAG_WIDTH;
            width = 0;
            while (isdigit(*p)) {
                width = width * 10 + (*p - '0');
                p++;
            }
        }

        /* Parse length modifiers */
        if (*p == 'l') {
            p++;
            if (*p == 'l') {
                flags |= FLAG_LONG_LONG;
                p++;
            } else {
                flags |= FLAG_LONG;
            }
        } else if (*p == 'h') {
            flags |= FLAG_SHORT;
            p++;
        }

        /* Skip whitespace before input (except for %c, %[, %n) */
        if (*p != 'c' && *p != '[' && *p != 'n') {
            while (isspace(*s))
                s++;
            if (!*s) {
                return count;
            }
        }

        /* Handle conversion specifier */
        switch (*p) {
            case 'd': case 'i':
                base = 10;
                goto scan_int;
            case 'o':
                base = 8;
                goto scan_int;
            case 'x': case 'X':
                base = 16;
                goto scan_int;
            case 'u':
                base = 10;
                neg = 0;
                goto scan_unsigned;
            
            scan_int: {
                long long val = 0;
                int sign = 1;
                int digits = 0;
                
                /* Optional sign */
                if (*s == '-') {
                    sign = -1;
                    s++;
                } else if (*s == '+') {
                    s++;
                }
                
                /* Parse digits */
                while (*s) {
                    int digit;
                    
                    if (*s >= '0' && *s <= '9') {
                        digit = *s - '0';
                    } else if (base == 16) {
                        if (*s >= 'a' && *s <= 'f') {
                            digit = *s - 'a' + 10;
                        } else if (*s >= 'A' && *s <= 'F') {
                            digit = *s - 'A' + 10;
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                    
                    if (digit >= base) {
                        break;
                    }
                    
                    val = val * base + digit;
                    digits++;
                    s++;
                }
                
                if (digits == 0) {
                    return count;
                }
                
                val *= sign;
                
                if (!(flags & FLAG_SKIP)) {
                    if (flags & FLAG_LONG_LONG) {
                        *va_arg(ap, long long *) = val;
                    } else if (flags & FLAG_LONG) {
                        *va_arg(ap, long *) = (long)val;
                    } else if (flags & FLAG_SHORT) {
                        *va_arg(ap, short *) = (short)val;
                    } else {
                        *va_arg(ap, int *) = (int)val;
                    }
                    count++;
                }
                break;
            }
            
            scan_unsigned: {
                unsigned long long val = 0;
                int digits = 0;
                
                while (*s) {
                    int digit;
                    
                    if (*s >= '0' && *s <= '9') {
                        digit = *s - '0';
                    } else if (base == 16) {
                        if (*s >= 'a' && *s <= 'f') {
                            digit = *s - 'a' + 10;
                        } else if (*s >= 'A' && *s <= 'F') {
                            digit = *s - 'A' + 10;
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                    
                    if (digit >= base) {
                        break;
                    }
                    
                    val = val * base + digit;
                    digits++;
                    s++;
                }
                
                if (digits == 0) {
                    return count;
                }
                
                if (!(flags & FLAG_SKIP)) {
                    if (flags & FLAG_LONG_LONG) {
                        *va_arg(ap, unsigned long long *) = val;
                    } else if (flags & FLAG_LONG) {
                        *va_arg(ap, unsigned long *) = (unsigned long)val;
                    } else if (flags & FLAG_SHORT) {
                        *va_arg(ap, unsigned short *) = (unsigned short)val;
                    } else {
                        *va_arg(ap, unsigned int *) = (unsigned int)val;
                    }
                    count++;
                }
                break;
            }
            
            case 'f': case 'e': case 'g': {
                double val = 0.0;
                int sign = 1;
                int digits = 0;
                
                /* Optional sign */
                if (*s == '-') {
                    sign = -1;
                    s++;
                } else if (*s == '+') {
                    s++;
                }
                
                /* Integer part */
                while (isdigit(*s)) {
                    val = val * 10.0 + (*s - '0');
                    digits++;
                    s++;
                }
                
                /* Fractional part */
                if (*s == '.') {
                    double frac = 0.1;
                    s++;
                    while (isdigit(*s)) {
                        val += (*s - '0') * frac;
                        frac *= 0.1;
                        digits++;
                        s++;
                    }
                }
                
                /* Exponent */
                if (toupper(*s) == 'E') {
                    int exp_sign = 1;
                    int exponent = 0;
                    
                    s++;
                    if (*s == '-') {
                        exp_sign = -1;
                        s++;
                    } else if (*s == '+') {
                        s++;
                    }
                    
                    while (isdigit(*s)) {
                        exponent = exponent * 10 + (*s - '0');
                        s++;
                    }
                    
                    exponent *= exp_sign;
                    val *= xytherOs_pow(10.0, exponent);
                }
                
                if (digits == 0) {
                    return count;
                }
                
                val *= sign;
                
                if (!(flags & FLAG_SKIP)) {
                    if (flags & FLAG_LONG) {
                        *va_arg(ap, double *) = val;
                    } else {
                        *va_arg(ap, float *) = (float)val;
                    }
                    count++;
                }
                break;
            }
            
            case 'c': {
                if (flags & FLAG_WIDTH) {
                    int i;
                    if (!(flags & FLAG_SKIP)) {
                        char *cp = va_arg(ap, char *);
                        for (i = 0; i < width && *s; i++) {
                            cp[i] = *s++;
                        }
                        count++;
                    } else {
                        for (i = 0; i < width && *s; i++) {
                            s++;
                        }
                    }
                } else {
                    if (!*s) {
                        return count;
                    }
                    if (!(flags & FLAG_SKIP)) {
                        *va_arg(ap, char *) = *s;
                        count++;
                    }
                    s++;
                }
                break;
            }
            
            case 's': {
                /* Skip leading whitespace */
                while (isspace(*s))
                    s++;
                if (!*s) {
                    return count;
                }
                
                if (!(flags & FLAG_SKIP)) {
                    char *sp = va_arg(ap, char *);
                    int i = 0;
                    
                    if (flags & FLAG_WIDTH) {
                        while (i < width && *s && !isspace(*s)) {
                            sp[i++] = *s++;
                        }
                    } else {
                        while (*s && !isspace(*s)) {
                            sp[i++] = *s++;
                        }
                    }
                    sp[i] = '\0';
                    count++;
                } else {
                    int i = 0;
                    if (flags & FLAG_WIDTH) {
                        while (i < width && *s && !isspace(*s)) {
                            s++;
                            i++;
                        }
                    } else {
                        while (*s && !isspace(*s)) {
                            s++;
                        }
                    }
                }
                break;
            }
            
            case '[': {
                char scan_set[256] = {0};
                int negate = 0;
                
                p++;  /* skip '[' */
                
                /* Check for negation */
                if (*p == '^') {
                    negate = 1;
                    p++;
                }
                
                /* Build scan set */
                if (*p == ']') {
                    scan_set[(unsigned char)*p] = 1;
                    p++;
                }
                
                while (*p && *p != ']') {
                    if (*p == '-' && p[1] && p[1] != ']') {
                        /* Range specification */
                        for (int c = (unsigned char)p[-1]; c <= (unsigned char)p[1]; c++) {
                            scan_set[c] = 1;
                        }
                        p += 2;
                    } else {
                        scan_set[(unsigned char)*p] = 1;
                        p++;
                    }
                }
                
                if (*p != ']') {
                    /* Malformed format string */
                    return count;
                }
                p++;
                
                /* Skip leading whitespace */
                while (isspace(*s))
                    s++;
                if (!*s) {
                    return count;
                }
                
                if (!(flags & FLAG_SKIP)) {
                    char *sp = va_arg(ap, char *);
                    int i = 0;
                    
                    if (flags & FLAG_WIDTH) {
                        while (i < width && *s && 
                               (negate ? !scan_set[(unsigned char)*s] : scan_set[(unsigned char)*s])) {
                            sp[i++] = *s++;
                        }
                    } else {
                        while (*s && (negate ? !scan_set[(unsigned char)*s] : scan_set[(unsigned char)*s])) {
                            sp[i++] = *s++;
                        }
                    }
                    sp[i] = '\0';
                    count++;
                } else {
                    int i = 0;
                    if (flags & FLAG_WIDTH) {
                        while (i < width && *s && 
                               (negate ? !scan_set[(unsigned char)*s] : scan_set[(unsigned char)*s])) {
                            s++;
                            i++;
                        }
                    } else {
                        while (*s && (negate ? !scan_set[(unsigned char)*s] : scan_set[(unsigned char)*s])) {
                            s++;
                        }
                    }
                }
                break;
            }
            
            case 'n': {
                if (!(flags & FLAG_SKIP)) {
                    *va_arg(ap, int *) = (int)(s - str);
                }
                break;
            }
            
            case '%': {
                if (*s != '%') {
                    return count;
                }
                s++;
                break;
            }
            
            default:
                /* Unknown format specifier */
                return count;
        }
        
        p++;
    }
    
    return count;
}

int xytherOs_sscanf(const char *str, const char *fmt, ...) {
    va_list ap;
    int count;
    
    va_start(ap, fmt);
    count = xytherOs_vsscanf(str, fmt, ap);
    va_end(ap);
    
    return count;
}