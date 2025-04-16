/* Includes */
#include "../headers/OpcodeHandlers.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Opcode Functions */

/* add: Adds rs and rt, stores result in rd. 
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void add(OpcodeParams *params) {
    *params->rd = params->rs + params->rt;
}

/* sub: Subtracts rt from rs, stores result in rd.
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void sub(OpcodeParams *params) {
    *params->rd = params->rs - params->rt;
}

/* and: Performs bitwise AND on rs and rt, stores result in rd.
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void and(OpcodeParams *params) {
    *params->rd = params->rs & params->rt;
}

/* or: Performs bitwise OR on rs and rt, stores result in rd.
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void or(OpcodeParams *params) {
    *params->rd = params->rs | params->rt;
}

/* xor: Performs bitwise XOR on rs and rt, stores result in rd.
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void xor(OpcodeParams *params) {
    *params->rd = params->rs ^ params->rt;
}

/* mul: Multiplies rs and rt, stores result in rd.
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void mul(OpcodeParams *params) {
    *params->rd = params->rs * params->rt;
}

/* logicShiftLeft: Shifts rs left by rt bits, stores result in rd.
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void logicShiftLeft(OpcodeParams *params) {
    *params->rd = params->rs << params->rt;
}

/* logicShiftRight: Shifts rs right by rt bits, stores result in rd.
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void logicShiftRight(OpcodeParams *params) {
    *params->rd = params->rs >> params->rt;
}

/* arithmeticShiftRight: Performs arithmetic right shift on rs by rt bits, stores result in rd.
 * Inputs: rs, rt, rd 
 * Return: None 
 */
void arithmeticShiftRight(OpcodeParams *params) {
    *params->rd = (int32_t)params->rs >> params->rt;
}

/* branchEqual: If rs == rt, sets pc to low 10 bits of rd.
 * Inputs: rs, rt, rd, pc 
 * Return: None 
 */
void branchEqual(OpcodeParams *params) {
    if (params->rs == params->rt) {
        *params->pc = (uint16_t)(*params->rd & 0x1FF);
    }
}

/* branchNotEqual: If rs != rt, sets pc to low 10 bits of rd.
 * Inputs: rs, rt, rd, pc 
 * Return: None 
 */
void branchNotEqual(OpcodeParams *params) {
    if (params->rs != params->rt) {
        *params->pc = (uint16_t)(*params->rd & 0x1FF);
    }
}

/* branchLessThan: If rs < rt, sets pc to low 10 bits of rd.
 * Inputs: rs, rt, rd, pc 
 * Return: None 
 */
void branchLessThan(OpcodeParams *params) {
    if (params->rs < params->rt) {
        *params->pc = (uint16_t)(*params->rd & 0x1FF);
    }
}

/* branchGreaterThan: If rs > rt, sets pc to low 10 bits of rd.
 * Inputs: rs, rt, rd, pc 
 * Return: None 
 */
void branchGreaterThan(OpcodeParams *params) {
    if (params->rs > params->rt) {
        *params->pc = (uint16_t)(*params->rd & 0x1FF);
    }
}

/* branchLessEqual: If rs <= rt, sets pc to low 10 bits of rd.
 * Inputs: rs, rt, rd, pc 
 * Return: None 
 */
void branchLessEqual(OpcodeParams *params) {
    if (params->rs <= params->rt) {
        *params->pc = (uint16_t)(*params->rd & 0x1FF);
    }
}

/* branchGreaterEqual: If rs >= rt, sets pc to low 10 bits of rd.
 * Inputs: rs, rt, rd, pc 
 * Return: None 
 */
void branchGreaterEqual(OpcodeParams *params) {
    if (params->rs >= params->rt) {
        *params->pc = (uint16_t)(*params->rd & 0x1FF);
    }
}

/* jump: Sets pc to low 10 bits of rd.
 * Inputs: rd, pc 
 * Return: None 
 */
void jump(OpcodeParams *params) {
    *params->pc = (uint16_t)(*params->rd & 0x1FF);
}

/* Flag Functions */

/* IsOpcodeBranch: Checks if the opcode is a branch instruction.
 * Inputs: opcode
 * Return: true if opcode is a branch, false otherwise 
 */
bool IsOpcodeBranch(uint16_t opcode) {
    return opcode >= BEQ && opcode <= JAL;
}

/* IsOpcodeMemory: Checks if the opcode is a memory instruction.
 * Inputs: opcode
 * Return: true if opcode is memory, false otherwise 
 */
bool IsOpcodeMemory(uint16_t opcode) {
    return opcode == LW || opcode == SW;
}
