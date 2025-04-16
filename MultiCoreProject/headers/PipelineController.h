/*!
******************************************************************************
file PipelineController.h

This is the header file for the PipelineController class.

*****************************************************************************/



#ifndef __PipelineController_H__
#define __PipelineController_H__


// include the necessary files
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "CacheController.h"
#include "OpcodeHandlers.h"
#include "sim.h" 

// define the necessary data structures for the pipeline


// An enum that represents the different states of the pipeline
typedef enum 
{
    FETCH = 0,
    DECODE,
    EXECUTE,
    MEM,
    WRITE_BACK,

    PIPE_SIZE
} Pipe_figstate;

//A struct that represents a stage in the pipeline - 
//it contains the state of the stage, the program counter, the instruction, the result of the execution
//and the operation to be performed

typedef struct
{
	Pipe_figstate state;
	uint16_t pc;
	Format_of_instruction instruction;
	uint32_t result_of_execution;
	void (*operation)(OpcodeParams* params);
} Pipe_instruction_stage;


// A struct that represents the statistics of the pipeline -
// it contains the number of decode stalls and memory stalls

typedef struct
{
	uint32_t stalls_in_decode;
	uint32_t stalls_in_mem;
} Pipe_Stats;

// A struct that represents the pipeline - it contains the halted flag, the data hazard stall flag, the memory stall flag, 
// the pointers to the instructions and the core registers, the cache data, the stages of the pipeline,
// the opcode parameters and the statistics

typedef struct
{
	bool is_halted;
	bool data_stall;
	bool mem_stall;
	uint32_t *insturcionts_pnt;
	uint32_t* regs_pnt;
	Cache_Data data_in_cache;
	Pipe_instruction_stage stages_in_pipe[PIPE_SIZE];
	OpcodeParams params_of_op;
	Pipe_Stats stats;
}Pipe_fig;


// function prototypes for the pipeline

// initialize the pipeline
void Pipe_Init(Pipe_fig *pipeline);

// execute the stages of the pipeline
void Pipe_iteration_exe(Pipe_fig* pipeline);

// print the pipeline information to the trace file
void Pipe_ToTrace(Pipe_fig* pipeline, FILE *trace_file); 

// put bubbles in the pipeline where needed
void Pipe_Bubbles(Pipe_fig* pipeline);
void BubbleStage(Pipe_instruction_stage* dest, const Pipe_instruction_stage* src);

// flush all the stages of the pipeline
bool Pipe_Flush(Pipe_fig* pipeline);


#endif // __PipelineController_H__