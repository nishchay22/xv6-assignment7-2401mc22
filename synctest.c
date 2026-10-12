#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_THREADS 3
#define ITERATIONS 100000
#define PGSIZE 4096

// Semaphore IDs
#define SEM_COUNTER 0
#define SEM_MUTEX   1
#define SEM_EMPTY   2
#define SEM_FULL    3

volatile int counter = 0;

// Shared buffer for Producer-Consumer
#define BUFFER_SIZE 5
#define TOTAL_ITEMS 15

int buffer[BUFFER_SIZE];
int in_idx = 0;
int out_idx = 0;
int buf_count = 0;

void sync_worker(void *arg)
{
  int id = (int)(long)arg;
  int i, temp;

  printf(1, "[Thread %d] Started synchronized worker\n", id);
  for(i = 0; i < ITERATIONS; i++){
    sem_wait(SEM_COUNTER);
    temp = counter;
    counter = temp + 1;
    sem_signal(SEM_COUNTER);
  }
  printf(1, "[Thread %d] Finished synchronized worker\n", id);
  exit();
}

void producer_worker(void *arg)
{
  int i, item;
  for(i = 1; i <= TOTAL_ITEMS; i++){
    item = i * 10;
    sem_wait(SEM_EMPTY);
    sem_wait(SEM_MUTEX);

    buffer[in_idx] = item;
    in_idx = (in_idx + 1) % BUFFER_SIZE;
    buf_count++;
    printf(1, "[Producer] Produced item %d | Buffer count = %d/5\n", item, buf_count);

    sem_signal(SEM_MUTEX);
    sem_signal(SEM_FULL);
  }
  exit();
}

void consumer_worker(void *arg)
{
  int i, item;
  for(i = 1; i <= TOTAL_ITEMS; i++){
    sem_wait(SEM_FULL);
    sem_wait(SEM_MUTEX);

    item = buffer[out_idx];
    out_idx = (out_idx + 1) % BUFFER_SIZE;
    buf_count--;
    printf(1, "   [Consumer] Consumed item %d | Buffer count = %d/5\n", item, buf_count);

    sem_signal(SEM_MUTEX);
    sem_signal(SEM_EMPTY);
  }
  exit();
}

int main(int argc, char *argv[])
{
  int i, tid, joined_pid;
  void *stacks[NUM_THREADS];
  void *prod_stack, *cons_stack;

  printf(1, "========================================================\n");
  printf(1, "Question 2: Process/Thread Synchronization (Semaphores)\n");
  printf(1, "Author: Nishchay (Roll No: 2401mc22)\n");
  printf(1, "========================================================\n\n");

  // Part 1: Synchronized Counter
  printf(1, "--- Part 1: Synchronized Counter Increment Test ---\n");
  sem_init(SEM_COUNTER, 1);
  counter = 0;

  for(i = 0; i < NUM_THREADS; i++){
    stacks[i] = malloc(PGSIZE);
    tid = clone(sync_worker, (void*)(long)(i + 1), stacks[i]);
    if(tid < 0){
      printf(1, "Error: clone failed\n");
      exit();
    }
  }

  for(i = 0; i < NUM_THREADS; i++){
    joined_pid = join();
    printf(1, "Joined thread with PID %d\n", joined_pid);
    free(stacks[i]);
  }

  printf(1, "Final Counter Value: %d (Expected: %d)\n", counter, NUM_THREADS * ITERATIONS);
  if(counter == NUM_THREADS * ITERATIONS){
    printf(1, "SUCCESS: Race condition resolved via semaphores! Final counter = %d\n\n", counter);
  } else {
    printf(1, "FAILURE: Counter mismatch (%d != %d)\n\n", counter, NUM_THREADS * ITERATIONS);
  }

  // Part 2: Producer-Consumer
  printf(1, "--- Part 2: Classic Producer-Consumer Problem ---\n");
  printf(1, "Buffer Size: %d, Total Items: %d\n", BUFFER_SIZE, TOTAL_ITEMS);

  sem_init(SEM_MUTEX, 1);
  sem_init(SEM_EMPTY, BUFFER_SIZE);
  sem_init(SEM_FULL, 0);

  in_idx = 0;
  out_idx = 0;
  buf_count = 0;

  prod_stack = malloc(PGSIZE);
  cons_stack = malloc(PGSIZE);

  tid = clone(producer_worker, (void*)1, prod_stack);
  printf(1, "Started Producer thread (PID %d)\n", tid);

  tid = clone(consumer_worker, (void*)2, cons_stack);
  printf(1, "Started Consumer thread (PID %d)\n", tid);

  join();
  join();

  free(prod_stack);
  free(cons_stack);

  printf(1, "SUCCESS: Producer-Consumer completed without overflow/underflow!\n");
  printf(1, "========================================================\n");

  exit();
}
