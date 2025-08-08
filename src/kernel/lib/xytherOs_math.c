#include <stdint.h>

// Custom frexp: splits x into mantissa in [0.5, 1) and exponent such that x = mantissa * 2^exponent
double xytherOs_frexp(double x, int *exponent) {
    if (x == 0.0) {
        *exponent = 0;
        return 0.0;
    }

    union {
        double d;
        uint64_t u;
    } v = { x };

    int exp_raw = (int)((v.u >> 52) & 0x7FF);
    *exponent = exp_raw - 1022; // Bias is 1023; we use 1022 to make mantissa ∈ [0.5, 1)

    v.u &= ~(0x7FFULL << 52);              // Clear exponent bits
    v.u |= ((uint64_t)1022 << 52);         // Set exponent to 1022

    return v.d;
}

// Custom exp(x): computes e^x using Horner's method
double xytherOs_exp(double x) {
    if (x < -20.0) return 0.0;          // underflow guard
    if (x > 20.0) return 1.0 / 0.0;     // overflow guard

    double sum = 1.0;
    for (int i = 10; i > 0; --i) {
        sum = 1.0 + x * sum / i;
    }
    return sum;
}

// Custom log(x): computes ln(x) using log1p-style series and frexp normalization
double xytherOs_log(double x) {
    if (x <= 0.0) return -1.0 / 0.0;  // return -INF for x <= 0

    int e;
    double m = xytherOs_frexp(x, &e); // x = m * 2^e, m ∈ [0.5, 1)

    double y = (m - 1.0) / (m + 1.0);
    double y2 = y * y;

    double term = y;
    double sum = 0.0;

    for (int i = 1; i <= 13; i += 2) {
        sum += term / i;
        term *= y2;
    }

    double ln2 = 0.69314718056;
    return 2.0 * sum + e * ln2;
}

// Custom pow(base, exponent): computes base^exponent using exp and log
double xytherOs_pow(double base, double exponent) {
    if (base <= 0.0) return 0.0; // crude handling, no support for complex numbers
    return xytherOs_exp(exponent * xytherOs_log(base));
}
