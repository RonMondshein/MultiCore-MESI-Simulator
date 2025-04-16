/*!
******************************************************************************
file CacheController.c

Implementation of Cache functionality with MESI protocol.

This file contains the implementation of cache operations, including read, write,
and coherency management using the MESI protocol. The cache interacts with the 
Bus and Main Memory to ensure data consistency.

Each core have a direct mapped cache, it size is 256 words, each block contain 4 words, there are 256/4 = 64 blocks.
The cache is write back and write allocate.
The cach use 2 SRAMS: 
1- DRAM: 32 bits width and 256 words depth for data.
2- TSRAM: contains 64 lines, each line contain tag and state of the MESI protocol.
*****************************************************************************/

/* Includes */
#include "../headers/CacheController.h"
#include "../headers/BusController.h"
#include "../headers/MainMemory.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    uint32_t offset : 2; // 2 bits for offset
    uint32_t index : 6;  // 6 bits for index
    uint32_t tag : 12;  // 12 bits for tag
} CacheAddressFields;

typedef union {
    uint32_t address;       // Raw 32-bit address
    CacheAddressFields fields;   // Decomposed fields
} CacheAddressInfo;

typedef Cache_Id_enum(*states_machine)(Cache_Data* data, bus_transaction* transaction); // Function pointer to the current state

/*Functions prototypes*/
static bool is_cache_busy(Cache_Data* cache_data);
static bool shared_or_modified_handler(void *cache, bus_transaction* transaction,  bool* is_modified);
static bool snooping_handler(void* cache, bus_transaction* transaction, uint8_t  address_offset);
static bool snooped_transaction(Cache_Data* cache_data, TSRAMLine* tsram_line, bus_transaction* transaction, CacheAddressInfo c_addr, uint8_t address_offset);
static bool is_block_valid_and_matching(TSRAMLine* tsram_line, uint16_t tag);
static bool cache_response_handle(void* data, bus_transaction* transaction, uint8_t* address_offset);
static void flush_data(Cache_Data* cache_data, bus_transaction* transaction);
static Cache_Id_enum MSEI_invalid (Cache_Data* cache_data, bus_transaction* transaction);
static Cache_Id_enum MSEI_shared (Cache_Data* cache_data, bus_transaction* transaction);
static Cache_Id_enum MSEI_exclusive (Cache_Data* cache_data, bus_transaction* transaction);
static Cache_Id_enum MSEI_modified (Cache_Data* cache_data, bus_transaction* transaction);
static bool readHit(Cache_Data* cache_data, CacheAddressInfo addr, uint32_t* data, bool miss_occurred_read);
static void handle_dirty_block(Cache_Data* cache_data, TSRAMLine* tsram_line, CacheAddressInfo addr);
static void handle_transaction(Cache_Data* cache_data, CacheAddressInfo addr, cmd_on_the_bus b_cmd);

static states_machine state_handler[number_of_states] = {
    // State machine for the cache controller
    MSEI_invalid,
    MSEI_shared,
    MSEI_exclusive,
    MSEI_modified
};


/*Functions implementations*/
void CacheController_Init(Cache_Data *cache_data, Cache_Id_enum id){
    // Initialize the cache
    memset((uint32_t *)cache_data, 0, sizeof(Cache_Data)); // Clear the cache data
    cache_data->id = id; // Set the cache ID

    cache_data->tracking_info.read_hits = 0;
    cache_data->tracking_info.read_misses = 0;
    cache_data->tracking_info.write_hits = 0;
    cache_data->tracking_info.write_misses = 0;

    //bus initialization
    Bus_core_cache cache_interface = { .core_id = id, .bus_cache_data = cache_data };
    Bus_InitializeCache(cache_interface);
}


static bool is_cache_busy(Cache_Data* cache_data) {
    // Check if the cache is busy
    return IsBusInTransaction((Bus_transaction_caller)cache_data->id) || 
        IsBusWaitForTransaction((Bus_transaction_caller)cache_data->id);
}


static bool readHit(Cache_Data* cache_data, CacheAddressInfo addr, uint32_t* data, bool miss_occurred_read) {
    // Read hit: retrieve data from cache.
      *data = cache_data->dram[addr.fields.index * BLOCK_SIZE + addr.fields.offset].data; // Read the data from the cache
        // Update statistics for a read hit.
        if (!miss_occurred_read) { 
            cache_data->tracking_info.read_hits++;
        } else {
            miss_occurred_read = false;
        }
        return miss_occurred_read;
}


