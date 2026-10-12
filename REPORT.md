# Operating Systems Lab — Assignment 7 Report
**xv6 Concurrency, Synchronization, Deadlock Detection, and Priority Scheduling**

- **Student Name**: Nishchay  
- **Roll Number**: 2401mc22  
- **Repository**: [https://github.com/nishchay22/xv6-assignment7-2401mc22.git](https://github.com/nishchay22/xv6-assignment7-2401mc22.git)  
- **Environment**: xv6 (x86) emulated on QEMU 8.2+ / Ubuntu WSL2  

---

## 1. Executive Summary

This report documents the architectural design, implementation details, challenges encountered, and empirical verification for **Operating Systems Lab Assignment 7** on the **xv6** operating system. The assignment comprises four key milestones:
1. **Kernel-Supported Threads**: Implementation of `clone()` and `join()` system calls with shared virtual address space (`pgdir`) and private kernel/user stacks, and reproduction of concurrent race conditions (`threadtest.c`).
2. **Counting Semaphores**: Implementation of kernel semaphores (`sem_init`, `sem_wait`, `sem_signal`) using spinlocks and selective wakeups, solving the race condition (`synctest.c`), and solving the classic bounded-buffer Producer-Consumer problem.
3. **Deadlock Detection and Banker's Algorithm**: Resource allocation modeling, detection of circular wait in static matrices and real xv6 pipe processes, and deadlock avoidance via Banker's algorithm (`deadlock.c`).
4. **Priority-Based Preemptive CPU Scheduling with Aging**: Replacing the default round-robin scheduler with a preemptive priority scheduler (priorities 1–20), implementing dynamic aging to prevent starvation, and analyzing completion times across distinct priorities (`priotest.c`).

---

## 2. Design Decisions & Key Data Structures

### Question 1: Kernel-Supported Threads (`clone` & `join`)
- **Key Modifications in `struct proc` (`proc.h`)**:
  - `int is_thread`: Flag indicating whether the process descriptor corresponds to a thread spawned via `clone()`.
- **System Call `int clone(void(*fcn)(void*), void *arg, void *stack)`**:
  - Allocates a new `struct proc` via `allocproc()`.
  - **Shared Memory**: Rather than allocating and copying page tables via `copyuvm()`, `np->pgdir` is set directly to `curproc->pgdir`, and `np->sz = curproc->sz`.
  - **Stack Setup**: In x86 `cdecl` calling convention, a function `fcn(arg)` expects its argument at `esp + 4` and its return address at `esp`. `clone()` initializes `ustack` (top of user-provided page or buffer), pushes `arg`, pushes a fake return address `0xffffffff`, and points `np->tf->esp = ustack` and `np->tf->eip = (uint)fcn`.
  - **Resource Sharing**: Open files (`ofile`) are duplicated using `filedup()`, and `cwd` is referenced using `idup()`.
- **System Call `int join(void)`**:
  - Scans `ptable.proc` for child threads (`p->parent == curproc && p->is_thread`).
  - Upon locating a zombie thread, frees its kernel stack `kfree(p->kstack)` and clears its `struct proc` entry.
  - **Crucial Distinction**: `join()` strictly **avoids** calling `freevm(p->pgdir)` because the virtual address space remains active for the parent process and other peer threads.
- **`exit()` and `wait()` Safety**:
  - `wait()` was updated to skip child threads (`p->is_thread == 1`), ensuring threads are reaped exclusively by `join()`. If parent process is `initproc`, orphaned threads are reaped without destroying active page directories.

### Question 2: Process/Thread Synchronization (Counting Semaphores)
- **Kernel Data Structure (`proc.c`)**:
  ```c
  #define MAX_SEM 10
  struct semaphore {
    int value;           // Current semaphore count (< 0 implies waiting processes)
    int active;          // Allocation status
    struct spinlock lk;  // Per-semaphore spinlock protecting value and sleeping processes
  };
  static struct semaphore semtable[MAX_SEM];
  ```
- **System Calls**:
  - `sem_init(int sem_id, int value)`: Validates ID and initial value; initializes spinlock and sets `semtable[sem_id].value = value`.
  - `sem_wait(int sem_id)`: Acquires spinlock, decrements `value`. If `value < 0`, blocks process via `sleep(&semtable[sem_id], &semtable[sem_id].lk)`.
  - `sem_signal(int sem_id)`: Acquires spinlock, increments `value`. If `value <= 0`, wakes a waiting process using custom `wakeup_one(&semtable[sem_id])`.
- **Selective Wakeup Design Decision**:
  xv6's default `wakeup(chan)` wakes *all* processes sleeping on `chan`. In counting semaphores with negative values representing waiters, waking all processes causes a thundering herd where multiple threads wake up on a single signal token. To preserve strict Dijkstra semaphore semantics, `wakeup_one1(chan)` was introduced in `proc.c` to transition exactly one sleeping thread to `RUNNABLE`.

### Question 3: Deadlock Detection & Banker's Algorithm
- **User-Space Simulation (`deadlock.c`)**:
  - Static structures: `Allocation[N][M]`, `Request[N][M]`, `Need[N][M]`, `Available[M]`.
  - **Deadlock Detection Algorithm (`detect_deadlock`)**:
    - Initializes `Work = Available`.
    - Sets `Finish[i] = 1` if `Allocation[i] == 0`, else `0`.
    - Iteratively searches for process `i` where `Finish[i] == 0` and `Request[i] <= Work`. If found, simulates completion by releasing resources: `Work += Allocation[i]`, `Finish[i] = 1`.
    - Returns all processes left with `Finish[i] == 0` as deadlocked.
  - **Banker's Algorithm Avoidance (`is_safe_state`)**:
    - Calculates `Need[i][j] = Max[i][j] - Allocation[i][j]`.
    - Verifies hypothetical state safety before granting any request, ensuring a complete safe execution sequence exists.
  - **Real xv6 Pipe Circular Wait (Task 3c)**:
    - Child process A holds write end of Pipe 1 and attempts blocking `read()` on Pipe 2.
    - Child process B holds write end of Pipe 2 and attempts blocking `read()` on Pipe 1.
    - Deadlock detector identifies the circular wait between Process A and Process B.

### Question 4: Priority-Based Preemptive CPU Scheduling with Aging
- **Fields Added to `struct proc`**:
  - `int priority`: Numerical priority where `1` is highest and `20` is lowest (default creation priority = `10`).
  - `int wait_ticks`: Counter tracking ticks spent continuously in `RUNNABLE` state without CPU allocation.
- **System Calls**:
  - `set_priority(int pid, int priority)`: Validates range `[1, 20]`, resets `wait_ticks = 0`, returns previous priority.
  - `get_priority(int pid)`: Reads current dynamic priority under `ptable.lock`.
- **Preemptive Scheduler (`scheduler` in `proc.c`)**:
  - Evaluates all `RUNNABLE` processes and determines `min_priority` (lowest priority number = highest priority).
  - Implements **round-robin tie-breaking** among processes sharing `min_priority` using `last_proc` circular cursor.
  - Switches context to the selected process and resets its `wait_ticks = 0`.
- **Dynamic Aging Mechanism**:
  - Hooked into `trap.c` under `case T_IRQ0 + IRQ_TIMER` when `cpuid() == 0` via `aging_tick()`.
  - Increments `wait_ticks` for all `RUNNABLE` processes.
  - When `wait_ticks >= 30`:
    - Decrements numerical priority (`p->priority--`, moving closer to 1).
    - Resets `wait_ticks = 0`.

---

## 3. Problems Faced & Solutions

| Problem Encountered | Technical Root Cause | Resolution |
| :--- | :--- | :--- |
| **Premature Address Space Deallocation on Thread Exit** | When a thread called `exit()`, `wait()` in xv6 automatically called `freevm(p->pgdir)`, destroying page tables while parent or sibling threads were still running. | Updated `wait()` to skip processes where `is_thread == 1`. In `join()`, freed only `kstack` and proc structure while keeping `pgdir` intact. |
| **Thundering Herd / Lockup in Semaphores** | xv6 `wakeup()` wakes every sleeper on a channel. If multiple threads woke up when semaphore incremented by 1, multiple threads tried to claim the token or re-slept incorrectly. | Implemented `wakeup_one()` which scans `ptable.proc` and wakes exactly one sleeping process per `sem_signal()` call when `value <= 0`. |
| **Thread Increment Loop Timing in QEMU** | Under `-O2` compiler optimization, tight loops with non-volatile variables ran in < 1 ms before the first timer interrupt (100 Hz quantum), occasionally finishing before peer threads scheduled. | Made interleaving delay variable `volatile int d` in `threadtest.c`. This forced real memory contention between CPU cores, reliably revealing the race condition. |
| **Compiler Warning `-Werror=unused-but-set-variable`** | xv6 Makefile enforces `-Werror`, failing compilation on unused PID tracker arrays. | Refactored child process harvesting to inspect PID directly from `wait()` return value without intermediate unused buffers. |

---

## 4. Empirical Verification & Test Results

### 4.1 Questions 1 & 2: Race Condition Before vs. After Synchronization

In both tests, 3 threads concurrently performed 100,000 increments each on a shared global variable `counter` (Expected theoretical sum = 300,000).

| Metric | Question 1 (`threadtest.c`) — Unsynchronized | Question 2 (`synctest.c`) — Synchronized via Semaphores |
| :--- | :--- | :--- |
| **Synchronization Primitive** | None (Raw concurrent memory accesses) | Counting Semaphore (`sem_wait` / `sem_signal`) |
| **Theoretical Target** | 300,000 | 300,000 |
| **Observed Final Counter** | **230,819** | **300,000** |
| **Lost Increments** | **69,181 lost updates (-23.06%)** | **0 (100% exact correctness)** |
| **Status** | Race condition conclusively reproduced | Race condition eliminated |

#### Producer-Consumer Problem Verification (`synctest.c`, Part 2)
- **Buffer Size**: 5 items.
- **Total Produced / Consumed**: 15 items.
- **Trace Output**:
  ```text
  [Producer] Produced item 10 | Buffer count = 1/5
  [Producer] Produced item 20 | Buffer count = 2/5
     [Consumer] Consumed item 10 | Buffer count = 1/5
  [Producer] Produced item 30 | Buffer count = 2/5
     [Consumer] Consumed item 20 | Buffer count = 1/5
  ...
     [Consumer] Consumed item 150 | Buffer count = 0/5
  SUCCESS: Producer-Consumer completed without overflow/underflow!
  ```
- **Result**: Buffer fill remained strictly bounded within $[0, 2] \subseteq [0, 5]$, proving zero underflow or overflow.

---

### 4.2 Question 3: Deadlock Detection & Avoidance Results

`deadlock.c` executed 3 formal resource matrix test cases and 1 live xv6 pipe circular-wait simulation:

| Test Case | Scenario Description | Input Matrices Summary | Algorithm Applied | Output Result |
| :--- | :--- | :--- | :--- | :--- |
| **Test 1** | Deadlocked state (circular dependency) | $N=3, M=3$; Available: `[0, 0, 0]` | Deadlock Detection | **Deadlock Detected!** Deadlocked processes: **`P0, P1, P2`** |
| **Test 2** | Safe state | $N=5, M=3$; Available: `[3, 3, 2]` | Banker's Safety Check | **Safe State**. Sequence: **`P1 -> P3 -> P4 -> P0 -> P2`** |
| **Test 3** | Safe request validation | $N=5, M=3$; P0 requests `[0, 2, 0]` | Banker's Avoidance | **Safe**. Tentative allocation checked; request granted |
| **Task 3c** | Real xv6 process & pipe deadlock | 2 forked processes holding opposite pipe ends | Deadlock Detection on Pipe Graph | **Circular Wait Deadlock** correctly identified between Process A and Process B |

---

### 4.3 Question 4: Priority Scheduling Behavior & Aging Analysis

Four CPU-bound processes were launched concurrently with workload $20,000,000$ iterations each:

| Process ID | Assigned Initial Priority | Completion Tick | Execution Duration (Ticks) | Final Dynamic Priority | Completion Order |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **PID 17 (P0)** | **5** (Highest) | 7174 | **539 ticks** | **5** | **1st** |
| **PID 18 (P1)** | **10** (Medium-High) | 8070 | **1183 ticks** | **5** (Aged) | **2nd** |
| **PID 20 (P3)** | **20** (Lowest) | 8336 | **1444 ticks** | **5** (Aged) | **3rd** |
| **PID 19 (P2)** | **15** (Medium-Low) | 8341 | **1453 ticks** | **5** (Aged) | **4th** |

#### Scheduling Behavior Graph (Duration vs. Priority)

```text
Priority 5  (PID 17) [#####] 539 ticks [Finished 1st]
Priority 10 (PID 18) [###########] 1183 ticks [Finished 2nd]
Priority 20 (PID 20) [##############] 1444 ticks [Finished 3rd via Aging]
Priority 15 (PID 19) [##############] 1453 ticks [Finished 4th via Aging]
```

#### Observations:
1. **Priority Ordering**: Process with Priority 5 finished first in only **539 ticks**, outperforming default priority 10 (1183 ticks).
2. **Starvation Prevention via Aging**: Process 3 started at the lowest possible priority (**20**). Under strict priority scheduling without aging, it would starve until all higher priority jobs exited. With aging enabled, every 30 wait ticks in `RUNNABLE` boosted its priority until it reached Priority 5, successfully completing without starvation.

---

## 5. Instructions to Build and Verify

To run the complete automated test suite or individual user programs:

```bash
# 1. Clean and build the xv6 kernel and filesystem image
make clean
make fs.img xv6.img

# 2. Run xv6 in headless QEMU
make qemu-nox

# 3. Inside the xv6 shell ($):
$ threadtest     # Runs Question 1 thread race test
$ synctest       # Runs Question 2 semaphore synchronization & producer-consumer
$ deadlock       # Runs Question 3 deadlock detection, Banker's algorithm, pipe test
$ priotest       # Runs Question 4 priority scheduler and aging test
```

To exit QEMU: press `Ctrl-A` followed by `x`.

---
*Report prepared by Nishchay (2401mc22) for OS Lab Assignment 7.*
