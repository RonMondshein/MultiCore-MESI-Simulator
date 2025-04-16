/*!
************************************************************
file MainMemory.c

Implementation of the main memory for the multicore processor simulator.

This file defines the main memory of size 2^20 bytes and provides initialization,
transaction handling via the bus and printing.
************************************************************/

/* Includes */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#endif
#include "../headers/MainMemory.h"
#include "../headers/BusController.h"
#include "../headers/FilesManager.h"

/* Global Variables */
static uint32_t mainMemory[MAIN_MEMORY_SIZE]; // Main memory of size 2^20 bytes
static uint32_t numOfCycles = 0; // Number of cycles taken by the main memory
static bool gIsMemoryBusy = false; // Flag to indicate if the memory is busy
uint32_t totalLines = 0; // Line in the program

/* Static Functions */
static size_t countMemoryLines(void); 
static bool initialize_memory_transaction(bool direct_transaction);
static bool process_memory_command(bus_transaction* transactionet);
static void valuesToChange(bus_transaction* transaction);
static bool bus_transaction_handler(bus_transaction* packet, bool direct_transaction);

/*Functions implementations*/
void MainMemoryInit() {
    // Initialize the main memory to the values from the input file. The rest of the memory is set to 0.
    memset((uint8_t*)mainMemory, 0, sizeof(mainMemory));

    // Read memory data from MemIn and load it into mainMemory
    for (totalLines = 0; totalLines < MAIN_MEMORY_SIZE; totalLines++) {
        if (fscanf(MemIn, "%08x", (uint32_t*)&(mainMemory[totalLines])) == EOF) {
            break;  // Exit the loop if end of file is reached
        }
}
    ConfigureMemoryCallback_for_bus(bus_transaction_handler); // Register the memory callback function.
}


static size_t countMemoryLines(void) {
    // Traverse the memory in reverse to count non-empty lines.
    for (size_t i = MAIN_MEMORY_SIZE; i-- > 0;) {
        if (mainMemory[i] != 0) { // Check if the memory location is not empty.
            return i + 1; // Return the total number of used lines.
        }
    }
    return 0; // Return 0 if no non-zero values are found.
}


static bool initialize_memory_transaction(bool direct_transaction) {
    //This function initializes the transaction state.
    if (!gIsMemoryBusy) {
        gIsMemoryBusy = true;
        if (direct_transaction) { 
            numOfCycles = 16; 
        } else {
            numOfCycles = 0; 
        }
    }
    return gIsMemoryBusy;
}


static bool process_memory_command(bus_transaction* transaction) { 
    // Process the memory command based on its type.
    if (transaction->bus_cmd == busRd || transaction->bus_cmd == busRdX)

		{
			// send the memory value
			transaction->origid = main_memory;
			transaction->bus_cmd = flush;
			transaction->bus_data = mainMemory[transaction->bus_addr];
		}
		else if (transaction->bus_cmd == flush)
		{
			// write data to memory
			mainMemory[transaction->bus_addr] = transaction->bus_data;
		}
    return true;
}




static void valuesToChange(bus_transaction* transaction){ 
    //This function changes the values of the packet.
    transaction->origid = main_memory;
    transaction->bus_cmd = flush;
    transaction->bus_data = mainMemory[transaction->bus_addr];

}


static bool bus_transaction_handler(bus_transaction* transaction, bool direct_transaction) { 
    //This function handles the bus transaction for the main memory.
    if (transaction->bus_cmd == no_cmd) {
        return false; // No command to process
    }
    // Initialize transaction if needed
    initialize_memory_transaction(direct_transaction);
    
    // Check if the transaction delay has been satisfied.
    if (numOfCycles >= 16) {
        // Process the memory command.
        process_memory_command(transaction);

        // Mark transaction complete after 19 cycles.
        if (numOfCycles == 19) {
            gIsMemoryBusy = false;
        }

        numOfCycles++;
        return true; // Transaction in progress or completed.
    }

    numOfCycles++; // Increment delay counter.
    return false;  // Waiting for delay to complete.
}


void MainMemoryPrint(FILE* file) {
    // Print the main memory contents in hexadecimal format.
    uint32_t currentLines = 0;
    currentLines = countMemoryLines();
    for (uint32_t i = 0; i < currentLines; i++) {
        fprintf(file, "%08X\n", mainMemory[i]);
    }
}
