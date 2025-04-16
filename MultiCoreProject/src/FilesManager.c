/*!
******************************************************************************
FilesManager.c

This file handles all file-related operations for the simulation. 
It manages the opening, closing, and validation of input and output files used by the 
simulation, such as memory initialization files, trace logs, and statistical 
outputs.

The functions in this file ensure that all required files are properly 
opened before the simulation starts and are safely closed after execution. 
It helps to centralize and streamline file management across the project.
******************************************************************************
*/

/* Includes */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#endif

#include "../headers/FilesManager.h"
FILE* MemIn;
FILE* MemOut;
FILE* BusTrace;

/* Static Functions */
static FILE* openFile(bool useRelativePath, const char* relativePath, const char* argvPath, const char* mode);
static bool fileCoreFailedToOpen(CoreFileHandles* coreFileHandles, int core);
static bool fileFailedToOpen();
CoreFileHandles coreFileHandlesArray[NUM_OF_CORES];


/* Functions */
static FILE* openFile(bool useRelativePath, const char* relativePath, const char* argvPath, const char* mode) {
    // Use relative path if no argument is provided
    return fopen(useRelativePath ? relativePath : argvPath, mode);
}


static bool fileCoreFailedToOpen(CoreFileHandles* coreFileHandles, int core) {
    // Check if all core-specific files are open, if not, print which files failed to open
    bool failed = false;
    if (coreFileHandles->instructionMemoryFile == NULL) {
        printf("Error: Failed to open instruction memory file in core %d.\n", core);
        failed = true;
    }
    if (coreFileHandles->registerOutputFile == NULL) {
        printf("Error: Failed to open register output file in core %d.\n", core);
        failed = true;
    }
    if (coreFileHandles->executionTraceFile == NULL) {
        printf("Error: Failed to open execution trace file in core %d.\n", core);
        failed = true;
    }
    if (coreFileHandles->dataCacheFile == NULL) {
        printf("Error: Failed to open data cache file in core %d.\n", core);
        failed = true;
    }
    if (coreFileHandles->tagCacheFile == NULL) {
        printf("Error: Failed to open tag cache file in core %d.\n", core);
        failed = true;
    }
    if (coreFileHandles->coreStatsFile == NULL) {
        printf("Error: Failed to open core stats file in core %d.\n", core);
        failed = true;
    }
    printf("Core %d files opened\n", core);
    return failed;
}


static bool fileFailedToOpen() {
    // Check if global files failed to open
    bool failed = false;

    if (MemIn == NULL) {
        printf("Error: Failed to open MemIn file.\n");
        failed = true;
    }
    if (MemOut == NULL) {
        printf("Error: Failed to open MemOut file.\n");
        failed = true;
    }
    if (BusTrace == NULL) {
        printf("Error: Failed to open BusTrace file.\n");
        failed = true;
    }
    printf("General files opened\n");
    // Check if any core-specific files failed to open
    for (int core = 0; core < NUM_OF_CORES; core++) {
        if (fileCoreFailedToOpen(&coreFileHandlesArray[core], core)) {
            failed = true;
        }
    }
    return failed;
}


int OpenRequiredFiles(char* argv[], int argc) {
    // Clear file arrays
    memset(coreFileHandlesArray, 0, NUM_OF_CORES * sizeof(CoreFileHandles));

    // Check if arguments are provided; default to relative paths if not
    bool relative_path_input;
    if (argc == 1) {
        relative_path_input = true;
    } else {
        relative_path_input = false;
    }

    // Open global files
    MemIn = openFile(relative_path_input, "memin.txt", argv[5], "r");
    MemOut = openFile(relative_path_input, "memout.txt", argv[6], "w");
    BusTrace = openFile(relative_path_input, "bustrace.txt", argv[15], "w");

    // Open core files
    const char* coreDefaults[NUM_OF_CORES][6] = {
        {"imem0.txt", "regout0.txt", "core0trace.txt", "dsram0.txt", "tsram0.txt", "stats0.txt"},
        {"imem1.txt", "regout1.txt", "core1trace.txt", "dsram1.txt", "tsram1.txt", "stats1.txt"},
        {"imem2.txt", "regout2.txt", "core2trace.txt", "dsram2.txt", "tsram2.txt", "stats2.txt"},
        {"imem3.txt", "regout3.txt", "core3trace.txt", "dsram3.txt", "tsram3.txt", "stats3.txt"}
    };

    for (int core = 0; core < NUM_OF_CORES; core++) {
        coreFileHandlesArray[core].instructionMemoryFile = openFile(relative_path_input, coreDefaults[core][0], argv[1 + core], "r");
        coreFileHandlesArray[core].registerOutputFile = openFile(relative_path_input, coreDefaults[core][1], argv[7 + core], "w");
        coreFileHandlesArray[core].executionTraceFile = openFile(relative_path_input, coreDefaults[core][2], argv[11 + core], "w");
        coreFileHandlesArray[core].dataCacheFile = openFile(relative_path_input, coreDefaults[core][3], argv[16 + core], "w");
        coreFileHandlesArray[core].tagCacheFile = openFile(relative_path_input, coreDefaults[core][4], argv[20 + core], "w");
        coreFileHandlesArray[core].coreStatsFile = openFile(relative_path_input, coreDefaults[core][5], argv[24 + core], "w");
    }

    // Check if any files failed to open
    if (fileFailedToOpen()) {
        printf("Error: One or more files failed to open.\n");
        return 1; // Failure
    }
    return 0; // Success
}


void closeFiles(){
    // Close global files
    fclose(MemIn);
    fclose(MemOut);
    fclose(BusTrace);

    // Close core files
    for (int core = 0; core < NUM_OF_CORES; core++) {
        fclose(coreFileHandlesArray[core].instructionMemoryFile);
        fclose(coreFileHandlesArray[core].registerOutputFile);
        fclose(coreFileHandlesArray[core].executionTraceFile);
        fclose(coreFileHandlesArray[core].dataCacheFile);
        fclose(coreFileHandlesArray[core].tagCacheFile);
        fclose(coreFileHandlesArray[core].coreStatsFile);
    }
}
