#include "threads.h"

#ifdef __linux__

int thread_create(thread_t *thread, void* (*start_routine)(void*), void* arg) {
	return pthread_create(thread, NULL, start_routine, arg);
}

void thread_join(thread_t thread) {
	pthread_join(thread, NULL);
}

#elif _WIN32

int thread_create(thread_t *thread, void* (*start_routine)(void*), void* arg) {
	*thread = CreateThread(
		NULL,
		0,
		(unsigned long (*)(void*))start_routine,
		arg,
		0,
		NULL
	);
	return 0;
}

void thread_join(thread_t thread) {
	WaitForSingleObject(thread, INFINITE);
}

#endif

