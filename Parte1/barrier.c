#include "barrier.h"
#include <stdlib.h>

int barrier_init(barrier_t *b, int N) {
    b->count = 0;
    b->N = N;
    b->stage = 0;
    pthread_mutex_init(&b->lock, NULL);
    pthread_cond_init(&b->cond, NULL);
    return 0;
}

int barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->lock);

    int my_stage = b->stage;
    b->count++;

    if (b->count == b->N) {
        // Última hebra en llegar
        b->count = 0;
        b->stage++;
        pthread_cond_broadcast(&b->cond);
        pthread_mutex_unlock(&b->lock);
        return 1;
    } else {
        while (my_stage == b->stage) {
            pthread_cond_wait(&b->cond, &b->lock);
        }
        pthread_mutex_unlock(&b->lock);
        return 0;
    }
}

int barrier_destroy(barrier_t *b) {
    pthread_mutex_destroy(&b->lock);
    pthread_cond_destroy(&b->cond);
    return 0;
}
