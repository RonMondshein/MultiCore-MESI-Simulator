/*!
******************************************************************************
file PipelineController.c

Implementation of Pipeline functionality.

*****************************************************************************/


// include the necessary header files
#include <string.h>
#include "../headers/PipelineController.h"
#include "../headers/CacheController.h"
#include "../headers/OpcodeHandlers.h"

// definition of the static functions of the pipeline
static void fetch(Pipe_fig* pipeline);
static void decode(Pipe_fig* pipeline);
static void execute(Pipe_fig* pipeline);
static void mem(Pipe_fig* pipeline);
static void writeback(Pipe_fig* pipeline);
static void execute_pipe_stages(Pipe_fig* pipeline);
static void enter_params_to_regs(Pipe_fig* pipeline, Pipe_figstate stage);
static bool checkfor_data_hazards(Pipe_fig* pipeline);
static bool check_hazrads_by_comparing_regs(Pipe_fig* pipeline, Pipe_figstate stage);
static void stats_update(Pipe_fig* pipeline);

// Array of function pointers corresponding to each pipeline stage.
static void (*stage_to_exe[PIPE_SIZE])(Pipe_fig* pipeline) = 
{
    fetch,         // Stage 0: Fetch instruction from memory.
    decode,        // Stage 1: Decode the fetched instruction.
    execute,       // Stage 2: Execute the decoded instruction.
    mem,           // Stage 3: Perform memory access if needed.
    writeback      // Stage 4: Write the result back to registers.
};

/* functions implementations */

/* void Pipe_Init(Pipe_fig *pipeline) : initialization of the pipeline */
void Pipe_Init(Pipe_fig *pipeline) {

    // setting the flags of the pipeline to false, the initial state of the pipeline
    *pipeline = (Pipe_fig){.is_halted = false, .data_stall = false, .mem_stall = false};

    // Clear the stats structure
    memset((uint8_t *)&pipeline->stats, 0, sizeof(pipeline->stats));

    // Clear the params_of_op structure
	memset((uint8_t *) &pipeline->params_of_op, 0, sizeof(pipeline->params_of_op));

    // Clear the stages of the pipeline
    memset((uint8_t*) pipeline->stages_in_pipe, 0, sizeof(pipeline->stages_in_pipe));

    // Set the halt flag to the is_halted flag
	pipeline->params_of_op.halt = &pipeline->is_halted;


    // Initialize the stages of the pipeline
    for (int stage = FETCH; stage < PIPE_SIZE; stage++) {

    // Initialize the state of the current pipeline stage
    pipeline->stages_in_pipe[stage].state = stage;

    // Set the program counter to a default value
    pipeline->stages_in_pipe[stage].pc = UINT16_MAX;
}

    // Set the program counter of the fetch stage to 0
	pipeline->stages_in_pipe[FETCH].pc = 0;
        

}

/* void Pipe_iteration_exe(Pipe_fig* pipeline) : execute an iteration of the pipeline based on it's condition */
void Pipe_iteration_exe(Pipe_fig* pipeline) {

    // Check for data hazards
    pipeline->data_stall = checkfor_data_hazards(pipeline);

    // Execute the pipeline stages
    execute_pipe_stages(pipeline);

    // Update the statistics of the pipeline
    stats_update(pipeline);

}

/* bool Pipe_Flush(Pipe_fig* pipeline) : flushing the pipeline. return true if the pipeline is flushed, false otherwise */

bool Pipe_Flush(Pipe_fig* pipeline){
    
    
    // Check if the pipeline is halted
    if (!pipeline->is_halted) {
        return false;
    }

    // Check if all pipeline stages are flushed, meaning that the program counter is set to UINT16_MAX
    for (int stage = FETCH; stage < PIPE_SIZE; stage++) {
        if (pipeline->stages_in_pipe[stage].pc != UINT16_MAX) {
            return false;
        }
    }

    return true; // All conditions are satisfied, the pipeline is flushed


}

