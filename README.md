# xv6 OS Lab Assignment 7 — Threads, Semaphores, Deadlock & Priority Scheduling

**Author**: Nishchay  
**Roll Number**: 2401mc22  

This repository contains the complete implementation and test programs for **Operating Systems Lab Assignment 7** on **xv6**.

---

## Features Implemented

### Question 1: Kernel-Supported Threads
- Implemented `clone(void(*fcn)(void*), void *arg, void *stack)` system call that creates a new thread sharing the parent's page directory (`pgdir`) and memory size while allocating a dedicated kernel stack and trapframe.
- Implemented `join(void)` system call that reaps child threads and frees their kernel stack without releasing the shared address space.
- Protected `exit()`, `wait()`, and `fork()` so thread address spaces are not freed prematurely.
- Test program: `threadtest.c` (spawns 3 threads incrementing a shared counter to demonstrate the un-synchronized race condition).

### Question 2: Process/Thread Synchronization (Counting Semaphores)
- Implemented counting semaphore table `semtable[10]` protected by spinlocks.
- Implemented system calls:
  - `int sem_init(int sem_id, int value)`
  - `int sem_wait(int sem_id)`
  - `int sem_signal(int sem_id)`
- Uses selective process wakeups (`wakeup_one`) to ensure exact semaphore semantics.
- Test program: `synctest.c`:
  - **Part 1**: Wraps the counter increment in `sem_wait`/`sem_signal` to achieve an exact final count of 300,000.
  - **Part 2**: Solves the classic Bounded-Buffer Producer-Consumer problem with buffer size 5 using `empty`, `full`, and `mutex` semaphores.

### Question 3: Deadlock Detection & Banker's Algorithm
- Test program: `deadlock.c`:
  - Implements matrix-based deadlock detection algorithm (`detect_deadlock`).
  - Implements deadlock avoidance using Banker's algorithm (`is_safe_state`).
  - Evaluates 3 test matrices (deadlocked state, safe state with safe sequence, and unsafe request detection).
  - Simulates circular wait deadlock using real xv6 processes and pipes (Task 3c).

### Question 4: Priority-Based Preemptive CPU Scheduling with Aging
- Extended `struct proc` with `priority` (1 = highest, 20 = lowest, default = 10) and `wait_ticks`.
- Implemented system calls:
  - `int set_priority(int pid, int priority)`
  - `int get_priority(int pid)`
- Rewrote `scheduler()` in `proc.c` to always dispatch the `RUNNABLE` process with lowest priority number, breaking ties in round-robin order.
- Implemented dynamic aging in timer interrupts: if a `RUNNABLE` process waits $\ge 30$ ticks, its priority number is decremented by 1.
- Test program: `priotest.c` (forks 4 CPU-bound processes with distinct priorities 5, 10, 15, 20 and demonstrates earlier completion for higher priorities and starvation prevention for priority 20).

---

## File Structure

- `proc.c`, `proc.h`: Thread creation/reaping, semaphores, priority scheduler, aging mechanism.
- `syscall.c`, `syscall.h`, `sysproc.c`: System call definitions, dispatchers, handlers.
- `defs.h`, `user.h`, `usys.S`: Kernel prototypes and user space wrappers.
- `trap.c`: Aging tick hook on timer interrupts.
- `threadtest.c`: Test program for Question 1.
- `synctest.c`: Test program for Question 2.
- `deadlock.c`: Test program for Question 3.
- `priotest.c`: Test program for Question 4.
- `REPORT.md`: Comprehensive 2–3 page consolidated report.

---

## Build and Run Instructions

```bash
# Build kernel and disk images
make clean
make

# Run in QEMU (headless mode)
make qemu-nox

# Inside the xv6 shell:
$ threadtest
$ synctest
$ deadlock
$ priotest
```

To exit QEMU: press `Ctrl-A` then `x`.
