/*!
******************************************************************************
sim.c

The main file of the simulator. This file orchestrates the execution of the 
entire simulation by initializing, coordinating, and managing interactions 
between the various components, including cores, memory, caches, and the bus.

Handles high-level tasks such as:
- Opening and managing required files.
- Initializing components (cores, memory, caches, and the bus).
- Running the simulation loop.
- Finalizing and printing results to output files.
*****************************************************************************/

/* includes */
#include "./MultiCoreProject/headers/FilesManager.h"
#include "./MultiCoreProject/headers/ProcessorCore.h"
#include "./MultiCoreProject/headers/MainMemory.h"
#include "./MultiCoreProject/headers/BusController.h"
#include <string.h>

/* Global Variables */
ProcessorCore cores[NUM_OF_CORES]; // Array of cores

/* static functions */
static void initCores(); // Initialize all cores
static bool isProcessorHalted(); // Check if all cores are halted

/* Functions */
static void initCores(){
    // Initialize all cores
    memset(cores, 0, sizeof(ProcessorCore) * NUM_OF_CORES);
    for (int i = 0; i < NUM_OF_CORES; i++){
        cores[i].fileHandles = coreFileHandlesArray[i]; // Assign the file handles
        ProcessorCore_Init(&cores[i], i); // Initialize the core
    }
}

static bool isProcessorHalted(){
    // Check if all cores are halted
    bool allHalted = true;
    for (int i = 0; i < NUM_OF_CORES; i++){
        if (!core_is_halted(&cores[i])){
            allHalted = false;
        }
    } 
    return allHalted;
}
/* MAIN FUNCTION */
int main(int argc, char* argv[]){
    // open all required files
    if(OpenRequiredFiles(argv, argc) != 0){
        printf("Error opening files\n");
        return 1;
    }

    // Main Memory Initialization
    MainMemoryInit();
    // Core Initialization
    initCores();

    while (!isProcessorHalted()){
        // Run a single cycle for each core
        Run_Bus_Iteration();
        for (int i = 0; i < NUM_OF_CORES; i++){
            core_run_single_cycle(&cores[i]);
        }
    }
    printf("Processor halted\n");
    for (int i = 0; i < NUM_OF_CORES; i++){
        // Shutdown each core
        Core_Shutdown(&cores[i]);
    }
    MainMemoryPrint(MemOut); // Print the contents of the main memory
    closeFiles(); // Close all files
    return 0;
}