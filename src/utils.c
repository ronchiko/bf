
#include <stdbool.h>

#include "bf.h"


void BF_FlattenProgram(BF_Instruction *a, size_t *len) {
	static const BF_Operation CLOSERS[]  = { BF_RPT, BF_WHILE_END };
	static const BF_Operation STARTERS[] = { BF_LBL, BF_WHILE };

	static const size_t COUNT = sizeof(CLOSERS) / sizeof(CLOSERS[0]); 
	
	for(size_t i = 0; i < *len; ++i) {
		size_t start = i;
		for(;i < *len && a[i].type == BF_NOP; ++i);

		if(i - start > 0) {
			size_t trimSize = i - start;
			for(int j = 0; j < *len - i; ++j) {
				a[start + j].operand1 = a[i + j].operand1;
				a[start + j].operand2 = a[i + j].operand2;
				a[start + j].type = a[i + j].type;

				uint32_t target = a[start + j].operand1;;
				switch(a[start + j].type) {
				case BF_RPT:
					if (target >= i) target -= trimSize;
					
					if (a[target].type == BF_LBL)
						a[target].operand1 = start + j;
					else printf("Unmatched jnz\n");
					
					a[start + j].operand1 = target;
					break;
				case BF_WHILE_END:
					if (target >= i) target -= trimSize;

					if(a[target].type != BF_WHILE) {
						printf("Unmatched while closer\n");
						continue;
					} // We have an error!

					a[start + j].operand1 = target;
					a[target].operand1 = start + j;
					break;

				default: continue;
				}
			}
			
			*len -= trimSize;
			
			i = start; // To rescan area
		}
	}
}

void BF_PassiveErase(BF_Instruction *start, size_t len) {
	for(size_t i = 0; i < len; ++i, ++start) {
		start->type = BF_NOP;
		start->operand1 = 0;
		start->operand2 = 0;
	}
}

static int32_t JumpingMotion(BF_Instruction *origin, size_t *i, size_t len, bool *unpredictable) {
	uint32_t end = origin[*i].operand1;
	uint32_t range = end - *i;

	size_t start = *i + 1;
	int32_t motion = LinearMotion(origin, &start, start + range, unpredictable);
	if (motion != 0) {
		*unpredictable = true;
		return 0;
	}

	*i = end;
	return motion;
}

static int32_t LinearMotion(BF_Instruction *origin, size_t *i, size_t len, bool *unpredictable) {
	int32_t motion = 0;
	for(;!*unpredictable && *i < len; ++*i) {
		switch (origin[*i].type) {
			case BF_MVL: motion -= origin[*i].operand1; break;
			case BF_MVR: motion += origin[*i].operand1; break;

			case BF_LBL: 
				motion += JumpingMotion(origin, i, len, unpredictable); 
				break;
			case BF_WHILE:	// While has shift of 0, we can skip everything inside the while loop
				*i = origin[*i].operand1;
				break;

			default: continue;
		}
	}

	return motion;
}

int32_t BF_SumMotion(BF_Instruction *origin, size_t start, size_t len, bool *unpredictable) {
	size_t i = start;
	*unpredictable = false;
	return LinearMotion(origin, &i, len, unpredictable);
}

static int32_t LinearDelta(BF_Instruction *origin, int64_t index, size_t *i, size_t len, bool *unpredictable) {
	int32_t cell = 0; 
	int32_t motion = 0, dp = 0;

	for (;!*unpredictable && *i < len; ++*i) {
		switch (origin[*i].type) {
			case BF_MVL: dp -= origin[*i].operand1; break;
			case BF_MVR: dp += origin[*i].operand1; break;

			case BF_INC: if (dp == index) cell += origin[*i].operand1; break;
			case BF_DEC: if (dp == index) cell -= origin[*i].operand1; break;
			case BF_ICL: if (dp - origin[*i].operand2 == index) cell += origin[*i].operand1; break;
			case BF_ICR: if (dp + origin[*i].operand2 == index) cell += origin[*i].operand1; break;
			case BF_DCL: if (dp - origin[*i].operand2 == index) cell -= origin[*i].operand1; break;
			case BF_DCR: if (dp + origin[*i].operand2 == index) cell -= origin[*i].operand1; break;

			case BF_SET: if (dp == index) cell = origin[*i].operand1;
			case BF_STL: if (dp - origin[*i].operand2 == index) cell = origin[*i].operand1;
			case BF_STR: if (dp + origin[*i].operand2 == index) cell = origin[*i].operand1;

			case BF_INP: if(dp == index) *unpredictable = true; break;		// If we an input segment on the targeted cell, we cant compute its delta

			case BF_LBL: 
				motion = JumpingMotion(origin, i, len, unpredictable); 
				if (motion != 0 || *unpredictable) {
					*unpredictable = true;
					break;
				}
				cell += LinearDelta(origin, cell - dp ,i, origin[*i].operand1, unpredictable);
				break;
			case BF_WHILE:	// While acts like SET 0, we can skip everything inside the while loop
				*i = origin[*i].operand1;
				if (dp == index) cell = 0;
				break;

			default: continue;
		}
	}

	return cell;
}

int32_t BF_CellDelta(BF_Instruction *origin, size_t cell, size_t start, size_t len, bool *unpredictable) {
	size_t i = start;

	*unpredictable = false;
	return LinearDelta(origin, cell, &i, len, unpredictable);
}