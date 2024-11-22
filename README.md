# OS Scheduler (University Project)

**Disclaimer:** This is not a real operating system scheduler but rather a simulation developed as part of a university project for the **Operating Systems (CIE 302)** course at Zewail City of Science and Technology. The primary purpose of this project is to explore and evaluate different CPU scheduling algorithms through simulation.

---

## Project Objectives

- Evaluate and compare the performance of various CPU scheduling algorithms.
- Implement **Inter-Process Communication (IPC)** techniques.
- Demonstrate effective use of algorithms and data structures.

---

## Features

1. **Scheduling Algorithms**:
   - Short Job First (SJF).
   - Preemptive Highest Priority First (PHPF).
   - Round Robin (RR).

2. **Process Lifecycle Simulation**:
   - Creation, execution, suspension, and termination of processes.
   - Process states tracked: Running, Ready, Blocked.

3. **Performance Metrics**:
   - CPU Utilization.
   - Average Weighted Turnaround Time (WTA).
   - Average Waiting Time.
   - Standard Deviation of Average WTA.

4. **Output**:
   - Detailed logs of process execution (`scheduler.log`).
   - Summary performance metrics (`scheduler.perf`).

---

## System Components

1. **Process Generator**:
   - Reads process data from an input file.
   - Initiates the scheduler and clock processes.
   - Manages process arrival times and IPC communication.

2. **Clock**:
   - Simulates an integer time clock to synchronize processes.

3. **Scheduler**:
   - Implements the scheduling algorithms.
   - Manages process control blocks (PCBs) for tracking states, times, and priorities.
   - Generates performance metrics.

4. **Processes**:
   - Simulated CPU-bound tasks that notify the scheduler upon termination.

---

## Input and Output

- **Input File** (`processes.txt`):
  - Describes processes with fields: ID, arrival time, runtime, priority.

- **Output Files**:
  - `scheduler.log`: Step-by-step execution details.
  - `scheduler.perf`: Overall performance metrics.

---

## How to Use

1. Compile the code using your preferred IDE or command-line tools (e.g., `make`).
2. Run the `process_generator` program and choose the desired scheduling algorithm.
3. Analyze the outputs (`scheduler.log` and `scheduler.perf`) for performance insights.

---

**Developed for educational purposes by students of the Communication and Information Engineering Program.**
