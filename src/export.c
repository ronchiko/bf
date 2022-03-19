
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bf.h"

static struct {
	const char *name;
} gOpcodes[] = {
	[BF_NOP] = { "NOP" },
	[BF_MVL] = { "MOVE %u LEFT" },
	[BF_MVR] = { "MOVE %u RIGHT" },
	[BF_INC] = { "ADD %u" },
	[BF_DEC] = { "SUB %u" },
	[BF_PRT] = { "PRINT" },
	[BF_INP] = { "INPUT" },
	[BF_LBL] = { "JZ %u" },
	[BF_RPT] = { "JNZ %u" },
	[BF_SET] = { "SET %u" },

	[BF_ICL] = { "ADD %u TO %u LEFT" },
	[BF_ICR] = { "ADD %u TO %u RIGHT" },
	[BF_DCL] = { "SUB %u TO %u LEFT" },
	[BF_DCR] = { "SUB %u TO %u RIGHT" },

	[BF_STL] = { "SET %u TO %u LEFT"},
	[BF_STR] = { "SET %u TO %u RIGHT"},

	[BF_WHILE] = { "WHILE %u" },
	[BF_WHILE_END] = { "END %u" }
};

char *BF_Export(BF_Context *context) {
	size_t bytes = 512, used = 0;
	char *string = malloc(bytes);
	string[0] = 0;

	char buffer[513];
	for(int i = 0; i < context->length; ++i) {
		const BF_Instruction *instruction = &context->instructions[i];

		int length = 0;
		length = snprintf(buffer, 512, gOpcodes[instruction->type].name, instruction->operand1, instruction->operand2);

		strcat(buffer, "\n");
		if (used + ++length >= bytes) {
			string = realloc(string, bytes += 512);
		}

		used += length;
		strcat(string, buffer);
	}

	return string;
}