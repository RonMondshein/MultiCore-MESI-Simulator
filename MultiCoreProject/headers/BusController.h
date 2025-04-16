
#ifndef BUSCONTROLLER_H
#define BUSCONTROLLER_H


#include <stdint.h>
#include <stdbool.h>

// relevant structs and enums
/*************************************************************************************/
// Bus originator - Enumerates the entities that can initiate a bus transaction
typedef enum
{
	core0,
	core1,
	core2,
	core3,
	main_memory,
	invalid_caller = 0xFFFF
} Bus_transaction_caller;

// cmd_on_the_bus - Enumerates the commands that can be executed on the bus
typedef enum
{
	no_cmd,
	busRd,
	busRdX,
	flush
} cmd_on_the_bus;


// bus_transaction - Represents a transaction on the bus
typedef struct
{
	Bus_transaction_caller original_caller;
	Bus_transaction_caller origid;
	cmd_on_the_bus bus_cmd;
	uint32_t bus_addr;
	uint32_t bus_data;
	bool bus_shared;
} bus_transaction;

// Bus_core_cache - Represents the cache interface of a core
typedef struct
{
	int core_id;
	void* bus_cache_data;
} Bus_core_cache;

/*************************************************************************************/


// callback functions
typedef bool (*SharedData_Callback)(void* bus_cache_data, bus_transaction* packet, bool* is_modified);
typedef bool (*SnoopingCache_Callback)(void* bus_cache_data, bus_transaction* packet, uint8_t address_offset);
typedef bool (*GetCacheResponse_Callback)(void* bus_cache_data, bus_transaction* packet, uint8_t* address_offset);
typedef bool (*Mem_Callback)(bus_transaction* packet, bool direct_transaction);


// bus implementation functions
void Bus_InitializeCache(Bus_core_cache cache_interface);
void ConfigureCacheCallbacks_for_bus(SharedData_Callback signal_callback, 
								SnoopingCache_Callback snooping_callback, 
								GetCacheResponse_Callback response_callback);
void ConfigureMemoryCallback_for_bus(Mem_Callback callback);


// Create a new transaction on the bus
void AddTransaction_to_bus(bus_transaction packet);

// Check if the bus is in transaction
bool IsBusInTransaction(Bus_transaction_caller originator);

// Check if the bus is waiting for transaction
bool IsBusWaitForTransaction(Bus_transaction_caller originator);

// Iterate the bus
void Run_Bus_Iteration(void);

#endif // BUSCONTROLLER_H

