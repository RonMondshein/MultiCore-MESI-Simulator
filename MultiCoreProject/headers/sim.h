
// Definitions for bit manipulation and opcode values


#ifndef __SIM_H__
#define __SIM_H__

// include the necessary files
#include <stdint.h>
#include <stdbool.h>


#define NUM_OF_CORES 4
#define NUM_OF_REGS 16
#define IMM_REG  1
#define ZERO_REG 0
#define START_MUTABLE_REG 2
#define PC_REG 15


typedef union
{
	struct
	{
		uint16_t imm : 12;	// [0:11]  Immediate value
		uint16_t rt : 4;			// [12:15] src1 value
		uint16_t rs : 4;			// [16:19] src0 value
		uint16_t rd : 4;			// [20:23] src0 value
		uint16_t opcode : 8;		// [24:31] opcode value
	} received_op;

	uint32_t cmd;
} Format_of_instruction;

#endif // __SIM_H__









