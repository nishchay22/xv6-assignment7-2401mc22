#include "types.h"
#include "stat.h"
#include "user.h"

#define MAX_N 5
#define MAX_M 4

// Deadlock Detection Algorithm
int detect_deadlock(int n, int m, int Available[MAX_M], int Allocation[MAX_N][MAX_M], int Request[MAX_N][MAX_M], int deadlocked[MAX_N])
{
  int Work[MAX_M];
  int Finish[MAX_N];
  int i, j, found;
  int deadlocked_count = 0;

  for(j = 0; j < m; j++)
    Work[j] = Available[j];

  for(i = 0; i < n; i++){
    int has_alloc = 0;
    for(j = 0; j < m; j++){
      if(Allocation[i][j] > 0){
        has_alloc = 1;
        break;
      }
    }
    Finish[i] = (has_alloc == 0) ? 1 : 0;
  }

  do {
    found = 0;
    for(i = 0; i < n; i++){
      if(!Finish[i]){
        int can_satisfy = 1;
        for(j = 0; j < m; j++){
          if(Request[i][j] > Work[j]){
            can_satisfy = 0;
            break;
          }
        }
        if(can_satisfy){
          for(j = 0; j < m; j++)
            Work[j] += Allocation[i][j];
          Finish[i] = 1;
          found = 1;
        }
      }
    }
  } while(found);

  for(i = 0; i < n; i++){
    if(!Finish[i]){
      deadlocked[deadlocked_count++] = i;
    }
  }

  return deadlocked_count;
}

// Banker's Algorithm Safety Algorithm
int is_safe_state(int n, int m, int Available[MAX_M], int Allocation[MAX_N][MAX_M], int Need[MAX_N][MAX_M], int safe_seq[MAX_N])
{
  int Work[MAX_M];
  int Finish[MAX_N];
  int i, j, found;
  int count = 0;

  for(j = 0; j < m; j++)
    Work[j] = Available[j];
  for(i = 0; i < n; i++)
    Finish[i] = 0;

  do {
    found = 0;
    for(i = 0; i < n; i++){
      if(!Finish[i]){
        int can_satisfy = 1;
        for(j = 0; j < m; j++){
          if(Need[i][j] > Work[j]){
            can_satisfy = 0;
            break;
          }
        }
        if(can_satisfy){
          for(j = 0; j < m; j++)
            Work[j] += Allocation[i][j];
          Finish[i] = 1;
          safe_seq[count++] = i;
          found = 1;
        }
      }
    }
  } while(found);

  return (count == n) ? 1 : 0;
}

void print_matrices(int n, int m, int Available[MAX_M], int Allocation[MAX_N][MAX_M], int Second[MAX_N][MAX_M], const char *second_name)
{
  int i, j;
  printf(1, "Available: [");
  for(j = 0; j < m; j++)
    printf(1, " %d", Available[j]);
  printf(1, " ]\n");

  printf(1, "Proc  | Allocation  | %s\n", second_name);
  for(i = 0; i < n; i++){
    printf(1, " P%d   |", i);
    for(j = 0; j < m; j++)
      printf(1, " %d", Allocation[i][j]);
    printf(1, "     |");
    for(j = 0; j < m; j++)
      printf(1, " %d", Second[i][j]);
    printf(1, "\n");
  }
}

void run_test1_deadlock(void)
{
  printf(1, "\n--------------------------------------------------\n");
  printf(1, "Test 1: Deadlocked Resource Allocation State\n");
  printf(1, "--------------------------------------------------\n");
  int n = 3, m = 3;
  int Available[MAX_M] = {0, 0, 0};
  int Allocation[MAX_N][MAX_M] = {
    {0, 1, 0},
    {2, 0, 0},
    {0, 0, 3}
  };
  int Request[MAX_N][MAX_M] = {
    {0, 0, 1},
    {0, 1, 0},
    {1, 0, 0}
  };
  int deadlocked[MAX_N];

  print_matrices(n, m, Available, Allocation, Request, "Request");

  int count = detect_deadlock(n, m, Available, Allocation, Request, deadlocked);
  if(count > 0){
    int i;
    printf(1, "-> RESULT: DEADLOCK DETECTED! Deadlocked processes: ");
    for(i = 0; i < count; i++)
      printf(1, "P%d ", deadlocked[i]);
    printf(1, "\n");
  } else {
    printf(1, "-> RESULT: No deadlock detected.\n");
  }
}

void run_test2_safe(void)
{
  printf(1, "\n--------------------------------------------------\n");
  printf(1, "Test 2: Safe State (Banker's Algorithm)\n");
  printf(1, "--------------------------------------------------\n");
  int n = 5, m = 3;
  int Available[MAX_M] = {3, 3, 2};
  int Allocation[MAX_N][MAX_M] = {
    {0, 1, 0},
    {2, 0, 0},
    {3, 0, 2},
    {2, 1, 1},
    {0, 0, 2}
  };
  int Max[MAX_N][MAX_M] = {
    {7, 5, 3},
    {3, 2, 2},
    {9, 0, 2},
    {2, 2, 2},
    {4, 3, 3}
  };
  int Need[MAX_N][MAX_M];
  int safe_seq[MAX_N];
  int i, j;

  for(i = 0; i < n; i++)
    for(j = 0; j < m; j++)
      Need[i][j] = Max[i][j] - Allocation[i][j];

  print_matrices(n, m, Available, Allocation, Need, "Need");

  if(is_safe_state(n, m, Available, Allocation, Need, safe_seq)){
    printf(1, "-> RESULT: System is in a SAFE state.\nSafe Sequence: ");
    for(i = 0; i < n; i++){
      printf(1, "P%d%s", safe_seq[i], (i == n - 1) ? "\n" : " -> ");
    }
  } else {
    printf(1, "-> RESULT: System is in an UNSAFE state!\n");
  }
}

