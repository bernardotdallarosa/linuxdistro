#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_LENGTH 512
#define DISK_SZ 1073741824
#define FORKS 29

int main()
{
	int ret, fd, i;
	pid_t pid;
	int is_child = 0;
	unsigned int pos;
	char buf[BUFFER_LENGTH];

	printf("Starting sector read example...\n");

	printf("Cleaning disk cache...\n");
	system("echo 3 > /proc/sys/vm/drop_caches");

	printf("Configuring scheduling queues...\n");
	system("echo 2 > /sys/block/sdb/queue/nomerges");
	system("echo 4 > /sys/block/sdb/queue/max_sectors_kb");
	system("echo 0 > /sys/block/sdb/queue/read_ahead_kb");

	printf("Forking processes to put stress on disk scheduler...\n");
	for (i = 0; i < FORKS; i++) {
		pid = fork();
		if (pid < 0) {
			perror("fork failed");
			break;
		}
		if (pid == 0) {
			is_child = 1;
			break;
		}
	}

	srand(getpid());

	fd = open("/dev/sdb", O_RDWR);
	if (fd < 0) {
		perror("Failed to open the device...");
		return errno;
	}

	for (i = 0; i < 10; i++) {
		pos = (rand() % (DISK_SZ >> 9));
		/* Set position */
		lseek(fd, pos * 512, SEEK_SET);
		/* Peform read. */
		read(fd, buf, 100);
	}
	close(fd);

	if (!is_child) {
		for (i = 0; i < FORKS; i++)
			wait(NULL);
	}

	return 0;
}