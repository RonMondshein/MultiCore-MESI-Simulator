/*!
******************************************************************************
file BusController.c

Implementation of the BUS functionality.

*****************************************************************************/

// include the necessary header files
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../headers/BusController.h"
#include "../headers/FilesManager.h"
#include "../headers/sim.h" 
#include "../headers/CacheController.h"

/* structs definitions */

// structs
/**********************************************************************************/
// mem_adder - Represents the address of a memory block
typedef struct {
    // used to track the main memory performance
    uint8_t offset : 2;  // 2 bits for index, 4 words per block
    uint32_t block : 18;  // 18 bits left for block
} AddressFields;

typedef union {
    uint32_t address;       // Raw 20-bit address
    AddressFields fields;   // Decomposed fields
} MemoryAddress;
// state_of_transaction - Enumerates the states of a bus transaction
typedef enum
{
	idle,
	wait_cmd,
	operation,
	finally,
} state_of_transaction;


// queue_for_bus - Represents a queue for bus transactions
typedef struct _queue_for_bus
{
	bus_transaction item;
	struct _queue_for_bus* prev;
	struct _queue_for_bus* next;
} queue_for_bus;
/**********************************************************************************/


/* variables definitions */
/**********************************************************************************/
static Bus_core_cache gCoreCache[NUM_OF_CORES];

// callbacks functions
static SharedData_Callback	gSharedData_Callback;
static SnoopingCache_Callback	gSnoopingCache_Callback;
static GetCacheResponse_Callback	gGetCacheResponse_Callback;
static Mem_Callback		gMemCallback;

// global variables
static bool gIsBusTransactionActive;
static state_of_transaction gTransactionStatePerCore[NUM_OF_CORES] = { 0, 0, 0, 0 };
static bus_transaction gOngoingTransaction;
static uint8_t gBusAddrOffset;
static uint32_t iterationCount = 0;

// bus fifo queue variables
static queue_for_bus* head_of_queue;
static queue_for_bus* tail_of_queue;

/**********************************************************************************/

// functions declarations
/**********************************************************************************/
bool is_queue_empty(void);
bool queue_enqueue(bus_transaction item);
bool queue_dequeue(bus_transaction* item);

static void print_to_bustrace(bus_transaction TransactionPacket);
static bool is_any_cache_snoop(bus_transaction* TransactionPacket);
static bool is_shared_line(bus_transaction* TransactionPacket, bool* is_data_modified);

/**********************************************************************************/


// Implemantation of the bus fifo queue for transactions
/**********************************************************************************/
/* check whether the queu is empty */
bool is_queue_empty(void)
{
    bool isempty = head_of_queue == NULL;
	return isempty;
}

/* enqueue a transaction to the queue */
bool queue_enqueue(bus_transaction transaction)
{
	queue_for_bus* item = malloc(sizeof(queue_for_bus));
	if (item == NULL) { // if the allocation failed then return false
		return false;
	}

	item->prev = NULL;
	item->next = NULL;
	item->item = transaction;

	if (is_queue_empty())
	{ // if the queue is empty then the head and tail are the same
		head_of_queue = item;
		tail_of_queue = item;
	}
	else
	{ // if the queue is not empty then the new item is the head
		head_of_queue->prev = item;
		item->next = head_of_queue;
		head_of_queue = item;
	}
	return true;
}

/* dequeue a transaction from the queue */
bool queue_dequeue(bus_transaction* transaction)
{
	if (tail_of_queue == NULL) { // if the queue is empty then return false
		return false;
	}

	// get the item from the tail of the queue and update the tail to the previous item
	queue_for_bus* item = tail_of_queue;
	tail_of_queue = item->prev;

	if (tail_of_queue == NULL) { 
		head_of_queue = NULL;
	}
	else
		tail_of_queue->next = NULL;

	item->prev = NULL;
	*transaction = item->item;

	free(item);
	return true;
}

/**********************************************************************************/


/* Implementation of static functions */ 
/**********************************************************************************/

/* print info of the bus stats: iteration number, originator id, command, address, data, shared */
static void print_to_bustrace(bus_transaction TransactionPacket)
{
	fprintf(BusTrace, "%d %d %d %05X %08X %d\n", iterationCount, TransactionPacket.origid, TransactionPacket.bus_cmd, 
		TransactionPacket.bus_addr, TransactionPacket.bus_data, TransactionPacket.bus_shared);
}

/* check if any cache has a response for the bus transaction */
static bool is_any_cache_snoop(bus_transaction* TransactionPacket)
{
	bool is_there_responding = false;

	for (int i = 0; i < NUM_OF_CORES; i++)
		is_there_responding = is_there_responding | gSnoopingCache_Callback(gCoreCache[i].bus_cache_data, TransactionPacket, gBusAddrOffset);
	
	return is_there_responding;
}


