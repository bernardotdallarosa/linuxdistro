#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 256
#define MAX_MESSAGES 10

int main(){
	int ret, fd, i;
	int num_msgs;
	char receive[BUFFER_LENGTH];
	char stringToSend[BUFFER_LENGTH];

	printf("Starting device test code example...\n");
	fd = open("/dev/list_driver", O_RDWR);
	if (fd < 0){
		perror("Failed to open the device...");
		return errno;
	}

	printf("Quantas mensagens deseja enviar (max %d)? ", MAX_MESSAGES);
	scanf("%d", &num_msgs);
	getchar();

	if (num_msgs > MAX_MESSAGES)
		num_msgs = MAX_MESSAGES;

	for (i = 0; i < num_msgs; i++){
		printf("Mensagem %d de %d: ", i + 1, num_msgs);
		scanf("%[^\n]%*c", stringToSend);

		ret = write(fd, stringToSend, strlen(stringToSend));
		if (ret < 0){
			perror("Failed to write the message to the device.");
			return errno;
		}
	}

	printf("Lendo as mensagens...\n");
	for (i = 0; i < num_msgs; i++){
		memset(receive, 0, BUFFER_LENGTH);

		ret = read(fd, receive, BUFFER_LENGTH);
		if (ret < 0){
			perror("Failed to read the message from the device.");
			return errno;
		}
		printf("Mensagem recebida: [%s]\n", receive);
	}

	printf("End of the program\n");
	return 0;
}