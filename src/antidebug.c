#include "../include/famine.h"

#if NOSTD

bool is_traced(void) {
	int fd = open("/proc/self/status", O_RDONLY, 0);
	if (fd < 0)
		return (false);
	char buf[512];
	ssize_t n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 0)
		return (false);
	buf[n] = '\0';
	char *p = memmem(buf, n, "TracerPid:\t", 11);
	if (!p)
		return (false);
	p += 11;
	return (*p != '0' || (p[1] >= '0' && p[1] <= '9'));
}

#else

bool is_traced(void) {
	FILE *f = fopen("/proc/self/status", "r");
	if (!f) return (false);
	char line[256];
	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "TracerPid:\t", 11) == 0) {
			fclose(f);
			return (line[11] != '0' || (line[12] >= '0' && line [12] <= '9'));
		}
	}
	fclose(f);
	return (false);
}

#endif
