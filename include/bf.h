#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BIT(n)		(1 << (n))


typedef enum {
	BF_NOP = 0,
	
	__BF_AGGREGATABLE_START__,
	BF_MVL,
	BF_MVR,
	BF_INC,
	BF_DEC,
	__BF_AGGREGATABLE_END__,
	
	BF_PRT,
	BF_INP,
	BF_LBL,
	BF_RPT,

	__BF_EXTEND_OPSET__,
	BF_ICL,			// Increment Left,
	BF_DCL,			// Decrement Left
	BF_ICR,			// Increment Right
	BF_DCR,			// Decrement Right
	BF_SET,
	BF_STL,			// Set Left
	BF_STR,			// Set Right

	BF_WHILE,		// While loop
	BF_WHILE_END,	// While loop end
} BF_Operation;

#define BF_OPT_NONE		0
#define BF_OPT_MIN		1
#define BF_OPT_MAX		3

#define BF_FLAG_GENERATE_SUDO		BIT(1)
#define BF_FLAG_DEBUG				BIT(2)	

typedef struct {
	char *source;
	char *output;
	uint8_t optimizationLevel;
	uint16_t flags;
} BF_Argv;

uint8_t IsAggregatableOpcode(char op);

void BF_LoadArguments(int argc, char *argv[], BF_Argv *bfArgv);
void BF_FreeArguments(BF_Argv *argv);

typedef struct {
	BF_Operation type;
	uint32_t operand1;
	uint32_t operand2;
} BF_Instruction;

typedef struct {
	BF_Instruction *instructions;
	size_t length;
} BF_Context;

BF_Context *BF_FromFile(void *handle);
BF_Context *BF_Open(char *source);

char *BF_Export(BF_Context *context);

void BF_FreeContext(BF_Context *context);

void BF_FlattenProgram(BF_Instruction *array, size_t *sl);
void BF_PassiveErase(BF_Instruction *start, size_t len);

#define CTX_MEMORY_SCALABLE		BIT(0)

int32_t BF_SumMotion(BF_Instruction *array, size_t start, size_t len, bool *unpredictable);
int32_t BF_CellDelta(BF_Instruction *origin, size_t cell, size_t start, size_t len, bool *unpredictable);

typedef struct {
	BF_Context *context;

	size_t dp;
	size_t ip;
	uint8_t error;

	struct {
		char *buffer;
		size_t length;
		uint8_t flags;
	} memory;

	struct {
		uint64_t *buffer;
		size_t used;
		size_t allocated;
	} stack;
} BF_SimulationContext;

BF_SimulationContext *BF_CreateSimulation(BF_Context *ctx);
void BF_FreeSimulation(BF_SimulationContext *ctx);

uint64_t BF_Run(BF_SimulationContext *sim);

void BF_OptimizeLevel1(BF_Context *context);
