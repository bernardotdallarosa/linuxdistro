#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 512

int main(void){
	int fd, ret;
	char command[BUFFER_LENGTH];
	char receive[BUFFER_LENGTH];

	printf("Starting XTEA driver test code example...\n");

	fd = open("/dev/xtea_driver", O_RDWR);
	if (fd < 0){
		perror("Failed to open the device...");
		return errno;
	}

	printf("Digite o comando (ex: enc 16 aabbccddeeff00112233445566778899):\n");
	if (!fgets(command, sizeof(command), stdin)){
		fprintf(stderr, "Erro ao ler o comando\n");
		return 1;
	}

	printf("Writing command to the device [%s].\n", command);
	ret = write(fd, command, strlen(command));
	if (ret < 0){
		perror("Failed to write the command to the device.");
		return errno;
	}

	printf("Reading the result from the device...\n");
	memset(receive, 0, sizeof(receive));
	ret = read(fd, receive, sizeof(receive));
	if (ret < 0){
		perror("Failed to read the result from the device.");
		return errno;
	}

	printf("Resultado: [%s]\n", receive);
	printf("End of the program\n");
	return 0;
}