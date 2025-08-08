#include <limits.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <xytherOs_string.h>

/**
 * Custom atoi implementation for kernel space
 */
int __xytherOs_atoi(const char *str) {
    int result = 0;
    int sign = 1;
    
    /* Handle leading whitespace */
    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }
    
    /* Handle optional sign */
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    /* Convert digits to integer */
    while (*str >= '0' && *str <= '9') {
        /* Check for overflow */
        if (result > (INT_MAX / 10) || 
            (result == (INT_MAX / 10) && (*str - '0') > (INT_MAX % 10))) {
            return sign == 1 ? INT_MAX : INT_MIN;
        }
        
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

/**
 * Find first occurrence of character in string
 */
char *__xytherOs_strchr(const char *s, int c) {
    while (*s != '\0') {
        if (*s == c) return (char *)s;
        s++;
    }
    return NULL;
}

char *__xytherOs_strtok_r(char *str, const char *delim, char **saveptr) {
    char *end;
    if (str == NULL) {
        str = *saveptr;
    }
    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }
    
    /* Skip leading delimiters */
    str += __xytherOs_strspn(str, delim);
    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }
    
    /* Find end of token */
    end = str + __xytherOs_strcspn(str, delim);
    if (*end == '\0') {
        *saveptr = end;
        return str;
    }
    
    /* Terminate token and set saveptr */
    *end = '\0';
    *saveptr = end + 1;
    return str;
}

size_t __xytherOs_strspn(const char *s, const char *accept) {
    const char *p;
    const char *a;
    size_t count = 0;

    for (p = s; *p != '\0'; ++p) {
        for (a = accept; *a != '\0'; ++a) {
            if (*p == *a)
                break;
        }
        if (*a == '\0')
            return count;
        ++count;
    }
    return count;
}

size_t __xytherOs_strcspn(const char *s, const char *reject) {
    size_t count = 0;
    const char *p, *r;

    for (p = s; *p != '\0'; ++p) {
        for (r = reject; *r != '\0'; ++r) {
            if (*p == *r)
                return count;
        }
        ++count;
    }
    return count;
}


/* String tokenizer implementation */
int __xytherOs_tokenize_r(char *s, const char *delim, char **tokens, size_t max_tokens, size_t *ptokcnt) {
    size_t  count = 0;
    char    *token, *saveptr = s;

    if (!s || !delim || !tokens || !max_tokens) {
        return -EINVAL;
    }

    token = __xytherOs_strtok_r(s, delim, &saveptr);
    while (token != NULL && (count < (max_tokens - 1))) {
        tokens[count] = xytherOs_strdup(token);
        if (tokens[count] == NULL) {
            /* Cleanup on allocation failure */
            while (count > 0) {
                count--;
                kfree(tokens[count]);
            }
            return -ENOMEM;
        }
        count++;
        token = __xytherOs_strtok_r(NULL, delim, &saveptr);
    }
    tokens[count] = NULL; /* NULL terminate array */

    if (ptokcnt) {
        *ptokcnt = count;
    }

    return 0;
}

void __xytherOs_tokens_free(char **tokens) {
    if (tokens == NULL) return;
    
    for (char **p = tokens; *p != NULL; p++) {
        kfree(*p);
    }
}