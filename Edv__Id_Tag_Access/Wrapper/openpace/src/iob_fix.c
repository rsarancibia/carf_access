#include <stdio.h>

FILE* __cdecl __iob_func(void)
{
    static FILE f[3];
    return f;
}