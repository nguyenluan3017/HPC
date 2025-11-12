#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N ((size_t)5)

typedef enum { 
    IDLE,
    THINKING,
    EATING,    
    SATIATED
} state_t;

typedef struct {
    state_t state;
    int index;
} parameter_t;

pthread_mutex_t chopsticks[N];
pthread_t philosophers[N];
parameter_t params[N];
sem_t gate_keeper;

void update_param(parameter_t *param) {
    if (param->state == IDLE) {
        param->state = THINKING;
    } else if (param->state == THINKING) {
        param->state = EATING;
    } else if (param->state == EATING) {
        param->state = SATIATED;
    }
    printf("[");
    for (size_t i = 0; i < N; i++) {
        state_t state = params[i].state;
        char state_str[64];
        if (i > 0) {
            printf(" ");
        }
        if (state == IDLE) {
            sprintf(state_str, "IDLE");
        } else if (state == THINKING) {
            sprintf(state_str, "THINKING");
        } else if (state == EATING) {
            sprintf(state_str, "EATING");
        } else {
            sprintf(state_str, "SATIATED");
        }        
        printf("%10s", state_str);
    }
    printf("]\n");
}

void *do_work(void *args) {
    parameter_t *param = (parameter_t *)args;
    int left_index = (param->index - 1 + N) % N;
    int right_index = (param->index + 1 + N) % N;

    update_param(param);

    sem_wait(&gate_keeper);

    pthread_mutex_lock(&chopsticks[left_index]);
    pthread_mutex_lock(&chopsticks[right_index]);

    update_param(param);
    
    pthread_mutex_unlock(&chopsticks[left_index]);
    pthread_mutex_unlock(&chopsticks[right_index]);

    update_param(param);

    return NULL;
}

int main(int argc, char **argv) {
    sem_init(&gate_keeper, 0, N);

    for (size_t i = 0; i < N; i++) {
        pthread_mutex_init(&chopsticks[i], NULL);
        params[i].state = IDLE;
        params[i].index = i;
    }

    for (size_t i = 0; i < N; i++) {
        pthread_create(&philosophers[i], NULL, do_work, &params[i]);
    }

    sleep(2);

    for (size_t i = 0; i < N; i++) {
        pthread_join(philosophers[i], NULL);
    }

    sem_destroy(&gate_keeper);
    for (size_t i = 0; i < N; i++) {
        pthread_mutex_unlock(&chopsticks[i]);
        pthread_mutex_destroy(&chopsticks[i]);
    }
    return 0;
}
