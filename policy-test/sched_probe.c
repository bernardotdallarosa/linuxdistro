#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <semaphore.h>
#include <sys/syscall.h>

#define SCHED_LOW_IDLE 7

volatile int running = 1;
sem_t sem;

struct job {
	const char *name;
	int policy;
	int sleep_ms;		/* 0: laco puro; >0: dorme sleep_ms a cada volta */
	volatile long count;
	int post;		/* acorda B (sem_post) a cada ~1M de voltas */
	int wait;		/* espera o sem_post de A a cada volta */
};

void *run(void *data)
{
	struct job *j = data;
	struct sched_param param = { .sched_priority = 0 };

	if (sched_setscheduler(0, j->policy, &param) != 0)
		perror("sched_setscheduler");

	printf("%s: tid=%ld policy=%d sleep_ms=%d\n", j->name,
	       (long)syscall(SYS_gettid), sched_getscheduler(0), j->sleep_ms);

	while (running) {
		if (j->sleep_ms > 0)
			usleep(j->sleep_ms * 1000);
		else if (j->wait)
			sem_wait(&sem);
		j->count++;
		if (j->post && (j->count & 0xFFFFF) == 0)
			sem_post(&sem);
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct job a = { "A", 0, 0, 0, 0, 0 }, b = { "B", 0, 0, 0, 0, 0 };
	pthread_t ta, tb;

	if (argc < 4) {
		printf("usage: %s <segundos> <politica_A> <politica_B> [sleep_ms_B]\n", argv[0]);
		printf("politicas: 0=OTHER 5=IDLE 7=LOW_IDLE\n");
		printf("A sempre roda em laco puro; B dorme sleep_ms entre cada volta (se informado)\n");
		printf("sleep_ms_B = -1: B espera um sem_post feito por A a cada ~1M de voltas\n");

		return 0;
	}

	a.policy = atoi(argv[2]);
	b.policy = atoi(argv[3]);
	if (argc > 4)
		b.sleep_ms = atoi(argv[4]);
	if (b.sleep_ms < 0) {
		sem_init(&sem, 0, 0);
		a.post = 1;
		b.wait = 1;
	}

	pthread_create(&ta, NULL, run, &a);
	pthread_create(&tb, NULL, run, &b);

	sleep(atoi(argv[1]));
	running = 0;
	if (b.wait)
		sem_post(&sem);

	pthread_join(ta, NULL);
	pthread_join(tb, NULL);

	printf("A: %ld\nB: %ld\n", a.count, b.count);

	return 0;
}