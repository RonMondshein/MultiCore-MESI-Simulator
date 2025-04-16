#ifndef CACHECONTROLLER_H
#define CACHECONTROLLER_H

/* Includes */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "./BusController.h"

/* Defines */
#define CACHE_SIZE 256 // 256 words
#define BLOCK_SIZE 4  // 4 words
#define NUM_BLOCKS (CACHE_SIZE / BLOCK_SIZE) // 64 blocks as 256/4 = 64

/* Types & Consts*/
typedef enum {
    // MESI protocol states
    MESI_STATE_INVALID,
    MESI_STATE_SHARED,
    MESI_STATE_EXCLUSIVE,
    MESI_STATE_MODIFIED,

    number_of_states //TODO: check if this is needed
    }MESIState;

typedef enum {
    // Cache IDs for each core among the 4 cores
    CACHE_ID_CORE0,
    CACHE_ID_CORE1,
    CACHE_ID_CORE2,
    CACHE_ID_CORE3
}Cache_Id_enum;

typedef struct {
    uint16_t tag : 12;   // 12 bits for the tag
    uint8_t mesi : 2;    // 2 bits for MESI state
} TSRAMLine;

typedef struct {
    uint32_t data;
} DRAMLine;

typedef struct {
    // used to track the cache performance
    uint32_t read_hits;
    uint32_t read_misses;
    uint32_t write_hits;
    uint32_t write_misses;
} tracking_info;

typedef struct {
    Cache_Id_enum id; // Cache ID: 0-3
    TSRAMLine tsram[NUM_BLOCKS]; // Tag and state SRAM
    DRAMLine dram[CACHE_SIZE]; // Data SRAM
    tracking_info tracking_info; // Cache performance tracking
    bool isStalled; // Flag to indicate if the cache is stalled
} Cache_Data;



/* Functions Prototypes */
void CacheController_Init(Cache_Data *cache_data, Cache_Id_enum id);
void Cache_InitializeBusCallbacks(void);
void print_Cache_Data(Cache_Data* cache_data, FILE* file_dram, FILE* file_tsram);
bool Write_Data_to_Cache(Cache_Data* cache_data, uint32_t address, uint32_t data);
bool Read_Data_from_Cache(Cache_Data* cache_data, uint32_t address, uint32_t* data);


#endif // CACHECONTROLLER_H_