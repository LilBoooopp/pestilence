#include "../include/famine.h"

bool process_active(const char *name) {
	struct my_DIR dir;
	if (!opendir(&dir, "/proc"))
		return (false);
	struct linux_dirent64 *ent;
	size_t name_len = strlen(name);
	while ((ent = readdir(&dir))) {
		if (ent->d_name[0] < '0' || ent->d_name[0] > '9')
			continue;
		char path[64];
		// build "/proc/<pid>/comm"
		char *p = path;
		const char *prefix = "/proc/";
		while (*prefix) *p++ = *prefix++;
		char *d = ent->d_name;
		while (*d) *p++ = *d++;
		const char *suffix = "/comm";
		while (*suffix) *p++ = *suffix++;
		*p = '\0';

		int fd = open(path, O_RDONLY, 0);
		if (fd < 0)
			continue;
		char comm[64];
		ssize_t n = read(fd, comm, sizeof(comm) - 1);
		close(fd);
		if (n <= 0)
			continue;
		if (comm[n - 1] == '\n')
			n--;
		if ((size_t)n == name_len && memcmp(comm, name, name_len) == 0) {
			closedir(&dir);
			return (true);
		}
	}
	closedir(&dir);
	return (false);
}

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
