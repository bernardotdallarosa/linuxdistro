#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

/*
André Luiz, Bernardo Dallarosa e Guilherme Braun
Laboratório de Sistemas Operacionais - 2026/02
Trabalho 3 - CPU Scheduling Profiler
*/

#ifndef SCHED_IDLE
#define SCHED_IDLE 5
#endif
#ifndef SCHED_LOW_IDLE
#define SCHED_LOW_IDLE 7
#endif

#define MAX_THREADS 26

char *buf, *ptr, *end;
sem_t mutex;
pthread_barrier_t start;
int policy[MAX_THREADS];
struct sched_param param[MAX_THREADS];

void *task(void *arg){
	long id = (long)arg;
	char c = 'A' + id;
	int err = pthread_setschedparam(pthread_self(), policy[id], &param[id]);
 
	if (err) {
		fprintf(stderr, "thread %c: %s\n", c, strerror(err));
		exit(1);
	}
	printf("Thread %c -> TID: %d\n", c, (pid_t)syscall(__NR_gettid));
	pthread_barrier_wait(&start);
    if (policy[id] == SCHED_FIFO)
	    usleep(100000);	/* temporário: a fifo dorme 100 ms */
	sem_wait(&mutex);
	while (ptr < end) {
		*ptr++ = c;
		sem_post(&mutex);
		sem_wait(&mutex);
	}
	sem_post(&mutex);
	return NULL;
}

int parse(char *s){
	if (!strcmp(s, "low_idle")) return SCHED_LOW_IDLE;
	if (!strcmp(s, "idle")) return SCHED_IDLE;
	if (!strcmp(s, "fifo")) return SCHED_FIFO;
	if (!strcmp(s, "rr")) return SCHED_RR;
	return -1;
}

void use(char *prog){
	fprintf(stderr, "uso: %s <tam_buffer> <num_threads(1-26)> "
		"<politica> <prio> [<politica> <prio> ...]\n"
		"politicas: low_idle idle fifo rr\n", prog);
	exit(1);
}

int main(int argc, char **argv){
	pthread_t tid[MAX_THREADS];
	int runs[MAX_THREADS] = {0};
	long chars[MAX_THREADS] = {0};
	long size, n, i;

	if (argc < 3)
		use(argv[0]);
	size = atol(argv[1]);
	n = atol(argv[2]);
	if (size < 1 || n < 1 || n > MAX_THREADS || argc != 3 + 2 * n)
		use(argv[0]);

	for (i = 0; i < n; i++) {
		policy[i] = parse(argv[3 + 2 * i]);
		if (policy[i] < 0) {
			fprintf(stderr, "unknown policy: %s\n", argv[3 + 2 * i]);
			return 1;
		}
		if (policy[i] == SCHED_FIFO || policy[i] == SCHED_RR)
			param[i].sched_priority = atoi(argv[4 + 2 * i]);
	}

	buf = malloc(size);
	ptr = buf;
	end = buf + size;
	sem_init(&mutex, 0, 1);
	pthread_barrier_init(&start, NULL, n);
 
	for (i = 0; i < n; i++)
		pthread_create(&tid[i], NULL, task, (void *)i);
	for (i = 0; i < n; i++)
		pthread_join(tid[i], NULL);
	sem_destroy(&mutex);
	pthread_barrier_destroy(&start);

	printf("\n");
	for (i = 0; i < size; i++) {
		chars[buf[i] - 'A']++;
		if (i == 0 || buf[i] != buf[i - 1]) {
			putchar(buf[i]);
			runs[buf[i] - 'A']++;
		}
	}
	printf("\n");
	for (i = 0; i < n; i++)
		printf("%c = %d (%ld caracteres)\n", 'A' + (int)i, runs[i], chars[i]);
	free(buf);
	return 0;
}