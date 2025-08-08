#include <mm/kalloc.h>
#include <xytherOs_string.h>

/**
 * @brief Allocate and zero-out memory.
 * 
 * @param size size of memory buffer.
 * @return void* NULL if allocation failed, otherwise, a pointer to the allocated memory.
 */
void *kzalloc(size_t size) {
    void *data = kmalloc(size);
    if (data) { // xytherOs_memset() is a fast version of memset.
        xytherOs_memset(data, 0, size);
    }
    return data;
}