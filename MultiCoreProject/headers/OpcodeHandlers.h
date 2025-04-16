#ifndef OPCODESHANDLERS_H
#define OPCODESHANDLERS_H

/* Includes */
#include <stdint.h>
#include <stdbool.h>

/* Defines */
#define NUMBER_OPCODES 21 

/* Types & Constants */
typedef enum
{
    ADD = 0,            /**< Addition operation */
    SUB,                /**< Subtraction operation */
    AND,                /**< Logical AND operation */
    OR,                 /**< Logical OR operation */
    XOR,                /**< Logical XOR operation */
    MUL,                /**< Multiplication operation */
    SLL,                /**< Logical shift left operation */
    SRA,                /**< Arithmetic shift right operation */
    SRL,                /**< Logical shift right operation */
    BEQ,                /**< Branch if equal operation */
    BNE,                /**< Branch if not equal operation */
    BLT,                /**< Branch if less than operation */
    BGT,                /**< Branch if greater than operation */
    BLE,                /**< Branch if less than or equal operation */
    BGE,                /**< Branch if greater than or equal operation */
    JAL,                /**< Jump and link operation */
    LW,                 /**< Load word operation */
    SW,                 /**< Store word operation */
    HALT = 20           /**< Halt operation */
} OpcodeFunctions;

typedef struct
{
    uint32_t *rd;           /**< Pointer to the destination register */
    uint32_t rs;            /**< Source register value */
    uint32_t rt;            /**< Target register value */
    uint32_t *memory_p;     /**< Pointer to memory (if needed) */
    uint16_t *pc;           /**< Program counter (10 bits) */
    bool *halt;             /**< Flag to indicate whether to halt execution */
} OpcodeParams;


/* Function Prototypes */
void add(OpcodeParams* params);
void sub(OpcodeParams* params);
void and(OpcodeParams* params);
void or(OpcodeParams* params);
void xor(OpcodeParams* params);
void mul(OpcodeParams* params);
void logicShiftLeft(OpcodeParams* params);
void arithmeticShiftRight(OpcodeParams* params);
void logicShiftRight(OpcodeParams* params);
void branchEqual(OpcodeParams* params);
void branchNotEqual(OpcodeParams* params);
void branchLessThan(OpcodeParams* params);
void branchGreaterThan(OpcodeParams* params);
void branchLessEqual(OpcodeParams* params);
void branchGreaterEqual(OpcodeParams* params);
void jump(OpcodeParams* params);
bool IsOpcodeBranch(uint16_t opcode);
bool IsOpcodeMemory(uint16_t opcode);

/* Map opcode to function */
static void (*OpcodeFunctionTable[NUMBER_OPCODES])(OpcodeParams* params) = {
     // Arithmetic and logic operations
    add, sub, and, or, xor, mul, 
    
     // Shift operations
    logicShiftLeft, arithmeticShiftRight,logicShiftRight, 

     // Branch operations
    branchEqual, branchNotEqual, branchLessThan, branchGreaterThan,
	branchLessEqual, branchGreaterEqual, 
    
     // Jump operation
    jump
};

#endif // OPCODESHANSLERS_H
