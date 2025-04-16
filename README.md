# MultiCore-MESI-Simulator

A cycle-accurate simulator for a 4-core processor architecture in C, featuring:
- 5-stage instruction pipeline per core (Fetch, Decode, Execute, Mem, Write Back)
- Direct-mapped data caches (write-back, write-allocate)
- MESI cache coherency via a shared Bus
- Main memory handling and bus arbitration
- Full trace and stats output per core

## Features
- Instruction and data memory per core
- Cache simulation with MESI protocol
- Pipeline control including stalls and hazard resolution
- File-based input/output handling for simulation traces, registers, memory, and stats

## Architecture
        +------------+    +------------+    +------------+    +------------+
        |  Core 0    |    |  Core 1    |    |  Core 2    |    |  Core 3    |
        |  IMEM      |    |  IMEM      |    |  IMEM      |    |  IMEM      |
        |  Cache     |    |  Cache     |    |  Cache     |    |  Cache     |
        +------------+    +------------+    +------------+    +------------+
                 \             |         |           /
                       +--------------------+
                       |        BUS         |
                       |     (MESI)         |
                       +--------+-----------+
                                |
                         +-------------+
                         | Main Memory |
                         +-------------+


## 📁 Project Structure
Each `.c` file is located in the `src/` directory and has a corresponding `.h` header file located in the `headers/` directory.

| File                 | Description                                         |
|----------------------|-----------------------------------------------------|
| `BusController.c`     | Manages bus arbitration and inter-core transactions |
| `CacheController.c`   | Implements per-core cache logic with MESI protocol  |
| `MainMemory.c`        | Simulates shared main memory with latency modeling  |
| `FilesManager.c`      | Handles file I/O for simulation inputs and outputs  |
| `OpcodeHandlers.c`    | Contains ALU, branching, and memory instruction logic |
| `PipelineController.c`| Implements a 5-stage instruction pipeline per core  |
| `ProcessorCore.c`     | Coordinates core execution and pipeline lifecycle   |
                         
## 🧪 Assembly Tests

The `asm tests/` directory includes custom-written assembly programs used to verify the correctness and performance of the simulator. These test cases are designed to evaluate various scenarios such as serial and parallel vector addition and multi-core synchronization. Each folder contains input files, memory initialization, and expected outputs.

- `addserial/` – Performs vector addition using a single core sequentially.
- `addparallel/` – Splits vector addition across multiple cores for parallel execution.
- `counter/` – Implements a shared counter updated cooperatively by all cores in a round-robin fashion.


## Build Instructions
To compile:

gcc -o sim.exe *.c
To run:
./sim.exe imem0.txt imem1.txt imem2.txt imem3.txt memin.txt memout.txt \
regout0.txt regout1.txt regout2.txt regout3.txt core0trace.txt core1trace.txt \
core2trace.txt core3trace.txt bustrace.txt dsram0.txt dsram1.txt dsram2.txt \
dsram3.txt tsram0.txt tsram1.txt tsram2.txt tsram3.txt stats0.txt stats1.txt \
stats2.txt stats3.txt

## 📄 Documentation

For full project requirements and specifications, see the [Documentation PDF](./Documentation.pdf).


