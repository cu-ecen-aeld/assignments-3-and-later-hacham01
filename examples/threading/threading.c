#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_args = (struct thread_data *) thread_param;

    // Sleep before attempting to obtain the mutex
    usleep(thread_args->wait_to_obtain_ms * 1000);

    // Obtain the mutex
    pthread_mutex_lock(thread_args->mutex);

    // Sleep while holding the mutex
    usleep(thread_args->wait_to_release_ms * 1000);

    // Release the mutex
    pthread_mutex_unlock(thread_args->mutex);

    // Mark success
    thread_args->thread_complete_success = true;

    return (void*) thread_args;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *thread_args = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (thread_args == NULL) {
        return false;
    }

    thread_args->mutex = mutex;
    thread_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_args->wait_to_release_ms = wait_to_release_ms;
    thread_args->thread_complete_success = false;

    int rc = pthread_create(thread, NULL, threadfunc, thread_args);
    if (rc != 0) {
        free(thread_args);
        return false;
    }

    return true;
}

