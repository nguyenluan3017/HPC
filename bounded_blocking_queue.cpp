#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <iostream>
#include <string>

using namespace std;

const size_t N = 10;

class BoundedBlockingQueue {
private:
    int *_mem;
    size_t _cap;
    size_t _len;
    sem_t _sem;
    pthread_mutex_t _mut;

    void print_sem_val() {
        int val;
        sem_getvalue(&this->_sem, &val);
        printf("sem value = %d\n", val);
    }
public:

    // Initialize the queue with a maximum capacity limit.
    BoundedBlockingQueue(int capacity) {
        this->_mem = new int[capacity];
        this->_cap = capacity;
        this->_len = 0;
        sem_init(&this->_sem, 0, capacity);
        pthread_mutex_init(&this->_mut, nullptr);
    }

    ~BoundedBlockingQueue() {
        pthread_mutex_unlock(&this->_mut);
        pthread_mutex_destroy(&this->_mut);
        for (size_t i = 0; i < this->_cap; i++) {
            sem_post(&this->_sem);
        }
        sem_destroy(&this->_sem);
        delete[] this->_mem;
        this->_mem = nullptr;
    }

    // Add an element to the queue. 
    // The key requirement is that if the queue is already at full capacity, 
    // the calling thread must block (wait) until space becomes available in the queue.
    void enqueue(int element) {
        sem_wait(&this->_sem);

        pthread_mutex_lock(&this->_mut);
        if (this->_mem != nullptr) {
            this->_mem[this->_len] = element;
            this->_len++;
        }
        pthread_mutex_unlock(&this->_mut);

        this->print_sem_val();
    }

    // Remove and return an element from the queue. 
    // If the queue is empty, the calling thread 
    // must block (wait) until an element becomes available.
    int dequeue() {
        int result;

        pthread_mutex_lock(&this->_mut);
        if (this->_mem != nullptr) {
            result = this_mem[this->_len];
            this->_len--;
        }
        pthread_mutex_unlock(&this->_mut);

        sem_post(&this->_sem);

        this->print_sem_val();

        return result;
    }

    // Return the current number of elements in the queue.
    size_t size() {
        return this->_len;
    }
} q(7);

typedef struct {
    int code;
    BoundedBlockingQueue *q;
} param;

void *add_to_queue(void *args) {
    param *p = (param *)args;
    printf("Speaking from %d\n", p->code);
    usleep(500000 * (rand() % N));
    if (p->q->size() < p->q->cap()) {
        p->q->enqueue(p->code);
        printf("I added %d\n", p->code);
    } else {
        int value = p->q->dequeue();
        printf("I read %d\n", value);
    }    
    return nullptr;
}

int main() {
    pthread_t threads[N];
    param params[N];
    BoundedBlockingQueue q(N / 2);
    
    for (size_t i = 0; i < N; i++) {
        params[i] = param();
        params[i].code = i;
        params[i].q = &q;
        pthread_create(&threads[i], nullptr, add_to_queue, &params[i]);
    }

    for (size_t i = 0; i < N; i++) {
        pthread_join(threads[i], nullptr);
    }    

    return 0;
}