void run_test3_unsafe_request(void)
{
  printf(1, "\n--------------------------------------------------\n");
  printf(1, "Test 3: Banker's Avoidance - Request Leads to Unsafe State\n");
  printf(1, "--------------------------------------------------\n");
  int n = 5, m = 3;
  int Available[MAX_M] = {3, 3, 2};
  int Allocation[MAX_N][MAX_M] = {
    {0, 1, 0},
    {2, 0, 0},
    {3, 0, 2},
    {2, 1, 1},
    {0, 0, 2}
  };
  int Max[MAX_N][MAX_M] = {
    {7, 5, 3},
    {3, 2, 2},
    {9, 0, 2},
    {2, 2, 2},
    {4, 3, 3}
  };
  int Need[MAX_N][MAX_M];
  int safe_seq[MAX_N];
  int i, j;

  for(i = 0; i < n; i++)
    for(j = 0; j < m; j++)
      Need[i][j] = Max[i][j] - Allocation[i][j];

  printf(1, "Initial State:\n");
  print_matrices(n, m, Available, Allocation, Need, "Need");

  int req_pid = 0;
  int request[MAX_M] = {0, 2, 0};
  printf(1, "\nProcess P%d requests resources: [ 0 2 0 ]\n", req_pid);

  // Tentative allocation
  for(j = 0; j < m; j++){
    Available[j] -= request[j];
    Allocation[req_pid][j] += request[j];
    Need[req_pid][j] -= request[j];
  }

  printf(1, "Evaluating tentative state:\n");
  print_matrices(n, m, Available, Allocation, Need, "Need");

  if(is_safe_state(n, m, Available, Allocation, Need, safe_seq)){
    printf(1, "-> RESULT: Request can be safely granted.\n");
  } else {
    printf(1, "-> RESULT: Request REJECTED! State would become UNSAFE (potential deadlock).\n");
    // Rollback
    for(j = 0; j < m; j++){
      Available[j] += request[j];
      Allocation[req_pid][j] -= request[j];
      Need[req_pid][j] += request[j];
    }
    printf(1, "Resources rolled back safely.\n");
  }
}

void run_pipe_deadlock_simulation(void)
{
  printf(1, "\n--------------------------------------------------\n");
  printf(1, "Task 3c: Real xv6 Process & Pipe Deadlock Simulation\n");
  printf(1, "--------------------------------------------------\n");
  int p1[2], p2[2];
  if(pipe(p1) < 0 || pipe(p2) < 0){
    printf(1, "Error: pipe creation failed\n");
    return;
  }

  printf(1, "Creating Process A and Process B with circular pipe wait...\n");
  printf(1, "Resource 0: Pipe 1 | Resource 1: Pipe 2\n");
  printf(1, "Process A holds Pipe 1 (write) and waits for Pipe 2 (read)\n");
  printf(1, "Process B holds Pipe 2 (write) and waits for Pipe 1 (read)\n");

  int pid_a = fork();
  if(pid_a == 0){
    // Process A
    close(p1[0]); // holds p1[1] (write)
    close(p2[1]); // wants p2[0] (read)
    char buf;
    read(p2[0], &buf, 1); // blocks waiting for B
    exit();
  }

  int pid_b = fork();
  if(pid_b == 0){
    // Process B
    close(p2[0]); // holds p2[1] (write)
    close(p1[1]); // wants p1[0] (read)
    char buf;
    read(p1[0], &buf, 1); // blocks waiting for A
    exit();
  }

  // Model the state in deadlock detection matrix:
  int n = 2, m = 2;
  int Available[MAX_M] = {0, 0};
  int Allocation[MAX_N][MAX_M] = {
    {1, 0}, // Process A holds Resource 0 (Pipe 1)
    {0, 1}  // Process B holds Resource 1 (Pipe 2)
  };
  int Request[MAX_N][MAX_M] = {
    {0, 1}, // Process A waits for Resource 1 (Pipe 2)
    {1, 0}  // Process B waits for Resource 0 (Pipe 1)
  };
  int deadlocked[MAX_N];

  sleep(5); // Allow children to reach blocking read
  print_matrices(n, m, Available, Allocation, Request, "Request");

  int count = detect_deadlock(n, m, Available, Allocation, Request, deadlocked);
  if(count == 2){
    printf(1, "-> RESULT: Circular Wait Deadlock correctly identified between Process A and B!\n");
  }

  // Terminate deadlocked children so test finishes cleanly
  kill(pid_a);
  kill(pid_b);
  wait();
  wait();
  close(p1[0]); close(p1[1]);
  close(p2[0]); close(p2[1]);
  printf(1, "Cleaned up pipe simulation processes.\n");
}

int main(int argc, char *argv[])
{
  printf(1, "========================================================\n");
  printf(1, "Question 3: Deadlock Detection & Banker's Avoidance\n");
  printf(1, "Author: Nishchay (Roll No: 2401mc22)\n");
  printf(1, "========================================================\n");

  run_test1_deadlock();
  run_test2_safe();
  run_test3_unsafe_request();
  run_pipe_deadlock_simulation();

  printf(1, "\n========================================================\n");
  printf(1, "Question 3 Demonstration Completed Successfully!\n");
  printf(1, "========================================================\n");
  exit();
}
