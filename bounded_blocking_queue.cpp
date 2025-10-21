#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <list>

using namespace std;

const size_t N = 10;

class BoundedBlockingQueue {
private:
    list<int> _mem;
    size_t _cap;
    size_t _len;
    sem_t _sem_full_slots;
    sem_t _sem_empty_slots;
    pthread_mutex_t _mut;

    void print_sem_val() {
        return;
        int full_val, empty_val;
        sem_getvalue(&this->_sem_full_slots, &full_val);
        sem_getvalue(&this->_sem_empty_slots, &empty_val);
        printf("full = %d, empty = %d\n", full_val, empty_val);
    }
public:

    // Initialize the queue with a maximum capacity limit.
    BoundedBlockingQueue(int capacity) {
        this->_cap = capacity;
        this->_len = 0;
        sem_init(&this->_sem_full_slots, 0, capacity);
        sem_init(&this->_sem_empty_slots, 0, 0);
        pthread_mutex_init(&this->_mut, nullptr);
    }

    ~BoundedBlockingQueue() {
        pthread_mutex_unlock(&this->_mut);
        pthread_mutex_destroy(&this->_mut);
        for (size_t i = 0; i < this->_cap; i++) {
            sem_post(&this->_sem_empty_slots);
            sem_post(&this->_sem_full_slots);
        }
        sem_destroy(&this->_sem_empty_slots);
        sem_destroy(&this->_sem_full_slots);
        this->_mem.clear();        
    }

    // Add an element to the queue. 
    // The key requirement is that if the queue is already at full capacity, 
    // the calling thread must block (wait) until space becomes available in the queue.
    void enqueue(int element) {
        sem_wait(&this->_sem_full_slots);

        pthread_mutex_lock(&this->_mut);
        this->_mem.push_back(element);
        this->_len++;
        pthread_mutex_unlock(&this->_mut);

        sem_post(&this->_sem_empty_slots);

        this->print_sem_val();
    }

    // Remove and return an element from the queue. 
    // If the queue is empty, the calling thread 
    // must block (wait) until an element becomes available.
    int dequeue() {
        int result;

        sem_wait(&this->_sem_empty_slots);

        pthread_mutex_lock(&this->_mut);
        result = this->_mem.front();
        this->_mem.pop_front();
        this->_len--;
        pthread_mutex_unlock(&this->_mut);

        sem_post(&this->_sem_full_slots);

        this->print_sem_val();

        return result;
    }

    // Return the current number of elements in the queue.
    size_t size() const {
        return this->_len;
    }

    size_t cap() const {
        return this->_cap;
    }
} q(7);

typedef struct {
    int code;
    BoundedBlockingQueue *q;
} param;

void *add_to_queue(void *args) {
    param *p = (param *)args;
    printf("Speaking from %d\n", p->code);
    usleep(500000 * p->code);
    p->q->enqueue(p->code);
    printf("I added %d\n", p->code);
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

    usleep(600000 * (N / 2));
    for (size_t i = 0; i < N; i++) {
        printf("main(): I popped %d\n", q.dequeue());
        usleep(500000 * (i + 1));
    }

    for (size_t i = 0; i < N; i++) {
        pthread_join(threads[i], nullptr);
    }    

    return 0;
}
