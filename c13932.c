pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	InitializeCriticalSection(mutex);
	return 0;
}