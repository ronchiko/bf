#include <stdbool.h>
#include <stdio.h>

#include "bf.h"

#define OPTIMIZE_SET		"[-]+*"
#define OPTIMIZE_LEFT_INC	"<*+*>*"

static struct { BF_Operation a, b; } gInversePairs[] = {
	{ BF_INC, BF_DEC },
	{ BF_MVL, BF_MVR },
	{ BF_ICL, BF_DCL },
	{ BF_ICR, BF_DCR },
};
static int gInversePairsLength = sizeof(gInversePairs) / sizeof(gInversePairs[0]);

static bool IsInversePair(BF_Operation a, BF_Operation b) {
	for(int i = 0; i < gInversePairsLength; ++i) {
		if((a == gInversePairs[i].a && b == gInversePairs[i].b) || 
			(a == gInversePairs[i].b && b == gInversePairs[i].a))
			return true;
	}
	return false;
}

static BF_Operation GetInverseOf(BF_Operation o) {
	for(int i = 0; i < gInversePairsLength; ++i) {
		if (o == gInversePairs[i].a) return gInversePairs[i].b;
		if (o == gInversePairs[i].b) return gInversePairs[i].a;
	}

	return BF_NOP;
}

static bool SetPatternOptimizer(BF_Instruction *inst, size_t begin, BF_Context *ctx) {
	size_t current = begin;
	
	uint8_t setValue = 0;
	// Must start with [-]
	if (current >= ctx->length || inst[current].type != BF_LBL) return false;
	++current;
	if (current >= ctx->length || (inst[current].type != BF_DEC && inst[current].type != BF_INC) || inst[current].operand1 != 1) return false;
	++current;
	if (current >= ctx->length || inst[current].type != BF_RPT) return false;
	++current;
	
	// If the next operation is INC or DEC, we can compress it into the set
	if (inst[current].type == BF_INC || inst[current].type == BF_DEC) {
		setValue = inst[current].operand1;
		if (inst[current].type == BF_DEC) {
			setValue = 0xFF - inst[current].operand1;
		}
	}

	inst[begin].type = BF_SET;
	inst[begin].operand1 = setValue;
	BF_PassiveErase(inst + begin + 1, 2 + (setValue ? 1 : 0));
	return true;
}

static bool ShiftSetPatternOptimizer(BF_Instruction *inst, size_t begin, BF_Context *ctx) {
	size_t current = begin;
	
	// Must start with >[-]+*<
	BF_Operation shift;
	if (inst[current].type != BF_MVL && inst[current].type != BF_MVR) return false;
	shift = inst[current].type;

	++current;
	if (current >= ctx->length || inst[current].type != BF_SET) return false;

	++current;
	if (current >= ctx->length || inst[current].type != GetInverseOf(shift)) return false;
	
	
	inst[begin].type = shift == BF_MVL ? BF_STL : BF_STR;
	inst[begin].operand2 = inst[begin].operand1;
	inst[begin].operand1 = inst[begin + 1].operand1;
	BF_PassiveErase(inst + begin + 1, 2);
	return true;
}

static bool UnusedJumpsOptimizer(BF_Instruction *inst, size_t begin, BF_Context *ctx) {
	size_t current = begin;

	size_t set = 0;
	if(inst[current].type != BF_SET) return false;
	set = inst[current].operand1;
	++current;

	if (current >= ctx->length || (inst[current].type != BF_RPT && inst[current].type != BF_LBL)) return false;
	BF_Operation op = inst[current].type;

	// [-][ OR [-]+*]
	bool willAlwaysSkip = (op == BF_LBL && set) || (op == BF_RPT && !set);
	if (willAlwaysSkip) {
		BF_PassiveErase(inst + begin + 1, 1);
		return true;
	}

	// [-]++[
	bool willAlwaysJump = op == BF_LBL && !set;
	if(willAlwaysJump) {
		BF_PassiveErase(inst + begin + 1, inst[current].operand1 - inst[inst[current].operand1].operand1);
		return true;
	}

	return false;
}

static bool ShiftIncrementPatternOptimizer(BF_Instruction *inst, size_t begin, BF_Context *ctx) {
	// >*(+|-)*<
	size_t current = begin;

	BF_Operation shiftDirection, additionDirection;
	if (inst[current].type != BF_MVL && inst[current].type != BF_MVR) return false; 
	shiftDirection = inst[current].type;
	++current;

	if (current >= ctx->length || ((inst[current].type != BF_INC && inst[current].type != BF_DEC) || inst[current].operand1 == 0)) return false;
	additionDirection = inst[current].type;
	++current;

	if (current >= ctx->length || inst[current].type != GetInverseOf(shiftDirection)) return false;

	if (inst[current].operand1 != inst[begin].operand1 || !inst[current].operand1) return false;

	if (additionDirection == BF_INC) inst[begin].type = shiftDirection == BF_MVL ? BF_ICL : BF_ICR;
	if (additionDirection == BF_DEC) inst[begin].type = shiftDirection == BF_MVL ? BF_DCL : BF_DCR;

	inst[begin].operand2 = inst[begin].operand1;
	inst[begin].operand1 = inst[begin + 1].operand1;
	BF_PassiveErase(inst + begin + 1, 2); 
	return true;
}

