#include "cripple_log.h"

void cri_log(const char *fmt, ...)
{
	va_list ap;
	struct tm *tp;
	char *ti;
	int m, n;
	size_t l;
	char log_line[1024];
	int ret;
	time_t current_time;

	va_start(ap, fmt);
	tp = localtime(&current_time);
	ti = asctime(tp);
	l = sprintf(log_line, "%.24s ", ti);
	m = sizeof log_line - l - 1;
	n = vsnprintf(log_line + l, m, fmt, ap);
	l += n < m ? n : m - 1;
	log_line[l++] = '\n';
	ret = write(STDERR_FILENO, log_line, l);
	if (ret != l)
		fprintf(stderr, "write: %s", strerror(errno));
	va_end(ap);
}