/* void Pipe_ToTrace(Pipe_fig* pipeline, FILE *trace_file) : making the pipeline tracing file */

void Pipe_ToTrace(Pipe_fig* pipeline, FILE *trace_file){

        // print the pc of each stage of the pipeline to the trace file or print "---" to mark a non active stage
        for (int stage = FETCH; stage < PIPE_SIZE; stage++) {
            fprintf(trace_file, 
                    (pipeline->stages_in_pipe[stage].pc == UINT16_MAX) ? "--- " : "%03X ", 
                    pipeline->stages_in_pipe[stage].pc);
    }
}

/*void BubbleStage(Pipe_instruction_stage* dest, const Pipe_instruction_stage* src) : bubbling condition to next stage */
void BubbleStage(Pipe_instruction_stage* dest, const Pipe_instruction_stage* src) {
    dest->pc = src->pc;
    dest->instruction.cmd = src->instruction.cmd;
    dest->operation = *src->operation;
    dest->result_of_execution = src->result_of_execution;
}

/*void Pipe_Bubbles(Pipe_fig* pipeline) : bubble the commands where needed*/
void Pipe_Bubbles(Pipe_fig* pipeline) {

    // Handle pipeline stalls and bubbling from back to front
    for (int stage = PIPE_SIZE - 1; stage > FETCH; stage--) {
        if (pipeline->mem_stall) {
            pipeline->stages_in_pipe[WRITE_BACK].pc = UINT16_MAX;
            break; // Stop processing further stages
        } else if (pipeline->data_stall && stage == EXECUTE) {
            pipeline->stages_in_pipe[EXECUTE].pc = UINT16_MAX;
            break; // Stop processing further stages
        } else if (pipeline->stages_in_pipe[stage - 1].pc == UINT16_MAX) {
            pipeline->stages_in_pipe[stage].pc = UINT16_MAX; // Bubble this stage
        } else { // Bubble the stage
            BubbleStage(&pipeline->stages_in_pipe[stage], &pipeline->stages_in_pipe[stage - 1]);
        }
    }

    // Handle halted pipeline state
    if (pipeline->is_halted) {
        pipeline->stages_in_pipe[FETCH].pc = UINT16_MAX;
        pipeline->stages_in_pipe[DECODE].pc = UINT16_MAX;
    }
}


/* void fetch(Pipe_fig* pipeline) : Fetch stage of the pipeline */

static void fetch(Pipe_fig* pipeline)
{
	// Fetch stage cannot proceed in case of memory stall
    if (pipeline->mem_stall) return;

    // Fetching part
    
    // Fetch the program counter (PC) value from the pipeline's opcode parameters
	pipeline->stages_in_pipe[FETCH].pc = *(pipeline->params_of_op.pc);
    
    // Use the current PC value to fetch the instruction from the instruction memory.
	pipeline->stages_in_pipe[FETCH].instruction.cmd = pipeline->insturcionts_pnt[*(pipeline->params_of_op.pc)];

    // If there is no data stall, increment the program counter (PC) to point to the next instruction.
	if (!pipeline->data_stall) 
	{
		*(pipeline->params_of_op.pc) += 1;
	}
}

/* void decode(Pipe_fig* pipeline) : Decode stage of the pipeline. use functions from OpcodeHandlers */
static void decode(Pipe_fig* pipeline)
{
	
    // Get the opcode from the instruction in the decode stage
    uint16_t opcode = pipeline->stages_in_pipe[DECODE].instruction.received_op.opcode;

    //if the opcode means HALT, set the pipeline to halt
	if (opcode == HALT) 
	{
		pipeline->is_halted = true;
		return;
	}

	// decode the instruction
    pipeline->stages_in_pipe[DECODE].operation = OpcodeFunctionTable[opcode];
    
    // If the opcode is of branch operation, prepare the registers parameters
    if (IsOpcodeBranch(pipeline->stages_in_pipe[DECODE].instruction.received_op.opcode))
	{
		enter_params_to_regs(pipeline, DECODE);
		pipeline->stages_in_pipe[DECODE].operation(&pipeline->params_of_op);
	}
}

