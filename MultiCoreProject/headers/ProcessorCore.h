#ifndef ProcessorCore_H
#define ProcessorCore_H

/* Includes */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FilesManager.h"
#include "PipelineController.h"

/* typedef & Consts */
/* Defines */
#define REGISTERCOUNT 16 // 16 registers per core
#define INSTRUCTIONMEMORYSIZE 1024 // 1K words

/* typedef */
typedef struct{
    uint32_t cycles; // Number of cycles
    uint32_t instructions; // Number of instructions    
} tracking_info_core;

typedef struct {
    uint32_t coreId; // Between 0 and 3
    uint32_t pc; // Program Counter, 10 bits as the address space is 1K words long
    uint32_t registers[REGISTERCOUNT]; // 16 registers, each register is 32 bits
    uint32_t instruction_memory[INSTRUCTIONMEMORYSIZE]; // 1K words, each word is 32 bits
    CoreFileHandles fileHandles; // File handles for the core
    Pipe_fig pipelineController;
    bool isHalted; // Flag to indicate if the core is halted
    tracking_info_core tracking_info_core;
} ProcessorCore;

/* Functions Prototypes */
void ProcessorCore_Init(ProcessorCore* core, uint32_t coreId);
void core_run_single_cycle(ProcessorCore* c);
void Core_Shutdown(ProcessorCore* core);
bool core_is_halted(ProcessorCore* core);


#endif // ProcessorCore_H