#ifndef MAINMEMORY_H
#define MAINMEMORY_H

#include "./sim.h"
#include <stdio.h>
#define MAIN_MEMORY_SIZE (1 << 20) // 2^20


typedef struct {
    // used to track the main memory performance
    uint8_t offset : 2;  // 2 bits for index, 4 words per block
    uint32_t block : 18;  // 18 bits left for block
} AddressFields;

typedef union {
    uint32_t address;       // Raw 20-bit address
    AddressFields fields;   // Decomposed fields
} MemoryAddress;

void MainMemoryInit(); // Initialize the main memory
void MainMemoryPrint(FILE* file); // Print the main memory contents

#endif // MAINMEMORY_H_