static void handle_dirty_block(Cache_Data* cache_data, TSRAMLine* tsram_line, CacheAddressInfo addr) {
    // Handle dirty block if necessary.
    if (tsram_line->mesi == MESI_STATE_MODIFIED) {
        uint32_t evict_addr = (tsram_line->tag << 8) | (addr.fields.index << 2); // Calculate the address to evict
        uint32_t evict_data = cache_data->dram[addr.fields.index * BLOCK_SIZE + addr.fields.offset].data;
        bus_transaction evict_transaction = {
            .origid = (Bus_transaction_caller)cache_data->id,
            .bus_cmd = flush,
            .bus_addr = evict_addr,
            .bus_data = evict_data,
            .bus_shared = 0
        };
        evict_transaction.bus_data = evict_data;
        AddTransaction_to_bus(evict_transaction);
    }
}


static void handle_transaction(Cache_Data* cache_data, CacheAddressInfo addr, cmd_on_the_bus b_cmd) {
    // Initiate read transaction to fetch data from memory.
    bus_transaction transaction = {
        .origid = (Bus_transaction_caller)cache_data->id,
        .bus_cmd = b_cmd,
        .bus_addr = addr.address,
        .bus_data = 0,
        .bus_shared = 0
    };

    AddTransaction_to_bus(transaction); // Add the transaction to the bus
}


/*
* Read_Data_from_Cache*
*/
bool Read_Data_from_Cache(Cache_Data* cache_data, uint32_t address, uint32_t* data) {
    static bool miss_occurred_read = false; // Distinct between read hit to hit after miss that doesn't count as read hit
    uint32_t index = 0;
    CacheAddressInfo addr;
    TSRAMLine* tsram_line; 

    // Step 1: Check if the cache is busy
    if (is_cache_busy(cache_data))
        return false;

    // Parse the address into fields.
    addr.address = address;
    index = addr.fields.index;

    // Step 2: Check for a cache hit.
    tsram_line = &(cache_data->tsram[index]);

    if (is_block_valid_and_matching(tsram_line, addr.fields.tag)) {
        // Read hit: retrieve data from cache.
        miss_occurred_read = readHit(cache_data, addr, data, miss_occurred_read);
        return true;
    }

    // Step 3: Handle a cache miss.
    cache_data->tracking_info.read_misses++;
    miss_occurred_read = true;
    
    // Handle dirty block if necessary.
    handle_dirty_block(cache_data, tsram_line, addr);
 
    // Initiate read transaction to fetch data from memory.
    handle_transaction(cache_data, addr, busRd);

    return false; 
}


static bool handle_share_state(Cache_Data* cache_data, CacheAddressInfo addr) {
    // Initiate write transaction for exclusive ownership.
    handle_transaction(cache_data, addr, busRdX);

    // Add an invalid transaction for delay.
    bus_transaction invalid_transaction = {
        .origid = invalid_caller
    };
    AddTransaction_to_bus(invalid_transaction);

    cache_data->tracking_info.write_misses++;
    return true;
}


void update_cache_write(Cache_Data *cache_data, CacheAddressInfo addr, uint32_t data, bool *miss_occurred_write) {
    // Update statistics for a write hit
    if (!*miss_occurred_write) {
        cache_data->tracking_info.write_hits++;
    } else {
        *miss_occurred_write = false;
    }

    // Write data to cache and update MESI state
    uint32_t offset = addr.fields.offset;
    uint32_t index = addr.fields.index;
    uint32_t data_addr = index * BLOCK_SIZE + offset;
    cache_data->dram[data_addr].data = data;
    cache_data->tsram[index].mesi = MESI_STATE_MODIFIED;
}