/* void execute(Pipe_fig* pipeline) : Execute stage of the pipeline*/
static void execute(Pipe_fig* pipeline)
{
    // Extract the opcode of the instruction currently in the EXECUTE stage.
    uint16_t opcode = pipeline->stages_in_pipe[EXECUTE].instruction.received_op.opcode;

    // If the opcode is not a branch or memory operation, prepare the registers parameters and perform the operation
	if (!IsOpcodeBranch(opcode) && !IsOpcodeMemory(opcode) && opcode != HALT)
	{
		enter_params_to_regs(pipeline, EXECUTE);
		pipeline->stages_in_pipe[EXECUTE].operation(&pipeline->params_of_op);
	}
}


/* void mem(Pipe_fig* pipeline) : Memory stage of the pipeline */
static void mem(Pipe_fig* pipeline){ 

    // Extract the opcode of the instruction currently in the MEM stage.
    uint16_t opcode = pipeline->stages_in_pipe[MEM].instruction.received_op.opcode;

    // Check if the opcode corresponds to a memory-related instruction.
    if (IsOpcodeMemory(opcode))
    {
        // Prepare the necessary parameters (register values) for the memory operation.
        enter_params_to_regs(pipeline, MEM);

        // Get a pointer to the data to be read or written (located in the rd register).
        uint32_t* data = pipeline->params_of_op.rd;

        // Calculate the effective memory address using the values of the rs and rt registers.
        uint32_t adr = pipeline->params_of_op.rs + pipeline->params_of_op.rt;

        // Perform the memory operation (read or write) based on the opcode:
        // - If the opcode is LW (Load Word), read data from the cache.
        // - Otherwise, write data to the cache (SW).
        bool success;
        if (opcode == LW)
        {
            success = Read_Data_from_Cache(&pipeline->data_in_cache, adr, data);
        }
        else
        {
            success = Write_Data_to_Cache(&pipeline->data_in_cache, adr, *data);
        }

        // If the operation was not successful, the pipeline stalls.
        pipeline->mem_stall = !success;
    }
}


/* void writeback(Pipe_fig* pipeline) : Write back stage of the pipeline */
static void writeback(Pipe_fig* pipeline){

    // Extract the instruction currently in the WRITE_BACK stage.
    Format_of_instruction instruction = { 
        .cmd = pipeline->stages_in_pipe[WRITE_BACK].instruction.cmd 
    };

    // Determine the register index to update:
    // - If the instruction is JAL (Jump and Link), the target is the program counter register.
    // - Otherwise, it's the destination register (rd).
    int chosen_reg;
    if (instruction.received_op.opcode == JAL)
    {
        chosen_reg = PC_REG;
    }
    else
    {
        chosen_reg = instruction.received_op.rd;
    }

    // Write the result from the EXECUTE stage into the determined register.
    pipeline->regs_pnt[chosen_reg] = pipeline->stages_in_pipe[WRITE_BACK].result_of_execution;
}



/*void enter_params_to_regs(Pipe_fig* pipeline, Pipe_figstate stage) : transfer parameters to registers for the operations */
static void enter_params_to_regs(Pipe_fig* pipeline, Pipe_figstate stage)
{
    // Extract the instruction from the given pipeline stage
    Format_of_instruction instuction = {.cmd = pipeline->stages_in_pipe[stage].instruction.cmd};

    // Store the immediate value from the instruction 
    pipeline->regs_pnt[IMM_REG] = instuction.received_op.imm;

    // Set the execute result of the current pipeline stage to the value in the register 'rd'
    pipeline->stages_in_pipe[stage].result_of_execution = pipeline->regs_pnt[instuction.received_op.rd];

    // Load the register values
    pipeline->params_of_op.rd = &pipeline->stages_in_pipe[stage].result_of_execution;
    pipeline->params_of_op.rs = pipeline->regs_pnt[instuction.received_op.rs];
    pipeline->params_of_op.rt = pipeline->regs_pnt[instuction.received_op.rt];
}