static bool MergeConstantSetsOptimizer(BF_Instruction *inst, size_t begin, BF_Context *ctx) {
	size_t current = begin;

	BF_Operation operation = inst[current].type; uint32_t shift = 0;
	if (operation != BF_SET && operation != BF_STL && operation != BF_STR) return false;
	shift = inst[current].operand2;
	
	size_t len = 0;
	for(;current < ctx->length && operation == inst[current].type && inst[current].operand2 == shift; ++current, ++len);

	bool successful = len > 1 && current < ctx->length;
	if (successful) {
		inst[begin].operand1 = inst[current - 1].operand1;

		BF_PassiveErase(inst + begin + 1, len - 1);
	}
	
	return successful;
}

static bool ArithmaticsFoldingOptimizer(BF_Instruction *inst, size_t begin, BF_Context *ctx) {
	size_t current = begin;

	static const BF_Operation SETTERS[] = { BF_SET, BF_STL, BF_STR };
	static const BF_Operation ADDERS[]  = { BF_INC, BF_ICL, BF_ICR };

	static const size_t COUNT = sizeof(SETTERS) / sizeof(SETTERS[0]);

	for(size_t i = 0; i < COUNT; ++i) {
		uint32_t shift = inst[current].operand2;
		BF_Operation op = inst[current].type;
		if (SETTERS[i] != op) continue;
	
		++current;
		if(current >= ctx->length || ADDERS[i] != inst[current].type || shift != inst[current].operand2) break;

		inst[begin].operand1 += inst[current].operand1;
		BF_PassiveErase(inst + begin, 1);
		return true;
	}

	return false;
}

static bool InverseOperationOptimizer(BF_Instruction *inst, size_t begin, BF_Context *ctx) {
	const size_t current = begin;
	const size_t next = begin + 1;

	if (next >= ctx->length) return false;
	if(inst[current].operand2 != inst[next].operand2) return false;
	
	
	if (IsInversePair(inst[current].type, inst[next].type)) {
		
		int64_t inverseValue = (int64_t)inst[current].operand1 - inst[next].operand1;
			
		inst[current].operand1 = inverseValue;
		if (inverseValue < 0) inst[current].operand1 = -inverseValue, inst[current].type = inst[next].type;

		if (!inverseValue) inst[current].type = BF_NOP;

		inst[next].type = BF_NOP;
		inst[next].operand2 = inst[next].operand1 = 0;
		return true;
	}

	if (inst[next].type == inst[current].type && IsAggregatableOpcode(inst[current].type)) {
		inst[current].operand1 += inst[next].operand1;
		
		inst[next].type = BF_NOP;
		inst[next].operand2 = inst[next].operand1 = 0;
		return true;
	}

	return false;
} 

static bool WhileLoopOptimizer(BF_Instruction *inst, size_t begin, BF_Context *ctx) {
	size_t current = begin;

	// [->*<] OR [>*<-] OR [-<*>]
	if (inst[current].type != BF_LBL) return false;
	uint32_t end = inst[begin].operand1;
	
	if (end >= ctx->length || inst[end].type != BF_RPT) return false;	// Optimizied away
	bool unpredictable = false;

	++current;
	bool frontDecrementor = current < ctx->length && inst[current].type == BF_DEC;
	bool backDecrementor = end - 1 >= current && inst[end - 1].type == BF_DEC;
	if(frontDecrementor || backDecrementor) {
		int32_t motion = BF_SumMotion(inst, current + 1, end - current + 2, &unpredictable);

		if (unpredictable || motion != 0) return false;

		int32_t delta = BF_CellDelta(inst, 0, current + 1, end - current + 1, &unpredictable);
		if (unpredictable || delta != 0) return false;

		if (frontDecrementor)
			BF_PassiveErase(inst + begin, 2);
		else
			BF_PassiveErase(inst + end - 1, 2);

		inst[end].type = BF_WHILE_END;
		inst[end].operand1 = begin;

		inst[begin].type = BF_WHILE;
		inst[begin].operand1 = end;

		return true;  
	}

	return false;
}

static bool Optimize(BF_Instruction *inst, size_t j, BF_Context *ctx) {
	static bool (*gOptimizers[])(BF_Instruction *, size_t, BF_Context *) = {
		&InverseOperationOptimizer,
		&SetPatternOptimizer,
		&ShiftIncrementPatternOptimizer,
		&ShiftSetPatternOptimizer,
		&MergeConstantSetsOptimizer,
		&UnusedJumpsOptimizer,
		&ArithmaticsFoldingOptimizer,
		&WhileLoopOptimizer,
		NULL
	};

	for(int i = 0; gOptimizers[i]; ++i) {
		if ((*gOptimizers[i])(inst, j, ctx)) return true;
	}

	return false;
}

void BF_OptimizeLevel1(BF_Context *ctx) {
	// Perform stateless optimizations
	size_t optimizations = 0;
	do {
		optimizations = 0;
		for(int64_t i = ctx->length - 1; i >= 0; --i) {
			optimizations += Optimize(ctx->instructions, i, ctx);
		}

		BF_FlattenProgram(ctx->instructions, &ctx->length);
	} while(optimizations);		// Keep optimizing until there is nothing left to optimize
}