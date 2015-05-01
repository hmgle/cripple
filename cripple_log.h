#ifndef _CRIPPLE_LOG_H
#define _CRIPPLE_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

void cri_log(const char *fmt, ...);

#endif
