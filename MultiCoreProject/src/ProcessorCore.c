/*!
******************************************************************************
file ProcessorCore.c
Implementation of Processor Core functionality.

*/

/* Includes */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#endif
#include "../headers/ProcessorCore.h"
#include "../headers/MainMemory.h"
#include "../headers/PipelineController.h"


/* Defines */
#define REGISTERCOUNT 16 // 16 registers per core
#define INSTRUCTIONMEMORYSIZE 1024 // 1K words

/* Functions Prototypes */
static void Print_tracking_info(ProcessorCore* core);
static void Print_registers(ProcessorCore* core);
static int InstMem_init(ProcessorCore* core);
static void write_trace(ProcessorCore *core, uint32_t* reg);
static void write_trace_reg(ProcessorCore* core, uint32_t* reg);
static void update_tracking_info(ProcessorCore* core);


/* Functions implementations */
void ProcessorCore_Init(ProcessorCore* core, uint32_t coreId){
    // Initialize the core
    core->pc = 0; // Initialize the program counter to 0
    core->isHalted = false; // Initialize the halt flag to false
    core->coreId = coreId; // Set the core ID

    memset(&core->registers, 0, sizeof(REGISTERCOUNT)); // Initialize registers to 0
    int num_loaded_instructions = InstMem_init(core); // Load instructions from file
    if (num_loaded_instructions == 0) {
        core->isHalted = true; // Halt the core if no instructions are loaded
        return;
    }

    // Initialize the Pipeline Controller
    memset(&core->pipelineController, 0, sizeof(PIPE_SIZE));
    Pipe_Init(&core->pipelineController);
    // Initialize the cache
    memset(&core->pipelineController.data_in_cache, 0, sizeof(Cache_Data)); 
    CacheController_Init(&core->pipelineController.data_in_cache, coreId);
    // Initialize the tracking info
    memset(&core->tracking_info_core, 0, sizeof(tracking_info_core));
    core-> tracking_info_core.cycles = -1; 

    Cache_InitializeBusCallbacks();

    core->pipelineController.regs_pnt = core->registers;
	core->pipelineController.insturcionts_pnt = core->instruction_memory;
	core->pipelineController.params_of_op.pc = (uint16_t *)&(core->pc);
}

static int InstMem_init(ProcessorCore* core){
    int loaded_instructions = 0; // Tracks the number of loaded instructions

    // Loop until we reach the memory size limit or end of the input file
    while (loaded_instructions < INSTRUCTIONMEMORYSIZE) 
    {
        // Read the next instruction from the file
        int read_status = fscanf(
            core->fileHandles.instructionMemoryFile, 
            "%08x", 
            (uint32_t*)&(core->instruction_memory[loaded_instructions])
        );

        // Stop if the end of file is reached
        if (read_status == EOF) {
            break;
        }

        // Increment the count of successfully loaded instructions
        loaded_instructions++;
    }

    return loaded_instructions;
}
void core_run_single_cycle(ProcessorCore* core){
    // Run the core for single cycle
    if (core_is_halted(core)) { return; } // Do nothing if the core is halted
    if (Pipe_Flush(&core->pipelineController)) { 
        // Flush the pipeline if needed, halt the core if the pipeline is flushed and return
        core -> isHalted= true;
        return;
    }
    // make a copy of the registers
    uint32_t regC[REGISTERCOUNT];
    memcpy(regC, core->registers, sizeof(core->registers));

    update_tracking_info(core); // Update the performance statistics
    Pipe_iteration_exe(&core->pipelineController); // Run the pipeline for a single cycle
    write_trace(core, regC); 
    Pipe_Bubbles(&core->pipelineController); // Bubble the pipeline stages if needed
}

bool core_is_halted(ProcessorCore* core){
    // Check if the core is halted
    return core->isHalted;
}

static void write_trace(ProcessorCore *core, uint32_t* reg){
    // Write the trace to the file
    fprintf(core->fileHandles.executionTraceFile, "%d ", core->tracking_info_core.cycles);
    Pipe_ToTrace(&core->pipelineController, core->fileHandles.executionTraceFile);
    write_trace_reg(core, reg);
    fprintf(core->fileHandles.executionTraceFile, "\n");
}

static void write_trace_reg(ProcessorCore* core, uint32_t* reg){
    // Write the register values to the trace file
    if (reg == NULL) {
        reg = core->registers;
    }
    for (int i = START_MUTABLE_REG; i < REGISTERCOUNT; i++) {
        fprintf(core->fileHandles.executionTraceFile, "%08X ",reg[i]);
    }
}

static void update_tracking_info(ProcessorCore* core){
    // Update the performance statistics for the core
    core->tracking_info_core.cycles++;
    if (!core->pipelineController.is_halted && !core->pipelineController.mem_stall && !core->pipelineController.data_stall)
    {
        core->tracking_info_core.instructions++;
    }
    
}

void Core_Shutdown(ProcessorCore* core){
    // Shutdown the core
    Print_registers(core); // Output the register values to the trace file
    // Output the cache data (dsram and tsram) to their respective files
    print_Cache_Data(
        &core->pipelineController.data_in_cache,       // Cache data structure
        core->fileHandles.dataCacheFile,      // Output file for data section
        core->fileHandles.tagCacheFile      // Output file for tag section
    );

    Print_tracking_info(core);// Log the performance statistics for the core
}

static void Print_registers(ProcessorCore* core){
    // Print the register values for the core into the trace file
    for (int i = START_MUTABLE_REG; i < REGISTERCOUNT; i++) {
        fprintf(core->fileHandles.registerOutputFile, "%08X\n", core->registers[i]);
    }
}
static void Print_tracking_info(ProcessorCore* core){
    // Print the performance statistics for the core into the stats file
    fprintf(core->fileHandles.coreStatsFile, "cycles %d\n", core->tracking_info_core.cycles + 1); // +1 to account for the initial cycle //TODO: check if +1 is needed
    fprintf(core->fileHandles.coreStatsFile, "instructions %d\n", core->tracking_info_core.instructions - 1); // -1 to account for the initial cycle //TODO: check if -1 is needed
    fprintf(core->fileHandles.coreStatsFile, "read_hit %d\n", core->pipelineController.data_in_cache.tracking_info.read_hits);
    fprintf(core->fileHandles.coreStatsFile, "write_hit %d\n", core->pipelineController.data_in_cache.tracking_info.write_hits); 
    fprintf(core->fileHandles.coreStatsFile, "read_miss %d\n", core->pipelineController.data_in_cache.tracking_info.read_misses);
    fprintf(core->fileHandles.coreStatsFile, "write_miss %d\n", core->pipelineController.data_in_cache.tracking_info.write_misses);
    fprintf(core->fileHandles.coreStatsFile, "decode_stall %d\n", core->pipelineController.stats.stalls_in_decode);
    fprintf(core->fileHandles.coreStatsFile, "mem_stall %d\n", core->pipelineController.stats.stalls_in_mem);
}