/*
* Write_Data_to_Cache*
*/
bool Write_Data_to_Cache(Cache_Data* cache_data, uint32_t address, uint32_t data) {
    static bool miss_occurred_write = false; // Track write misses distinctly.
    uint32_t index = 0;
    uint32_t offset = 0;
    CacheAddressInfo addr;
    TSRAMLine* tsram_line;

    // Step 1: Check if the cache is busy.
    if (is_cache_busy(cache_data)) {
        return false;
    }

    // Parse the address into fields.
    addr.address = address;
    index = addr.fields.index;

    // Step 2: Check for a cache hit.
    tsram_line = &(cache_data->tsram[index]);

    if (is_block_valid_and_matching(tsram_line, addr.fields.tag)) {
        // Handle shared state by upgrading to exclusive.
        if (tsram_line->mesi == MESI_STATE_SHARED) {
            miss_occurred_write = handle_share_state(cache_data, addr);
            return false;
        }

        update_cache_write(cache_data, addr, data, &miss_occurred_write);
        return true;
    }

    // Step 3: Handle a cache miss.
    cache_data->tracking_info.write_misses++;
    miss_occurred_write = true;

    // Handle dirty block if necessary.
    handle_dirty_block(cache_data, tsram_line, addr);
    // Initiate write transaction for exclusive ownership.
    handle_transaction(cache_data, addr, busRdX);

    return false;
}


/* Bus - Cache Callbacks begins */
void Cache_InitializeBusCallbacks(void){
    // Initialize the bus callbacks
    ConfigureCacheCallbacks_for_bus(  
        shared_or_modified_handler,    // Callback for shared or modified blocks
        snooping_handler,   // Callback for snooping Bus transactions
        cache_response_handle    // Callback for handling Bus responses
    );
}


static bool shared_or_modified_handler(void *cache, bus_transaction* transaction,  bool* is_modified) {
    Cache_Data *cache_data = (Cache_Data *)cache;  // Cast to the original type
    CacheAddressInfo c_addr;
    uint32_t index = 0;
    TSRAMLine* tsram_line;

    if((Cache_Id_enum)cache_data->id == (Cache_Id_enum)transaction->origid){
        return false; // Ignore the transaction from the same core
    }

    c_addr.address = transaction->bus_addr; // Parse the address fields 

    CacheAddressInfo address = { .address = c_addr.address };
    index = address.fields.index;
    tsram_line = &(cache_data->tsram[index]); // Get the TSRAM line 
    if (tsram_line->mesi == MESI_STATE_MODIFIED) {
        *is_modified = true;
    }
    return is_block_valid_and_matching(tsram_line, address.fields.tag);

}


static bool snooping_handler(void* cache, bus_transaction* transaction, uint8_t  address_offset) {
    Cache_Data *cache_data = (Cache_Data *)cache;
    CacheAddressInfo c_addr;
    uint32_t index = 0;
    TSRAMLine* tsram_line;

    c_addr.address = transaction->bus_addr; // Parse the address fields 
    index = c_addr.fields.index;
    tsram_line = &(cache_data->tsram[index]); // Get the TSRAM line 
    
    if((Bus_transaction_caller)cache_data->id == transaction->original_caller && transaction->bus_cmd != flush) {
        return false; // Ignore the transaction from the same core
    }
    return snooped_transaction(cache_data, tsram_line, transaction, c_addr, address_offset);
}


static bool snooped_transaction(Cache_Data* cache_data, TSRAMLine* tsram_line, bus_transaction* transaction, CacheAddressInfo c_addr, uint8_t address_offset) {
    // This function snoops on Bus transactions to update the cache’s MESI state.
    if (tsram_line->tag != c_addr.fields.tag ||tsram_line->mesi == MESI_STATE_INVALID) { 
        return false; // Cache line is invalid or does not match
    }

    Cache_Id_enum next = state_handler[tsram_line->mesi](cache_data, transaction); // Get the next state

    if ((address_offset == BLOCK_SIZE - 1) || (tsram_line->mesi != MESI_STATE_MODIFIED)) { // Check if the transaction is complete and the block is not modified
        tsram_line->mesi = next; // Update the MESI state
    }

    return true;
}


static bool is_block_valid_and_matching(TSRAMLine* tsram_line, uint16_t tag) {
    // Check if the block is valid and matches the requested tag
    return tsram_line->tag == tag && tsram_line->mesi != MESI_STATE_INVALID;
}


