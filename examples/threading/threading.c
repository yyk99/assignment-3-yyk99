#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#ifdef NDEBUG
#define DEBUG_LOG(msg,...)
#else
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#endif
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    DEBUG_LOG("[%lx] %p", pthread_self(), thread_param); 
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    if (thread_func_args)
    {
        usleep(thread_func_args->wait_to_obtain_ms * 1000); /* milli to micro */
        int rc;
        if ((rc = pthread_mutex_lock(thread_func_args->mutex))) {
            ERROR_LOG("[%lx] pthread_mutex_lock failed: (%d)", pthread_self(), rc);
            thread_func_args->thread_complete_success = false;
        } else {
            usleep(thread_func_args->wait_to_release_ms * 1000);
            pthread_mutex_unlock(thread_func_args->mutex);
            thread_func_args->thread_complete_success = true;
        }
    }
    return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    
    struct thread_data* thread_func_args = calloc(sizeof(thread_func_args), 1);
    if (!thread_func_args) {
        ERROR_LOG("Cannot allocate thread_data");
        return false;
    }
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms; 

    int rc;
    if ((rc = pthread_create(thread, NULL, threadfunc, thread_func_args))){
        ERROR_LOG("Cannot create thread (%d)", rc);
        return false;
    }

    DEBUG_LOG("Created [%lx]", *thread); 
    return true;
}

/* Local Variables: */
/* mode: c */
/* c-file-style: "linux" */
/* c-basic-offset: 4 */
/* indent-tabs-mode: nil */
/* End: */
