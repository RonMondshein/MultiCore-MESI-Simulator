#ifndef FilesManager_H
#define FilesManager_H

#include <stdio.h>
#include "./sim.h" 

extern FILE* MemIn;
extern FILE* MemOut;
extern FILE* BusTrace;

typedef struct
{
	FILE* instructionMemoryFile;  // File for instruction memory input
	FILE* registerOutputFile;     // File for register values output
	FILE* executionTraceFile;     // File for execution trace
	FILE* dataCacheFile;          // File for data cache content (DSRAM)
	FILE* tagCacheFile;           // File for tag cache content (TSRAM)
	FILE* coreStatsFile;     // File for core statistics output
} CoreFileHandles;

extern CoreFileHandles coreFileHandlesArray[NUM_OF_CORES];

/* Global Functions*/
int OpenRequiredFiles(char* argv[], int argc); // Open all required files
void closeFiles(); // Close all files

#endif // FilesManager_H
