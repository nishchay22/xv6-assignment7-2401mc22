#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_THREADS 3
#define ITERATIONS 100000
#define PGSIZE 4096

volatile int counter = 0;

void worker(void *arg)
{
  int id = (int)(long)arg;
  int i;
  volatile int d;
  int temp;

  printf(1, "[Thread %d] Started, initial counter = %d\n", id, counter);
  for(i = 0; i < ITERATIONS; i++){
    temp = counter;
    for(d = 0; d < 30; d++);
    counter = temp + 1;
  }
  printf(1, "[Thread %d] Finished work\n", id);
  exit();
}

int main(int argc, char *argv[])
{
  int i, tid, joined_pid;
  void *stacks[NUM_THREADS];

  printf(1, "========================================\n");
  printf(1, "Question 1: Kernel Threads (clone/join)\n");
  printf(1, "Author: Nishchay (Roll No: 2401mc22)\n");
  printf(1, "========================================\n");
  printf(1, "Creating %d threads, each incrementing counter %d times...\n", NUM_THREADS, ITERATIONS);
  printf(1, "Expected total increments: %d\n", NUM_THREADS * ITERATIONS);

  counter = 0;

  for(i = 0; i < NUM_THREADS; i++){
    stacks[i] = malloc(PGSIZE);
    if(!stacks[i]){
      printf(1, "Error: malloc failed for thread %d\n", i + 1);
      exit();
    }
    tid = clone(worker, (void*)(long)(i + 1), stacks[i]);
    if(tid < 0){
      printf(1, "Error: clone failed for thread %d\n", i + 1);
      exit();
    }
    printf(1, "Spawned thread %d with PID %d\n", i + 1, tid);
  }

  for(i = 0; i < NUM_THREADS; i++){
    joined_pid = join();
    printf(1, "Joined thread with PID %d\n", joined_pid);
    free(stacks[i]);
  }

  printf(1, "\nFinal Counter Value: %d (Expected without race: %d)\n", counter, NUM_THREADS * ITERATIONS);
  if(counter < NUM_THREADS * ITERATIONS){
    printf(1, "SUCCESS: Race condition verified! Shared counter (%d) < %d\n", counter, NUM_THREADS * ITERATIONS);
  } else {
    printf(1, "Observed Counter: %d\n", counter);
  }

  exit();
}