static bool cache_response_handle(void* data, bus_transaction* transaction, uint8_t* address_offset) {
    Cache_Data *cache_data = (Cache_Data *)data;
    uint32_t index = 0;
    TSRAMLine* tsram_line;
    
    if ((Bus_transaction_caller)cache_data->id == transaction->origid && transaction->bus_cmd != flush) {
        return false; // Ignore the transaction from the same core that is not a flush
    }
    else if ((Bus_transaction_caller)cache_data->id == transaction->origid && transaction->bus_cmd == flush) {
        if ( *address_offset == (BLOCK_SIZE - 1)) {
            return true; // Transaction completed
        }
        *address_offset += 1; // Increment the address offset for multi-word transactions
        return false; // More data to send
    }
    CacheAddressInfo addr = { .address = transaction->bus_addr };
    index = addr.fields.index;
    tsram_line = &(cache_data->tsram[index]); // Get the TSRAM line 

    tsram_line->tag = addr.fields.tag;
    if(transaction->bus_cmd == flush) {
        cache_data->dram[addr.fields.index * BLOCK_SIZE + addr.fields.offset].data = transaction->bus_data;
    }
    if(*address_offset == (BLOCK_SIZE - 1)) {
        if (transaction->bus_shared) {
        // If the transaction indicates shared ownership, set the MESI state to SHARED.
        tsram_line->mesi = MESI_STATE_SHARED;
        } else {
            // Otherwise, set the MESI state to EXCLUSIVE.
            tsram_line->mesi = MESI_STATE_EXCLUSIVE;
        }
        return true; // Transaction completed
    }
    *address_offset += 1; // Increment the address offset for multi-word transactions
    return false; // More data to send
}


/* Bus - Cache Callbacks ends */
/* States machine functions start */
static Cache_Id_enum MSEI_invalid (Cache_Data* cache_data, bus_transaction* transaction) {
    // Handle the MESI state invalid
    return (Cache_Id_enum)MESI_STATE_INVALID;
}


static Cache_Id_enum MSEI_shared (Cache_Data* cache_data, bus_transaction* transaction) {
    // Handle the MESI state shared
    if (transaction->bus_cmd == busRdX) { 
        // Upgrade to modified state
        return (Cache_Id_enum)MESI_STATE_INVALID;
    }
    return (Cache_Id_enum)MESI_STATE_SHARED;
}


static Cache_Id_enum MSEI_exclusive(Cache_Data* cache_data, bus_transaction* transaction) {
    // Handle the MESI state exclusive
    if (transaction->bus_cmd == busRd) { 
        // Downgrade to shared state
        return (Cache_Id_enum)MESI_STATE_SHARED;
    }
    if (transaction->bus_cmd == busRdX) { 
        // Downgrade to shared state
        return (Cache_Id_enum)MESI_STATE_INVALID;
    }
    return (Cache_Id_enum)MESI_STATE_EXCLUSIVE;
}


static void flush_data(Cache_Data* cache_data, bus_transaction* transaction){
        CacheAddressInfo address =  { .address = transaction->bus_addr };
        transaction->bus_data = cache_data->dram[address.fields.index * BLOCK_SIZE + address.fields.offset].data; // Send the modified data back to the Bus
        transaction->bus_cmd = flush; 
        transaction->origid = (Bus_transaction_caller)cache_data->id;
}


static Cache_Id_enum MSEI_modified(Cache_Data* cache_data, bus_transaction* transaction) {
    // Handle the MESI state modified
    if (transaction->bus_cmd == busRd) { 
        flush_data(cache_data, transaction);
        return (Cache_Id_enum)MESI_STATE_SHARED; // Downgrade to shared state
    }
    else if (transaction->bus_cmd == busRdX) { 
        flush_data(cache_data, transaction);
        return (Cache_Id_enum)MESI_STATE_INVALID; // Downgrade to invalid state
    }
    else if (transaction->bus_cmd == flush) { 
        flush_data(cache_data, transaction);
        return (Cache_Id_enum)MESI_STATE_MODIFIED; // Downgrade to shared state
    }
    return (Cache_Id_enum)MESI_STATE_MODIFIED;
}
/* States machine functions end */


void print_Cache_Data(Cache_Data* cache_data, FILE* file_dram, FILE* file_tsram) {
    // Print the cache data
    for (uint32_t i = 0; i < NUM_BLOCKS; i++) {
        fprintf(file_tsram, "%08X\n", cache_data->tsram[i].mesi << 12 | cache_data->tsram[i].tag);
    }
    for (uint32_t i = 0; i < CACHE_SIZE; i++) {
        fprintf(file_dram, "%08X\n", cache_data->dram[i].data);
    }
}
