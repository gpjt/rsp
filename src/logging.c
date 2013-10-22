#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


void rsp_log(char* format, ...)
{
    char without_ms[64];
    char with_ms[64];
    struct timeval tv;
    struct tm *tm;

    gettimeofday(&tv, NULL);
    if ((tm = localtime(&tv.tv_sec)) != NULL)
    {
        strftime(without_ms, sizeof(without_ms), "%Y-%m-%d %H:%M:%S.%%06u %z", tm);
        snprintf(with_ms, sizeof(with_ms), without_ms, tv.tv_usec);
        fprintf(stdout, "[%s] ", with_ms); 
    }

    va_list argptr;
    va_start(argptr, format);
    vfprintf(stdout, format, argptr);
    va_end(argptr);

    fprintf(stdout, "\n");

    fflush(stdout);
}


void rsp_log_error(char* message)
{
    char* error = strerror(errno);
    rsp_log("%s: %s", message, error);
}

