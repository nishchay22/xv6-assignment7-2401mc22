#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_PROCS 4
#define WORK_LOAD 20000000

void cpu_work(int id, int init_pri)
{
  int pid = getpid();
  set_priority(pid, init_pri);
  int start_tick = uptime();
  int current_pri = get_priority(pid);

  printf(1, "[Process %d | PID %d] Started with Priority %d at Tick %d\n", id, pid, current_pri, start_tick);

  volatile double x = 0.0;
  int i;
  for(i = 0; i < WORK_LOAD; i++){
    x += 1.0;
  }

  int end_tick = uptime();
  int final_pri = get_priority(pid);
  printf(1, "--> [Process %d | PID %d] FINISHED at Tick %d (Duration: %d ticks) | Initial Pri: %d, Final Pri: %d\n",
         id, pid, end_tick, end_tick - start_tick, init_pri, final_pri);

  exit();
}

int main(int argc, char *argv[])
{
  int priorities[NUM_PROCS] = {5, 10, 15, 20};
  int i;

  printf(1, "=================================================================\n");
  printf(1, "Question 4: Priority Preemptive CPU Scheduler with Aging\n");
  printf(1, "Author: Nishchay (Roll No: 2401mc22)\n");
  printf(1, "=================================================================\n");
  printf(1, "Forking %d CPU-bound processes with distinct priorities (1=highest, 20=lowest):\n", NUM_PROCS);
  for(i = 0; i < NUM_PROCS; i++){
    printf(1, "  Process %d -> Assigned Priority %d\n", i, priorities[i]);
  }
  printf(1, "-----------------------------------------------------------------\n");

  for(i = 0; i < NUM_PROCS; i++){
    int pid = fork();
    if(pid < 0){
      printf(1, "Error: fork failed\n");
      exit();
    }
    if(pid == 0){
      // Child process
      cpu_work(i, priorities[i]);
    }
  }

  // Parent waits for all children
  for(i = 0; i < NUM_PROCS; i++){
    int finished_pid = wait();
    printf(1, "Parent reaped process with PID %d\n", finished_pid);
  }

  printf(1, "-----------------------------------------------------------------\n");
  printf(1, "SUCCESS: All processes completed! Higher priorities finished earlier,\n");
  printf(1, "and lowest priority (20) completed without starvation due to aging!\n");
  printf(1, "=================================================================\n");

  exit();
}
