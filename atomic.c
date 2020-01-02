#include <stdint.h>

char *cdef_string =
    "void clear(uint32_t *);"
    "uint32_t decrement(uint32_t *);"
    "uint32_t increment(uint32_t *);"
    ;

uint32_t decrement(uint32_t *atom)
{
    return __atomic_fetch_sub(atom, 1, __ATOMIC_SEQ_CST);
}

uint32_t increment(uint32_t *atom)
{
    return __atomic_add_fetch(atom, 1, __ATOMIC_SEQ_CST);
}


void clear(uint32_t *atom)
{
    uint32_t zero = 0;
    __atomic_store(atom, &zero, __ATOMIC_SEQ_CST);
}
