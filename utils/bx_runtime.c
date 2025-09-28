#include <stdio.h>
#include <stdint.h>

/* Note: TAC int == C int64_t
This is because C int is usually only 32 bits. */

void bx_print_int(int64_t x)
{
    printf("%ld\n", x);
}