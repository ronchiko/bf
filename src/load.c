
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bf.h"

#define INSERT(op)	InstructionSetInsert(ctx, &alloc, (op), &s)

typedef struct {
	uint32_t *buffer;
	size_t used, allocated;
} Stack;

static void StackPush(Stack *s, uint32_t addr) {
	if(s->used >= s->allocated) s->buffer = realloc(s->buffer, (s->allocated += 64) * sizeof(uint32_t));
	s->buffer[s->used++] = addr;
}

static uint32_t StackPop(Stack *s) {
	return s->buffer[--s->used];
}

uint8_t IsAggregatableOpcode(char op) {
	return __BF_AGGREGATABLE_START__ <= op && op < __BF_AGGREGATABLE_END__;
}

void InstructionSetInsert(BF_Context *ctx, size_t *alloc, char operation, Stack *stack) {
	// Can we aggregate?
	if(ctx->length > 0 && IsAggregatableOpcode(operation) && operation == ctx->instructions[ctx->length - 1].type) {
		ctx->instructions[ctx->length - 1].operand1++;
		return;
	} 

	if (ctx->length >= *alloc) {
		ctx->instructions = realloc(ctx->instructions, (*alloc += 50) * sizeof(BF_Instruction));
	}

	if (IsAggregatableOpcode(operation) || operation == BF_INP || operation == BF_PRT) {
		ctx->instructions[ctx->length++] = (BF_Instruction){ .type = operation, .operand1 = 1 };
		return;
	}

	if(operation == BF_LBL) {
		ctx->instructions[ctx->length++] = (BF_Instruction){
			.type = BF_LBL,
			.operand1 = -1
		};
		StackPush(stack, ctx->length - 1);
		return;
	}
	
	// operation == BF_RPT
	if(stack->used <= 0) {
		printf("Unmached ] at byte %zu\n", ctx->length);
		return;
	}

	uint32_t addr = StackPop(stack);
	ctx->instructions[addr].operand1 = ctx->length;
	ctx->instructions[ctx->length++] = (BF_Instruction){
		.type = BF_RPT,
		.operand1 = addr
	};
}

BF_Context *BF_FromFile(FILE *handle) {
	BF_Context *ctx = malloc(sizeof(BF_Context));
	ctx->instructions = NULL;
	ctx->length = 0;
	
	Stack s = { .allocated = 0, .used = 0, .buffer = NULL };

	size_t alloc = 0;
	for(int c;(c = fgetc(handle)) > 0;) {
		switch(c) {
		case '<': INSERT(BF_MVL); break;
		case '>': INSERT(BF_MVR); break;
		case '+': INSERT(BF_INC); break;
		case '-': INSERT(BF_DEC); break;
		case '.': INSERT(BF_PRT); break;
		case ',': INSERT(BF_INP); break;
		case '[': INSERT(BF_LBL); break;
		case ']': INSERT(BF_RPT); break;
		default: continue;	// Invalid operation
		}
	}

	return ctx;
}

BF_Context *BF_Open(char *source) {
	FILE *handle = fopen(source, "r");
	if (!handle) {
		return NULL;
	}

	return BF_FromFile(handle);
}

void BF_FreeContext(BF_Context *ctx) {
	free(ctx->instructions);
	free(ctx);
}
