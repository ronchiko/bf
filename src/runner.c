
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bf.h"


#define DEFAULT_MEMORY_STRIP_LENGTH			30000
#define DEFAULT_STACK_SIZE					64

static inline void RunInstruction(BF_SimulationContext *sim);

static void StackPush(BF_SimulationContext *ctx, uint64_t item) {
	if(ctx->stack.used >= ctx->stack.allocated) {
		ctx->stack.allocated += DEFAULT_STACK_SIZE;
		ctx->stack.buffer = realloc(ctx->stack.buffer, ctx->stack.allocated * sizeof(uint64_t));
	}

	ctx->stack.buffer[ctx->stack.used++] = item;
}

inline static uint64_t StackPop(BF_SimulationContext *ctx) {
	if(ctx->stack.used <= 0) {
		printf("Empty stack\n");
		ctx->error = 1;
		return -1;
	}

	return ctx->stack.buffer[--ctx->stack.used];
}

inline static char *MemReadOff(BF_SimulationContext *sim, int32_t shift) {
	static char S_Write = 0;

	size_t odp = sim->dp + shift;
	if(odp < 0 || odp >= sim->memory.length) {
		if (sim->memory.flags & CTX_MEMORY_SCALABLE) {
			size_t newLength = odp + DEFAULT_MEMORY_STRIP_LENGTH;
			sim->memory.buffer = realloc(sim->memory.buffer, newLength);
			memset(sim->memory.buffer + sim->memory.length, 0, newLength - sim->memory.length);
			
			sim->memory.length = newLength;
			return &sim->memory.buffer[sim->ip];
		}

		printf("\nOOM (ip: %zu, dp: %zu(READ: %zu, SHIFT: %d) , size: %zu)\n", sim->ip, sim->dp, odp, shift, sim->memory.length);
		sim->error = 1;
		return &S_Write;	// Cant read
	}

	return &sim->memory.buffer[odp];
}

inline static char *MemRead(BF_SimulationContext *sim) {
	return MemReadOff(sim, 0);
}

inline static void MemIncrement(BF_SimulationContext *sim, int operand1) {
	*MemRead(sim) += operand1;
}

inline static void IOWrite(BF_SimulationContext *sim) {
	putchar(*MemRead(sim)); fflush(stdout);
}

inline static void IORead(BF_SimulationContext *sim) {
	int chr = getchar();
	if (chr != EOF) *MemRead(sim) = chr;
}

inline static void WhileLoop(BF_SimulationContext *sim, uint32_t jump) {
	size_t dp = sim->dp, ip = sim->ip;
	for(;*MemRead(sim); MemIncrement(sim, -1)) {
		for (sim->ip = ip; sim->ip <= jump; ++sim->ip) {
			RunInstruction(sim);
		}

		sim->dp = dp;
	}
	sim->ip = jump;
}

BF_SimulationContext *BF_CreateSimulation(BF_Context *ctx) {
	BF_SimulationContext *sim = malloc(sizeof(BF_SimulationContext));

	sim->dp = 0;
	sim->ip = 0;

	sim->memory.buffer = malloc(DEFAULT_MEMORY_STRIP_LENGTH);
	sim->memory.length = DEFAULT_MEMORY_STRIP_LENGTH;
	sim->memory.flags = 0;

	memset(sim->memory.buffer, 0, sizeof(char) * DEFAULT_MEMORY_STRIP_LENGTH);	

	sim->context = ctx;

	sim->stack.buffer = malloc(DEFAULT_STACK_SIZE * sizeof(uint64_t));
	sim->stack.used = 0;
	sim->stack.allocated = DEFAULT_STACK_SIZE; 

	return sim;
}

void BF_FreeSimulation(BF_SimulationContext *sim) {
	free(sim->memory.buffer);
	free(sim->stack.buffer);

	free(sim);
}

static inline void RunInstruction(BF_SimulationContext *sim) {
	const BF_Instruction *instruction = &sim->context->instructions[sim->ip]; 
	switch(instruction->type) {
	case BF_MVL: sim->dp -= instruction->operand1; break;
	case BF_MVR: sim->dp += instruction->operand1; break;
	case BF_INC: MemIncrement(sim, instruction->operand1); break;
	case BF_DEC: MemIncrement(sim, -instruction->operand1); break;
	case BF_PRT: IOWrite(sim); break;
	case BF_INP: IORead(sim); break;
	case BF_LBL: if(!*MemRead(sim)) sim->ip = instruction->operand1; break;
	case BF_RPT: if(*MemRead(sim)) sim->ip = instruction->operand1; break;
	case BF_SET: *MemRead(sim) = instruction->operand1; break;
	
	case BF_DCL: *MemReadOff(sim, -instruction->operand2) -= instruction->operand1; break;
	case BF_DCR: *MemReadOff(sim, instruction->operand2) -= instruction->operand1; break;
	case BF_ICL: *MemReadOff(sim, -instruction->operand2) += instruction->operand1; break;
	case BF_ICR: *MemReadOff(sim, instruction->operand2) += instruction->operand1; break;

	case BF_STR: *MemReadOff(sim, instruction->operand2) = instruction->operand1; break;
	case BF_STL: *MemReadOff(sim, -instruction->operand2) = instruction->operand1; break;

	case BF_WHILE: if(!*MemRead(sim)) sim->ip = instruction->operand1; else MemIncrement(sim, -1); break;
	case BF_WHILE_END: if(*MemRead(sim)) sim->ip = instruction->operand1 - 1; break;

	case BF_NOP: default: break; // Not a instruction
	}
}

uint64_t BF_Run(BF_SimulationContext *sim) {
	char *current = NULL;
	uint64_t steps = 0;
	for(sim->ip = 0, sim->error = 0; !sim->error && sim->ip < sim->context->length; ++sim->ip, ++steps) {
		RunInstruction(sim);
	}

	return steps;
}