
#include <stdio.h>
#include <stdbool.h>

#include "bf.h"

#define ASSERT(expr) if (!(expr)) { printf("%s:%d: Assert failed: "#expr"\n", __FUNCTION__, __LINE__); result = 1; goto cleanup; }

FILE *EmulateStream(const char *content) {
	FILE *file = tmpfile();

	fputs(content, file);
	fpos_t pos = 0;
	fsetpos(file, &pos);

	return file;
} 

#pragma region SumMotion

int TestSumMotion_OnLinear_ThenCountMoves(void) {
	int result = 0;

	FILE *f = EmulateStream(">>>.<++.>");

	BF_Context *ctx = BF_FromFile(f);

	bool unpredictable;
	int32_t motion = BF_SumMotion(ctx->instructions, 0, ctx->length, &unpredictable);

	ASSERT(motion == 3);
	ASSERT(!unpredictable);
cleanup:
	fclose(f);
	BF_FreeContext(ctx);
	return result;
}

int TestSumMotion_OnMotionlessLoop_ThenCountMoves(void) {
	int result = 0;

	FILE *f = EmulateStream(">>>.<[++].>");

	BF_Context *ctx = BF_FromFile(f);

	bool unpredictable;
	int32_t motion = BF_SumMotion(ctx->instructions, 0, ctx->length, &unpredictable);

	ASSERT(motion == 3);
	ASSERT(!unpredictable);
cleanup:
	fclose(f);
	BF_FreeContext(ctx);
	return result;
}

int TestSumMotion_OnMotionInLoop_ThenUnpretictable(void) {
	int result = 0;

	FILE *f = EmulateStream(">>>.<[+>>>+].>");

	BF_Context *ctx = BF_FromFile(f);

	bool unpredictable;
	int32_t motion = BF_SumMotion(ctx->instructions, 0, ctx->length, &unpredictable);

	ASSERT(unpredictable);
cleanup:
	fclose(f);
	BF_FreeContext(ctx);
	return result;
}

int TestSumMotion_OnMotionInNestedLoop_ThenUnpretictable(void) {
	int result = 0;

	FILE *f = EmulateStream(">>>.<[+[>>>]+].>");

	BF_Context *ctx = BF_FromFile(f);

	bool unpredictable;
	int32_t motion = BF_SumMotion(ctx->instructions, 0, ctx->length, &unpredictable);

	ASSERT(unpredictable);
cleanup:
	fclose(f);
	BF_FreeContext(ctx);
	return result;
}

int TestSumMotion_OnMotionlessNestedLoop_ThenCountMoves(void) {
	int result = 0;

	FILE *f = EmulateStream(">>>.<[+[>+.<]+].>");

	BF_Context *ctx = BF_FromFile(f);

	bool unpredictable;
	int32_t motion = BF_SumMotion(ctx->instructions, 0, ctx->length, &unpredictable);

	ASSERT(motion == 3);
	ASSERT(!unpredictable);
cleanup:
	fclose(f);
	BF_FreeContext(ctx);
	return result;
}


int TestSumMotion_OnDualMotionlessNestedLoop_ThenCountMoves(void) {
	int result = 0;

	FILE *f = EmulateStream(">>>.<[->+[>+.<]+[->+<]<].>");

	BF_Context *ctx = BF_FromFile(f);

	bool unpredictable;
	int32_t motion = BF_SumMotion(ctx->instructions, 0, ctx->length, &unpredictable);

	ASSERT(motion == 3);
	ASSERT(!unpredictable);
cleanup:
	fclose(f);
	BF_FreeContext(ctx);
	return result;
}

int TestSumMotion_OnWhileLoop_ThenCountMoves(void) {
	int result = 0;

	BF_Instruction instructions[] = {
		{ .type = BF_INC, .operand1 = 2 },
		{ .type = BF_MVL, .operand1 = 1},
		{ .type = BF_SET, .operand1 = 10},
		{ .type = BF_WHILE, .operand1 = 4 },
		{ .type = BF_INC, .operand1 = 1},
		{ .type = BF_WHILE_END }
	};

	bool unpredictable;
	int32_t motion = BF_SumMotion(instructions, 0, sizeof instructions  / sizeof instructions[0], &unpredictable);

	ASSERT(motion == -1);
	ASSERT(!unpredictable);

cleanup:
	return result;
}

#pragma endregion

#pragma region CellDelta

int TestCellDelta_WhenLinear_ThenComputeDelta(void) {
	int result = 0;

	FILE *f = EmulateStream(">>+<-->+>+<<+>-<>");

	BF_Context *ctx = BF_FromFile(f);

	bool unpredictable;
	int32_t delta = BF_CellDelta(ctx->instructions, 2, 0, ctx->length, &unpredictable);

	ASSERT(!unpredictable);
	ASSERT(delta == 1);
cleanup:
	fclose(f);
	BF_FreeContext(ctx);
	return result;
}

#pragma endregion


int main(void) {
	int result = 0;

	result |= TestSumMotion_OnLinear_ThenCountMoves();
	result |= TestSumMotion_OnMotionlessLoop_ThenCountMoves();
	result |= TestSumMotion_OnMotionInLoop_ThenUnpretictable();
	result |= TestSumMotion_OnMotionInNestedLoop_ThenUnpretictable();
	result |= TestSumMotion_OnMotionlessNestedLoop_ThenCountMoves();
	result |= TestSumMotion_OnDualMotionlessNestedLoop_ThenCountMoves();
	result |= TestSumMotion_OnWhileLoop_ThenCountMoves();

	result |= TestCellDelta_WhenLinear_ThenComputeDelta();

	return result;
}