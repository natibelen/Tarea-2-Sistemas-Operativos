#ifndef BARRIER_H
#define BARRIER_H
#include <pthread.h>

typedef struct {
    int count;
    int N;
    int stage;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} barrier_t;

int barrier_init(barrier_t *b, int N);
int barrier_wait(barrier_t *b);
int barrier_destroy(barrier_t *b);

#endif