/* determinate if the current transaction involves shared data across cores */
static bool is_shared_line(bus_transaction* TransactionPacket, bool* is_data_modified)
{
	bool is_shared = false;
	bool call_back_result = false;

	for (int i = 0; i < NUM_OF_CORES; i++){
		//((Cache_Data*)(gCoreCache[i].bus_cache_data))->id = gCoreCache[i].core_id;
		call_back_result = gSharedData_Callback(gCoreCache[i].bus_cache_data, TransactionPacket, is_data_modified);
		is_shared |= call_back_result;
	}
	return is_shared;
}
/**********************************************************************************/


/* Implementation of the bus functionality */
/**********************************************************************************/
/* register the cache interface */
void Bus_InitializeCache(Bus_core_cache cache_info)
{
	gCoreCache[cache_info.core_id] = cache_info;
}

/* register the callbacks for the bus */
void ConfigureCacheCallbacks_for_bus(SharedData_Callback first_callback,
								SnoopingCache_Callback second_callback,
								GetCacheResponse_Callback third_callback)
{
	gSharedData_Callback = first_callback;
	gSnoopingCache_Callback = second_callback;
	gGetCacheResponse_Callback = third_callback;
}

/* register the memory callback */
void ConfigureMemoryCallback_for_bus(Mem_Callback callback)
{
	gMemCallback = callback;
}

/* add a new transaction to the bus transaction queue */
void AddTransaction_to_bus(bus_transaction transaction)
{
	// add the transaction to the queue
	queue_enqueue(transaction);

	// if the transaction is invalid then return
	if (transaction.origid == invalid_caller){
		return;
	}

	// set the state of the transaction initiated by the specified core to waiting
	gTransactionStatePerCore[transaction.origid] = wait_cmd;
}

/* check if the bus is currently in transaction */
bool IsBusInTransaction(Bus_transaction_caller initiator)
{

	return gTransactionStatePerCore[initiator] != idle;
}

/* check if the bus is waiting for a transaction */
bool IsBusWaitForTransaction(Bus_transaction_caller initiator)
{

	return gTransactionStatePerCore[initiator] == wait_cmd;
}

/* iterate the bus */
void Run_Bus_Iteration(void)
{
	// Track whether this is the first time a shared line is detected for the current transaction.
	static bool is_first_access_shared = true;
	
	// increment the iteration counter
	iterationCount++;
		
	// if the transaction is finally done then set the state of the core to idle
	if (gTransactionStatePerCore[gOngoingTransaction.origid] == finally)
		gTransactionStatePerCore[gOngoingTransaction.origid] = idle;

	// if the queue is empty and there is no ongoing transaction then return
	if (is_queue_empty() && !gIsBusTransactionActive)
	{
		// Mark the current transaction as invalid, indicating no active one.
		gOngoingTransaction.origid = invalid_caller;
		return;
	}

	// If no transaction is currently in progress, start processing the next one.
	if (!gIsBusTransactionActive)
	{
		// Reset the shared-line detection flag for the new transaction.
		is_first_access_shared = true;

		// Store the previous originator ID.
		int previous_origid = gOngoingTransaction.origid;
		
		// Dequeue the next transaction from the queue.
		if (!queue_dequeue(&gOngoingTransaction) || gOngoingTransaction.origid == invalid_caller)
			return;

		// Set the original sender of the transaction to the current originator.
		gOngoingTransaction.original_caller = gOngoingTransaction.origid;

		// Mark the bus as busy and update the state of the current transaction to "operation".
		gIsBusTransactionActive = true;
		gTransactionStatePerCore[gOngoingTransaction.origid] = operation;

		// Reset the address offset for the transaction (used for block-wise memory access).
		gBusAddrOffset = 0;

		// print to the bus trace
		print_to_bustrace(gOngoingTransaction);
	}

	bus_transaction transaction;
	memcpy(&transaction, &gOngoingTransaction, sizeof(gOngoingTransaction));

	// Update the memory address for the current transaction.
	MemoryAddress address = { .address = gOngoingTransaction.bus_addr };
	address.fields.offset = gBusAddrOffset;
	transaction.bus_addr = address.address;

	// Check if the current transaction involves shared data across cores.
	bool is_data_modified = false;
	transaction.bus_shared = is_shared_line(&gOngoingTransaction, &is_data_modified);


	// If the data is modified and this is the first time a shared line is detected, skip the current iteration.
	if (is_data_modified && is_first_access_shared)
	{
		is_first_access_shared = false;
		return;
	}

	// Perform cache snooping for the current transaction, checking if any cache responds.
	bool cache_response  = is_any_cache_snoop(&transaction);

	// Send the transaction to memory and check if there is a memory response.
	bool memory_response = gMemCallback(&transaction, is_data_modified);
	
	// If memory responds, handle it.
	if (memory_response)
	{
		// print to the bus trace
		print_to_bustrace(transaction);

		
		// Send the response back to the originating cache via the response callback.
		if (gGetCacheResponse_Callback(gCoreCache[gOngoingTransaction.origid].bus_cache_data, &transaction,&gBusAddrOffset))
		{
			// If the response was successful, mark the transaction as "finally" and clear the bus active flag.
			gTransactionStatePerCore[gOngoingTransaction.origid] = finally;
			gIsBusTransactionActive = false;
		}
	}
}

/**********************************************************************************/
