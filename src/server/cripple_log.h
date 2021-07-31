#ifndef _CRIPPLE_LOG_H
#define _CRIPPLE_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "server_export.h"

void SERVER_EXPORT cri_log(const char *fmt, ...);

#endif
