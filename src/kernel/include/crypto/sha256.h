#pragma once

#include <core/types.h>
#include <string.h>
#include "../../crypto/SHA256/sha256.h"

static inline size_t sha256_index(const char *key, size_t limit) {
    uint8_t hash[SHA256_SIZE_BYTES];
    sha256(key, strlen(key), hash);

    // Combine first few bytes into an integer
    size_t idx = 0;
    for (usize i = 0; i < sizeof(size_t); i++) {
        idx = (idx << 8) | hash[i];
    }

    return idx % limit;
}
