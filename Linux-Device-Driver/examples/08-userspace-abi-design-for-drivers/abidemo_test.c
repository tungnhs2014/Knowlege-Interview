// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "abidemo_uapi.h"

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "/dev/abidemo0";
	struct abidemo_config cfg = { 0 };
	struct pollfd pfd;
	char buf[128];
	ssize_t n;
	int fd;
	int ret;

	fd = open(path, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = ioctl(fd, ABIDEMO_IOC_GET_CONFIG, &cfg);
	if (ret) {
		perror("ioctl GET_CONFIG");
		goto out_close;
	}
	printf("initial config: version=%u mode=%u flags=%u\n",
	       cfg.version, cfg.mode, cfg.flags);

	cfg.mode = 1;
	ret = ioctl(fd, ABIDEMO_IOC_SET_CONFIG, &cfg);
	if (ret) {
		perror("ioctl SET_CONFIG");
		goto out_close;
	}

	pfd.fd = fd;
	pfd.events = POLLIN;
	ret = poll(&pfd, 1, 100);
	if (ret < 0) {
		perror("poll before write");
		goto out_close;
	}
	printf("poll before write: ret=%d revents=0x%x\n", ret, pfd.revents);

	n = write(fd, "abi event\n", strlen("abi event\n"));
	if (n < 0) {
		perror("write");
		goto out_close;
	}
	printf("write: %zd bytes\n", n);

	pfd.revents = 0;
	ret = poll(&pfd, 1, 1000);
	if (ret < 0) {
		perror("poll after write");
		goto out_close;
	}
	printf("poll after write: ret=%d revents=0x%x\n", ret, pfd.revents);

	n = read(fd, buf, sizeof(buf) - 1);
	if (n < 0) {
		perror("read");
		goto out_close;
	}
	buf[n] = '\0';
	printf("read: %zd bytes: %s", n, buf);

	ret = ioctl(fd, ABIDEMO_IOC_CLEAR_EVENT);
	if (ret)
		perror("ioctl CLEAR_EVENT");

out_close:
	close(fd);
	return ret ? 1 : 0;
}
