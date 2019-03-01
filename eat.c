#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#define NUM 5

pthread_mutex_t chopstick[NUM];

#define LEFT(i) (&chopstick[((i) + NUM - 1)%NUM])
#define RIGHT(i) (&chopstick[((i) + 1)%NUM])

void *philosoper(void* pos) {
    int p = (int)pos;
    printf("Philosopher %c sits at the table.\n", 'A' + p);
    for(;;) {
        do {
            pthread_mutex_unlock(LEFT(p));
            pthread_mutex_unlock(RIGHT(p));
            sleep(rand() % 10);
        } while(pthread_mutex_trylock(LEFT(p)) == EBUSY ||
                pthread_mutex_trylock(RIGHT(p)) == EBUSY);

        // sleep(rand() % 10);
        // pthread_mutex_lock(LEFT(p));
        // printf("Pholosopher %c fetches chopstick %d\n", 'A' + p, (p + NUM - 1) % NUM);
        // pthread_mutex_lock(RIGHT(p));
        // printf("Pholosopher %c fetches chopstick %d\n", 'A' + p, (p + 1) % NUM);

        printf("Philosopher %c start eating.\n", 'A' + p);
        sleep(rand() % 10);
        printf("Philosopher %c eats finish.\n", 'A' + p);
        pthread_mutex_unlock(LEFT(p));
        pthread_mutex_unlock(RIGHT(p));
    }
    return NULL;
}

int main(void) {
    srand(time(NULL));
    pthread_t philosopers[NUM];

    for (int i = 0; i < NUM; i++)
        pthread_mutex_init(&chopstick[i], NULL);

    for (int i = 0; i < NUM; i++) 
        pthread_create(&philosopers[i], NULL, philosoper, (void*)i);
    
    for (int i = 0; i < NUM; i++) 
        pthread_join(philosopers[i], NULL);
    
    for (int i = 0; i < NUM; i++)
        pthread_mutex_destroy(&chopstick[i]);
    return 0;
}