/* void execute_pipe_stages(Pipe_fig* pipeline) : controls the execution of the pipeline's stages in sequence
 while handling conditions like stalls and halts*/
static void execute_pipe_stages(Pipe_fig* pipeline){
    uint8_t stage;

    // Determine the stage to execute from based on the pipeline's state
    if (pipeline->mem_stall) {
        stage = MEM; // Memory stall so we execute the MEM stage
    } else if (pipeline->data_stall) {
        stage = EXECUTE; // Data stall so we execute the EXECUTE stage
    } else {
        stage = DECODE; // No stalls so we can execute the DECODE stage
    }

    // If the pipeline is not halted, fetch the next instruction
    if(!pipeline->is_halted){
        stage_to_exe[FETCH](pipeline);
    }

    // Execute the valid stages of the pipeline from the determined stage
    for (; stage < PIPE_SIZE; stage++)
	{
		if (!(pipeline->stages_in_pipe[stage].pc == UINT16_MAX))
		{
			stage_to_exe[stage](pipeline);
		}
	}

}


/*bool check_hazrads_by_comparing_regs(Pipe_fig* pipeline, Pipe_figstate stage) : looking for potential hazards by comparing registers
in differents stages of the pipeline*/
static bool check_hazrads_by_comparing_regs(Pipe_fig* pipeline, Pipe_figstate stage)
{
    bool is_hazard = false;
    
    // Check if the PC is invalid (no instruction in the stage)
    if (pipeline->stages_in_pipe[stage].pc == UINT16_MAX) return false;
    

    // Extract the destination register (rd) of the instruction at the given pipeline stage
    uint16_t reg = pipeline->stages_in_pipe[stage].instruction.received_op.rd;


    // Retrieve the instruction from the DECODE stage to check for register dependencies
    Format_of_instruction ins_in_decode = pipeline->stages_in_pipe[DECODE].instruction;

    // Extract the opcode of the instruction in the WRITE_BACK stage
    uint16_t op = pipeline->stages_in_pipe[WRITE_BACK].instruction.received_op.opcode;

    // Immediate register and zero register aren't involved in hazards
    if (reg == IMM_REG || reg == ZERO_REG )
    {
        is_hazard = false;
    }

    // for operations that use rs and rt we need to check if the destination register is one of them
    else if (ins_in_decode.received_op.opcode <= SRL || ins_in_decode.received_op.opcode == LW ||
             (ins_in_decode.received_op.opcode == SW && op == SW))
    {
        is_hazard = (reg == ins_in_decode.received_op.rs 
                    || reg == ins_in_decode.received_op.rt);
    }
    else
    {
        // for operations that use rd as well, we need to check if the destination register is any of the three
        is_hazard = (reg == ins_in_decode.received_op.rd 
                    || reg == ins_in_decode.received_op.rs 
                    || reg == ins_in_decode.received_op.rt );
    }

    return is_hazard;
}



/* bool checkfor_data_hazards(Pipe_fig* pipeline) : look for data hazards  */
static bool checkfor_data_hazards(Pipe_fig* pipeline)
{
    // Check in stages actively deal with operations that modify or depend on register/memory data
    return check_hazrads_by_comparing_regs(pipeline, EXECUTE) 
        || check_hazrads_by_comparing_regs(pipeline, MEM)
        || check_hazrads_by_comparing_regs(pipeline, WRITE_BACK);
    
}


/* void stats_update(Pipe_fig* pipeline) : update the stats struct stalls counters by pipline position*/
static void stats_update(Pipe_fig* pipeline)
{
    pipeline->stats.stalls_in_decode += (pipeline->data_stall && !pipeline->mem_stall) ? 1 : 0;
    pipeline->stats.stalls_in_mem += pipeline->mem_stall ? 1 : 0;
}








