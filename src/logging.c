#include <stdarg.h>
#include <stdio.h>
#include <time.h>


void rsp_log(char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stdout, format, argptr);
    va_end(argptr);

    fflush(stdout);
}
