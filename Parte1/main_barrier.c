#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "barrier.h"

typedef struct {
    int id;
    barrier_t *bar;
    int E;
} thread_arg_t;

void *worker(void *arg) {
    thread_arg_t *a = (thread_arg_t*)arg;

    for (int e = 0; e < a->E; e++) {
        usleep((rand() % 300) * 1000); // trabajo random
        printf("[%d] esperando en etapa %d\n", a->id, e);
        fflush(stdout);

        barrier_wait(a->bar);

        printf("[%d] pasó barrera en etapa %d\n", a->id, e);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int N = 5; // número de hebras
    int E = 4; // etapas

    srand(42);

    barrier_t bar;
    barrier_init(&bar, N);

    pthread_t threads[N];
    thread_arg_t args[N];

    for (int i = 0; i < N; i++) {
        args[i].id = i;
        args[i].bar = &bar;
        args[i].E = E;
        pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    barrier_destroy(&bar);
    return 0;
}
