#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

char *buff;
char *buff_start;
char *buff_end;
sem_t semaphore;

#include <string.h>
#include <sched.h>

int get_sched_policy(const char *policy_str)
{
	if (strcmp(policy_str, "SCHED_FIFO") == 0)
		return SCHED_FIFO;

	if (strcmp(policy_str, "SCHED_RR") == 0)
		return SCHED_RR;

#ifdef SCHED_IDLE
	if (strcmp(policy_str, "SCHED_IDLE") == 0)
		return SCHED_IDLE;
#endif

#ifdef SCHED_BATCH
	if (strcmp(policy_str, "SCHED_BATCH") == 0)
		return SCHED_BATCH;
#endif

#ifdef SCHED_LOW_IDLE
	if (strcmp(policy_str, "SCHED_LOW_IDLE") == 0)
		return SCHED_LOW_IDLE;
#endif

	return -1; // política inválida
}

void *task(void *arg)
{
	int tid = (int)(long int)arg;
	char thread_char = 'a' + tid;

	while (1) {
		sem_wait(&semaphore);
		if (buff >= buff_end) {
			sem_post(&semaphore);
			break;
		}
		*buff = thread_char;
		buff++;
		sem_post(&semaphore);
	}
	return NULL;
}

void post_process(long int thread_num)
{
	printf("Raw Buffer:\n\n");

	for (char *p = buff_start; p < buff_end; p++)
		putchar(*p);

	printf("\n\n");

	printf("Post Process Info: Execute order.\n\n");
	char *curr_char = buff_start;
	long int times_thread_exec[thread_num];
	char last_printed_char = '0';

	for (int i = 0; i < thread_num; i++) {
		times_thread_exec[i] = 0;
	}

	while (curr_char < buff_end) {
		if (last_printed_char != *curr_char) {
			times_thread_exec[(int)(*curr_char - 'a')]++;
			last_printed_char = *curr_char;
			putchar(*curr_char);
		}
		curr_char++;
	}

	printf("\n\n");
	printf("Post Process Info: Times Executed.\n\n");

	for (long int i = 0; i < thread_num; i++) {
		printf("  %c =  %ld\n", (char)('a' + i), times_thread_exec[i]);
	}
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		return -1;
	}

	sem_init(&semaphore, 0, 1);

	int buff_size = atoi(argv[1]);
	buff = malloc(buff_size);

	if (buff == NULL) {
		perror("malloc");
		return -5;
	}

	buff_start = buff;
	buff_end = buff + buff_size;

	long int thread_num = atoi(argv[2]);

	int policy = get_sched_policy(argv[3]);

	if (policy == -1) {
		return -2;
	}

	pthread_t threads[thread_num];
	pthread_attr_t attr;
	struct sched_param param;
	pthread_attr_init(&attr);

	if (pthread_attr_setschedpolicy(&attr, policy) != 0) {
		perror("pthread_attr_setschedpolicy");
		return -3;
	}

	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

	param.sched_priority = 10;

	pthread_attr_setschedparam(&attr, &param);

	for (long int i = 0; i < thread_num; i++) {
		pthread_create(&threads[i], &attr, task, (void *)i);
	}

	for (long int i = 0; i < thread_num; i++) {
		pthread_join(threads[i], NULL);
	}

	post_process(thread_num);

	pthread_attr_destroy(&attr);
	sem_destroy(&semaphore);
	free(buff_start);

	return 0;
}
