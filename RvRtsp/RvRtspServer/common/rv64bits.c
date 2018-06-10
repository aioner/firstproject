/*****************************************************************************
Filename    : rv64bits.c
Description : 64 bit operations library
************************************************************************
        Copyright (c) 2001 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/
#include "rvtypes.h" /* Will include rv64bits.h */


#if (RV_64BITS_TYPE == RV_64BITS_MANUAL)

/* Just in case we want to change this or go higher than 64 bits. */
/* Current assumptions: */
/*        WORDSIZE and SIZE are a power of 2 numbers (to speed things up). */
/*        Rv64BitsCalc is at least as big as RvUint32 (for casting types up to RvUint32 and RvInt32) */
/*        RvUint64AssignFrom32s assumes WORDSIZE = 16. */
/*        size of RvInt, RvUint, and RvSize_t are >= size of Rv64BitsWord. */
/*        Rv64BitsWord and Rv64BitsCalc are unsigned types. */
typedef RvUint16 Rv64BitsWord; /* base type used to store values */
#define Rv64BitsWordConst RvUint16Const
#define RV_64BITS_WORDSIZE 16  /* size of Rv64BitsWord, in bits (can be less than actual type size) */
#define RV_64BITS_MAXWORD Rv64BitsWordConst(0xFFFF) /* Maximum value of WORDSIZE bits */
#define RV_64BITS_SIZE 4       /* Size of RvInt64 and RvUint64, in words */
typedef RvUint32 Rv64BitsCalc; /* Type used for calculations, must be at least 2x WORDSIZE */
#define Rv64BitsCalcConst RvUint32Const

/* These must be in types.h (words type and size must match settings above)
typedef struct {
	Rv64BitsWord words[RV_64BITS_SIZE];  * data is stored in WORDSIZE bits, any extra bits are 0 *
} RvInt64;

typedef struct {
	Rv64BitsWord words[RV_64BITS_SIZE];  * data is stored in WORDSIZE bits, any extra bits are 0 *
} RvUint64;
*/

/* defines for ease of use */
#define RV_64BITS_TOTALBITS (RV_64BITS_SIZE * RV_64BITS_WORDSIZE)
#define RV_64BITS_WORDHIGHBIT ((Rv64BitsWord)(1 << (RV_64BITS_WORDSIZE - 1)))
#define RV_64BITS_WORDSIGNBIT RV_64BITS_WORDHIGHBIT /* Sign bit in word 0 */

/**************** Miscellaneous Internal Operations **************/

/* Find size of RvUint Assumes it is a multiple of 8 bits. */
static RvInt Rv64BitsRvUintSize(void)
{
	RvUint val;
	RvInt numbits;

	val = ~RvUintConst(0);
	numbits = RvIntConst(0);
	while(val > RvUintConst(0)) {
		val >>= 8;
		numbits += RvIntConst(8);
	}
	return numbits;
}

/* Find size of RvSize_t. Assumes it is a multiple of 8 bits. */
static RvInt Rv64BitsRvSize_tSize(void)
{
	RvSize_t val;
	RvInt numbits;

	val = 0;
	val = ~val;
	numbits = RvIntConst(0);
	while(val > 0) {
		val >>= 8;
		numbits += RvIntConst(8);
	}
	return numbits;
}

/* val = 0 */
static void RvUint64Zero(RvUint64 *val)
{
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		val->words[i] = Rv64BitsWordConst(0);
}

/* val++ */
static void RvUint64Inc(RvUint64 *val)
{
	Rv64BitsCalc carry;
	RvInt i;

	carry = Rv64BitsCalcConst(1);
	for(i = (RV_64BITS_SIZE - RvIntConst(1)); i >= RvIntConst(0); i--) {
		carry += (Rv64BitsCalc)(val->words[i]);
		val->words[i] = (Rv64BitsWord)(carry & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		carry >>= RV_64BITS_WORDSIZE;
	}
}

/* Return highest bit set: 64..1, 0 = no bits set */
static RvInt RvUint64MSB(RvUint64 *val)
{
	RvInt i, result;
	Rv64BitsWord bitmask;

	result = RV_64BITS_TOTALBITS;
	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val->words[i] != Rv64BitsWordConst(0)) {
			bitmask = RV_64BITS_WORDHIGHBIT;
			while(bitmask != Rv64BitsWordConst(0)) {
				if((val->words[i] & bitmask) != RvIntConst(0))
					return result;
				bitmask >>= 1;
				result--;
			}
		} else result -= RvIntConst(RV_64BITS_WORDSIZE);
	}
	return RvIntConst(0);
}

/* val *= -1 */
static void RvInt64Negate(RvInt64 *val)
{
	Rv64BitsCalc carry;
	RvInt i;

	carry = Rv64BitsCalcConst(1);
	for(i = (RV_64BITS_SIZE - RvIntConst(1)); i >= RvIntConst(0); i--) {
		carry += (Rv64BitsCalc)(val->words[i] ^ RV_64BITS_MAXWORD);
		val->words[i] = (Rv64BitsWord)(carry & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		carry >>= RV_64BITS_WORDSIZE;
	}
}

/* val <<= 1 : Works for signed and unsigned values */
static void RvUint64ShiftLeft1(RvUint64 *val)
{
	Rv64BitsCalc carry;
	RvInt i;

	carry = Rv64BitsCalcConst(0);
	for(i = (RV_64BITS_SIZE - RvIntConst(1)); i >= RvIntConst(0); i--) {
		carry |= ((Rv64BitsCalc)val->words[i] << 1);
		val->words[i] = (Rv64BitsWord)(carry & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		carry >>= RV_64BITS_WORDSIZE;
	}
}

/* val >>= numbits : Works for signed and unsigned values */
/* Does not sign extend negative values. */
static void RvUint64ShiftRight1(RvUint64 *val)
{
	Rv64BitsCalc carry;
	RvInt i;

	carry = Rv64BitsCalcConst(0);
	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		carry |= (Rv64BitsCalc)val->words[i];
		val->words[i] = (Rv64BitsWord)((carry >> 1) & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		carry <<= RV_64BITS_WORDSIZE;
	}
}

/****************** Basic Assignments ********************/

/* This has to be changed if WORDSIZE != 16. Also works for RvInt64. */
static void RvUintAssignFrom32s(RvUint64 *result, RvUint32 ms32, RvUint32 ls32)
{
	result->words[3] = (Rv64BitsWord)(ls32 & (RvUint32)RV_64BITS_MAXWORD);
	result->words[2] = (Rv64BitsWord)((ls32 >> RV_64BITS_WORDSIZE) & (RvUint32)RV_64BITS_MAXWORD);
	result->words[1] = (Rv64BitsWord)(ms32 & (RvUint32)RV_64BITS_MAXWORD);
	result->words[0] = (Rv64BitsWord)((ms32 >> RV_64BITS_WORDSIZE) & (RvUint32)RV_64BITS_MAXWORD);
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Assign(RvUint32 ms32, RvUint32 ls32)
{
	RvUint64 result;
	RvUintAssignFrom32s(&result, ms32, ls32);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Assign(RvInt sign, RvUint32 ms32, RvUint32 ls32)
{
	RvInt64 result;

	RvUintAssignFrom32s((RvUint64 *)&result, ms32, ls32);
	if(sign < RvIntConst(0))
		RvInt64Negate(&result);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64ShortAssign(RvInt32 ls32)
{
	RvInt64 result;

	result.words[3] = (Rv64BitsWord)((RvUint32)ls32 & (RvUint32)RV_64BITS_MAXWORD);
	result.words[2] = (Rv64BitsWord)(((RvUint32)ls32 >> RV_64BITS_WORDSIZE) & (RvUint32)RV_64BITS_MAXWORD);
	if(ls32 > RvIntConst(0))	
	{
		result.words[1] = 0;
		result.words[0] = 0;
	}
	else
	{
		result.words[1] = 0xFFFF;
		result.words[0] = 0xFFFF;
	}

	return result;
}

/***************** Mathematical Operations *****************/

/* val << numbits : Works for signed and unsigned values. */
static void RvUint64ShiftLeftN(RvUint64 *result, RvUint64 *val, RvInt numbits)
{
	Rv64BitsCalc carry;
	RvInt i, carrypos, shiftbits, wordshift, wordshiftbits;

	shiftbits = numbits & (RvInt)(RV_64BITS_TOTALBITS - 1); /* Don't overshift */
	wordshiftbits = shiftbits & (RvInt)(RV_64BITS_WORDSIZE - 1);
	wordshift = shiftbits / RV_64BITS_WORDSIZE;
	carry = Rv64BitsCalcConst(0);
	carrypos = RV_64BITS_SIZE - RvIntConst(1) + wordshift;
	for(i = (RV_64BITS_SIZE - RvIntConst(1)); i >= RvIntConst(0); i--) {
		if(carrypos < RV_64BITS_SIZE)
			carry |= ((Rv64BitsCalc)val->words[carrypos] << wordshiftbits);
		result->words[i] = (Rv64BitsWord)(carry & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		carry >>= RV_64BITS_WORDSIZE;
		carrypos--;
	}
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64ShiftLeft(RvUint64 val, RvInt numbits)
{
	RvUint64 result;

	RvUint64ShiftLeftN(&result, &val, numbits);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64ShiftLeft(RvInt64 val, RvInt numbits)
{
	RvInt64 result;

	RvUint64ShiftLeftN((RvUint64 *)&result, (RvUint64 *)&val, numbits);
	return result;
}

/* val >> numbits : Works for signed and unsigned values */
/* Does not sign extend negative values. */
static void RvUint64ShiftRightN(RvUint64 *result, RvUint64 *val, RvInt numbits)
{
	Rv64BitsCalc carry;
	RvInt i, carrypos, shiftbits, wordshift, wordshiftbits;

	shiftbits = numbits & (RvInt)(RV_64BITS_TOTALBITS - 1); /* Don't overshift */
	wordshiftbits = (RvInt)RV_64BITS_WORDSIZE - (shiftbits & (RvInt)(RV_64BITS_WORDSIZE - 1));
	wordshift = shiftbits / RV_64BITS_WORDSIZE;
	carry = Rv64BitsCalcConst(0);
	carrypos =  Rv64BitsCalcConst(0) - wordshift;
	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(carrypos >= RvIntConst(0))
			carry |= ((Rv64BitsCalc)val->words[carrypos] << wordshiftbits);
		result->words[i] = (Rv64BitsWord)((carry >> RV_64BITS_WORDSIZE) & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		carry <<= RV_64BITS_WORDSIZE;
		carrypos++;
	}
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64ShiftRight(RvUint64 val, RvInt numbits)
{
	RvUint64 result;

	RvUint64ShiftRightN(&result, &val, numbits);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64ShiftRight(RvInt64 val, RvInt numbits)
{
	RvInt64 result;

	RvUint64ShiftRightN((RvUint64 *)&result, (RvUint64 *)&val, numbits);
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Not(RvUint64 val)
{
	RvInt64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = (RvUint16)(val.words[i] ^ RV_64BITS_MAXWORD);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Not(RvInt64 val)
{
	RvInt64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = (RvUint16)(val.words[i] ^ RV_64BITS_MAXWORD);
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64And(RvUint64 val1, RvUint64 val2)
{
	RvInt64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = (RvUint16)(val1.words[i] & val2.words[i]);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64And(RvInt64 val1, RvInt64 val2)
{
	RvInt64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = (RvUint16)(val1.words[i] & val2.words[i]);
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Or(RvUint64 val1, RvUint64 val2)
{
	RvInt64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = (RvUint16)(val1.words[i] | val2.words[i]);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Or(RvInt64 val1, RvInt64 val2)
{
	RvInt64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = (RvUint16)(val1.words[i] | val2.words[i]);
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Xor(RvUint64 val1, RvUint64 val2)
{
	RvUint64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = (RvUint16)(val1.words[i] ^ val2.words[i]);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Xor(RvInt64 val1, RvInt64 val2)
{
	RvInt64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = (RvUint16)(val1.words[i] ^ val2.words[i]);
	return result;
}

/* val1 + val2 : Works for signed and unsigned values. */
static void RvUint64Addition(RvUint64 *result, RvUint64 *val1, RvUint64 *val2)
{
	Rv64BitsCalc carry;
	RvInt i;

	carry = Rv64BitsCalcConst(0);
	for(i = (RV_64BITS_SIZE - 1); i >= 0; i--) {
		carry += (Rv64BitsCalc)val1->words[i] + (Rv64BitsCalc)val2->words[i];
		result->words[i] = (Rv64BitsWord)(carry & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		carry >>= RV_64BITS_WORDSIZE;
	}
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Add(RvUint64 val1, RvUint64 val2)
{
	RvUint64 result;

	RvUint64Addition(&result, &val1, &val2);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Add(RvInt64 val1, RvInt64 val2)
{
	RvInt64 result;

	RvUint64Addition((RvUint64 *)&result, (RvUint64 *)&val1, (RvUint64 *)&val2);
	return result;
}

/* val1 - val2 : Works for signed and unsigned values. */
static void RvUint64Subtraction(RvUint64 *result, RvUint64 *val1, RvUint64 *val2)
{
	Rv64BitsCalc carry;
	RvInt i;

	carry = Rv64BitsCalcConst(0);
	for(i = (RV_64BITS_SIZE - 1); i >= RvIntConst(0); i--) {
		carry = (Rv64BitsCalc)val1->words[i] - (Rv64BitsCalc)val2->words[i] - carry;
		result->words[i] = (Rv64BitsWord)(carry & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		carry = (carry >> RV_64BITS_WORDSIZE) & Rv64BitsCalcConst(1);
	}
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Sub(RvUint64 val1, RvUint64 val2)
{
	RvUint64 result;

	RvUint64Subtraction(&result, &val1, &val2);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Sub(RvInt64 val1, RvInt64 val2)
{
	RvInt64 result;

	RvUint64Subtraction((RvUint64 *)&result, (RvUint64 *)&val1, (RvUint64 *)&val2);
	return result;
}

/* val1 * val2 : Works for signed and unsigned values. */
static void RvUint64Multiplication(RvUint64 *result, RvUint64 *val1, RvUint64 *val2)
{
	RvInt resultdigit, digit1, digit2;
	Rv64BitsCalc carry, colsum, lastsum, carryover;

	colsum = Rv64BitsCalcConst(0);
	carryover = ((~colsum) >> RV_64BITS_WORDSIZE) + Rv64BitsCalcConst(1); /* Do this once for speed */
	for(resultdigit = (RV_64BITS_SIZE - 1); resultdigit >= RvIntConst(0); resultdigit--) {
		/* cross multiply for each column */
		carry = Rv64BitsCalcConst(0);
		for(digit1 = resultdigit, digit2 = (RV_64BITS_SIZE - 1); digit2 >= resultdigit; digit1++, digit2--) {
			lastsum = colsum;
			colsum += (Rv64BitsCalc)val1->words[digit1] * (Rv64BitsCalc)val2->words[digit2];
			if(colsum < lastsum)
				carry += carryover; /* overflow */
		}
		result->words[resultdigit] = (Rv64BitsWord)(colsum & (Rv64BitsCalc)RV_64BITS_MAXWORD);
		colsum = (colsum >> RV_64BITS_WORDSIZE) + carry; /* carry extra to next column */
	}
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Mul(RvUint64 val1, RvUint64 val2)
{
	RvUint64 result;

	RvUint64Multiplication(&result, &val1, &val2);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Mul(RvInt64 val1, RvInt64 val2)
{
	RvInt64 result;

	RvUint64Multiplication((RvUint64 *)&result, (RvUint64 *)&val1, (RvUint64 *)&val2);
	return result;
}

/* val1 / val2 : Unsigned only. Return result and remainder. */
static void RvUint64Divide(RvUint64 *result, RvUint64 *remainder, RvUint64 *val1, RvUint64 *val2)
{
	RvInt msb1, msb2, bitcount;
	RvUint64 tmpdivisor, tmpremainder;

	/* Find high bits and deal with obvious cases */
	msb1 = RvUint64MSB(val1);
	msb2 = RvUint64MSB(val2);
	if(msb2 == RvIntConst(0)) {
		bitcount = msb1 / msb2; /* Signal divide by zero */
		return;
	}
	bitcount = msb1 - msb2;
	if(bitcount < 0) {
		/* val1 < val2 so result is 0 and remainder is val1 */
		RvUint64Zero(result);
		*remainder = *val1;
		return;
	}

	/* Do long division, bit by bit */
	RvUint64Zero(result);
	tmpremainder = *val1;
	RvUint64ShiftLeftN(&tmpdivisor, val2, bitcount);
	for(;;) {
		if(RvUint64IsGreaterThanOrEqual(tmpremainder, tmpdivisor) == RV_TRUE) {
			tmpremainder = RvUint64Sub(tmpremainder, tmpdivisor);
			RvUint64Inc(result);
		}
		if(bitcount == RvIntConst(0))
			break;
		RvUint64ShiftLeft1(result);
		RvUint64ShiftRight1(&tmpdivisor);
		bitcount--;
	}

	*remainder = tmpremainder;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Div(RvUint64 val1, RvUint64 val2)
{
	RvUint64 result, remain;

	RvUint64Divide(&result, &remain, &val1, &val2);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Div(RvInt64 val1, RvInt64 val2)
{
	RvInt64 result, remain;
	Rv64BitsWord val1sign, val2sign;

	val1sign = (Rv64BitsWord)(val1.words[0] & RV_64BITS_WORDSIGNBIT);
	val2sign = (Rv64BitsWord)(val2.words[0] & RV_64BITS_WORDSIGNBIT);
	if(val1sign != Rv64BitsWordConst(0))
		RvInt64Negate(&val1);
	if(val2sign != Rv64BitsWordConst(0))
		RvInt64Negate(&val2);

	RvUint64Divide((RvUint64 *)&result, (RvUint64 *)&remain, (RvUint64 *)&val1, (RvUint64 *)&val2);

	if((val1sign ^ val2sign) != Rv64BitsWordConst(0))
		RvInt64Negate(&result); /* result is negative */
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64Mod(RvUint64 val1, RvUint64 val2)
{
	RvUint64 result, remain;

	RvUint64Divide(&result, &remain, &val1, &val2);
	return remain;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64Mod(RvInt64 val1, RvInt64 val2)
{
	RvInt64 result, remain;
	Rv64BitsWord val1sign, val2sign;

	val1sign = (Rv64BitsWord)(val1.words[0] & RV_64BITS_WORDSIGNBIT);
	val2sign = (Rv64BitsWord)(val2.words[0] & RV_64BITS_WORDSIGNBIT);
	if(val1sign != Rv64BitsWordConst(0))
		RvInt64Negate(&val1);
	if(val2sign != Rv64BitsWordConst(0))
		RvInt64Negate(&val2);

	RvUint64Divide((RvUint64 *)&result, (RvUint64 *)&remain, (RvUint64 *)&val1, (RvUint64 *)&val2);

	if(val1sign != Rv64BitsWordConst(0))
		RvInt64Negate(&remain);  /* remainder is negative */
	return remain;
}

/***************** Cast from rvInt64 to RvUint64 and back *********************/

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvInt64(RvInt64 val)
{
	RvUint64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = val.words[i];
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvUint64(RvUint64 val)
{
	RvInt64 result = RvInt64Const(0,0,0);
	RvInt i;

	for(i = RvIntConst(0); i < RV_64BITS_SIZE; i++)
		result.words[i] = val.words[i];
	return result;
}

/***************** Cast from base types to RvInt64 and RvUint64 *********************/

/* Cast from an unsigned value to a RvUint64 given the parameters of the orginal val. */
/* The size of the type (and thus, numbits) must be <= the size of Rv64BitsCalc. */
/* Can also be used for RvInt64. */
static void RvUint64CastFromUnsigned(RvUint64 *result, Rv64BitsCalc val, RvInt numbits)
{
	Rv64BitsCalc curval;
	RvInt i, curbits;

	/* Set each word */
	curval = val;
	curbits = numbits;
	for(i = (RV_64BITS_SIZE - RvIntConst(1)); i >= RvIntConst(0); i--) {
		if(curbits > RvIntConst(0)) {
			/* Real data avalable for this word */
			result->words[i] = (Rv64BitsWord)(curval & (Rv64BitsCalc)RV_64BITS_MAXWORD);
			curval >>= RV_64BITS_WORDSIZE;
			curbits -= RV_64BITS_WORDSIZE;
		} else result->words[i] = Rv64BitsWordConst(0);
	}
}

/* Cast from a signed value to RvInt64 given the parameters of the orginal val. */
/* The size of the type (and thus, numbits) must be <= the size of Rv64BitsCalc. */
/* Can also be used for RvUint64. */
static void RvInt64CastFromSigned(RvInt64 *result, Rv64BitsCalc val, RvInt numbits, Rv64BitsCalc maxval, Rv64BitsCalc signbit)
{
	Rv64BitsCalc uval, valsign;

	/* Check for sign and set sign extenders appropriately. */
	valsign = val & signbit;
	if(valsign != Rv64BitsCalcConst(0)) {
		uval = (~val & maxval) + Rv64BitsCalcConst(1);
	} else uval = val;

	RvUint64CastFromUnsigned((RvUint64 *)result, uval, numbits);

	if(valsign != Rv64BitsCalcConst(0))
		RvInt64Negate(result); /* Put minus sign back */
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvUint32(RvUint32 val)
{
	RvUint64 result;

	RvUint64CastFromUnsigned(&result, (Rv64BitsCalc)val, RvIntConst(32));
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvInt32(RvInt32 val)
{
	RvUint64 result;

	RvInt64CastFromSigned((RvInt64 *)&result, (Rv64BitsCalc)val, RvIntConst(32), Rv64BitsCalcConst(0xFFFFFFFF), Rv64BitsCalcConst(0x80000000));
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvUint32(RvUint32 val)
{
	RvInt64 result;

	RvUint64CastFromUnsigned((RvUint64 *)&result, (Rv64BitsCalc)val, RvIntConst(32));
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvInt32(RvInt32 val)
{
	RvInt64 result;

	RvInt64CastFromSigned(&result, (Rv64BitsCalc)val, RvIntConst(32), Rv64BitsCalcConst(0xFFFFFFFF), Rv64BitsCalcConst(0x80000000));
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvUint16(RvUint16 val)
{
	RvUint64 result;

	RvUint64CastFromUnsigned(&result, (Rv64BitsCalc)val, RvIntConst(16));
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvInt16(RvInt16 val)
{
	RvUint64 result;

	RvInt64CastFromSigned((RvInt64 *)&result, (Rv64BitsCalc)val, RvIntConst(16), Rv64BitsCalcConst(0xFFFF), Rv64BitsCalcConst(0x8000));
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvUint16(RvUint16 val)
{
	RvInt64 result;

	RvUint64CastFromUnsigned((RvUint64 *)&result, (Rv64BitsCalc)val, RvIntConst(16));
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvInt16(RvInt16 val)
{
	RvInt64 result;

	RvInt64CastFromSigned(&result, (Rv64BitsCalc)val, RvIntConst(16), Rv64BitsCalcConst(0xFFFF), Rv64BitsCalcConst(0x8000));
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvUint8(RvUint8 val)
{
	RvUint64 result;

	RvUint64CastFromUnsigned(&result, (Rv64BitsCalc)val, RvIntConst(8));
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvInt8(RvInt8 val)
{
	RvUint64 result;

	RvInt64CastFromSigned((RvInt64 *)&result, (Rv64BitsCalc)val, RvIntConst(8), Rv64BitsCalcConst(0xFF), Rv64BitsCalcConst(0x80));
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvUint8(RvUint8 val)
{
	RvInt64 result;

	RvUint64CastFromUnsigned((RvUint64 *)&result, (Rv64BitsCalc)val, RvIntConst(8));
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvInt8(RvInt8 val)
{
	RvInt64 result;

	RvInt64CastFromSigned(&result, (Rv64BitsCalc)val, RvIntConst(8), Rv64BitsCalcConst(0xFF), Rv64BitsCalcConst(0x80));
	return result;
}

/* Cast from RvUint to RvUint64. Can also be used for RvInt64. */
static void RvUint64CastFromRvUint(RvUint64 *result, RvUint val)
{
	RvInt i, numbits;

	numbits = Rv64BitsRvUintSize(); /* Find size of RvUint */

	/* Set each word */
	for(i = (RV_64BITS_SIZE - RvIntConst(1)); i >= RvIntConst(0); i--) {
		if(numbits > RvIntConst(0)) {
			/* Real data avalable for this word */
			result->words[i] = (Rv64BitsWord)(val & (RvUint)RV_64BITS_MAXWORD);
			val >>= RV_64BITS_WORDSIZE;
			numbits -= RV_64BITS_WORDSIZE;
		} else result->words[i] = Rv64BitsWordConst(0);
	}
}

/* Cast from RvInt to RvInt64. Can also be used for RvUint64. */
static void RvInt64CastFromRvInt(RvInt64 *result, RvInt val)
{
	RvUint uval, valsign;

	/* Find sign and convert to positive number if needed. */
	uval = ~RvUintConst(0);
	valsign = (RvUint)val & (~(uval >> 1));
	if(valsign != RvIntConst(0)) {
		uval = (RvUint)((val + RvIntConst(1)) * RvIntConst(-1)) + RvUintConst(1);
	} else uval = (RvUint)val;

	RvUint64CastFromRvUint((RvUint64 *)result, uval);

	if(valsign != RvIntConst(0))
		RvInt64Negate(result); /* Put the minus sign back */
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvUint(RvUint val)
{
	RvUint64 result;

	RvUint64CastFromRvUint(&result, val);
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvInt(RvInt val)
{
	RvUint64 result;

	RvInt64CastFromRvInt((RvInt64 *)&result, val);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvUint(RvUint val)
{
	RvInt64 result;

	RvUint64CastFromRvUint((RvUint64 *)&result, val);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvInt(RvInt val)
{
	RvUint64 result;

	RvInt64CastFromRvInt(&result, val);
	return result;
}

/* Cast from RvSize_t to RvUint64. Assumes size of RvSize_t is a */
/* multiple of 8 bits and that it is unsigned. */
static void RvUint64CastFromRvSize_t(RvUint64 *result, RvSize_t val)
{
	RvInt i, numbits;

	numbits = Rv64BitsRvSize_tSize(); /* Find size of RvSize_t */

	/* Set each word */
	for(i = (RV_64BITS_SIZE - RvIntConst(1)); i >= RvIntConst(0); i--) {
		if(numbits > RvIntConst(0)) {
			/* Real data avalable for this word */
			result->words[i] = (Rv64BitsWord)(val & (RvSize_t)RV_64BITS_MAXWORD);
			val >>= RV_64BITS_WORDSIZE;
			numbits -= RV_64BITS_WORDSIZE;
		} else result->words[i] = Rv64BitsWordConst(0);
	}
}

/* Assumes size of RvSize_t is a multiple of 8 bits. */
RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvSize_t(RvSize_t val)
{
	RvUint64 result;
	
	RvUint64CastFromRvSize_t(&result, val);
	return result;
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvSize_t(RvSize_t val)
{
	RvInt64 result;

	RvUint64CastFromRvSize_t((RvUint64 *)&result, val);
	return result;
}

RVCOREAPI RvUint64 RVCALLCONV RvUint64FromRvChar(RvChar val)
{
	return RvUint64FromRvInt((RvInt)val);
}

RVCOREAPI RvInt64 RVCALLCONV RvInt64FromRvChar(RvChar val)
{
	return RvInt64FromRvInt((RvInt)val);
}

/***************** Cast to base types from RvInt64 and RvUint64 *********************/

/* Cast to a value from a RvUint64 given the parameters of value. Signed */
/* and unsigned are the same since the RvInt64 is sign extended. */
/* The size of the type (and thus, numbits) must be <= the size of Rv64BitsCalc. */
static Rv64BitsCalc RvUint64CastTo(RvUint64 *val, RvInt numbits, Rv64BitsCalc maxresult)
{
	Rv64BitsCalc result;
	RvInt i, curbits;

	/* Get each word */
	result = Rv64BitsCalcConst(0);
	curbits = (RvInt)(RV_64BITS_TOTALBITS - RV_64BITS_WORDSIZE);
	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(curbits < numbits) {
			/* result has room for at least part of this word */
			result = (result << RV_64BITS_WORDSIZE) | (Rv64BitsCalc)val->words[i];
		}
		curbits -= RV_64BITS_WORDSIZE;
	}

	result &= maxresult;
	return result;
}

RVCOREAPI RvUint32 RVCALLCONV RvUint64ToRvUint32(RvUint64 val)
{
	return (RvUint32)RvUint64CastTo(&val, RvIntConst(32), Rv64BitsCalcConst(0xFFFFFFFF));
}

RVCOREAPI RvInt32 RVCALLCONV RvUint64ToRvInt32(RvUint64 val)
{
	return (RvInt32)RvUint64CastTo(&val, RvIntConst(32), Rv64BitsCalcConst(0xFFFFFFFF));
}

RVCOREAPI RvUint32 RVCALLCONV RvInt64ToRvUint32(RvInt64 val)
{
	return (RvUint32)RvUint64CastTo((RvUint64 *)&val, RvIntConst(32), Rv64BitsCalcConst(0xFFFFFFFF));
}

RVCOREAPI RvInt32 RVCALLCONV RvInt64ToRvInt32(RvInt64 val)
{
	return (RvInt32)RvUint64CastTo((RvUint64 *)&val, RvIntConst(32), Rv64BitsCalcConst(0xFFFFFFFF));
}

RVCOREAPI RvUint16 RVCALLCONV RvUint64ToRvUint16(RvUint64 val)
{
	return (RvUint16)RvUint64CastTo(&val, RvIntConst(16), Rv64BitsCalcConst(0xFFFF));
}

RVCOREAPI RvInt16 RVCALLCONV RvUint64ToRvInt16(RvUint64 val)
{
	return (RvInt16)RvUint64CastTo(&val, RvIntConst(16), Rv64BitsCalcConst(0xFFFF));
}

RVCOREAPI RvUint16 RVCALLCONV RvInt64ToRvUint16(RvInt64 val)
{
	return (RvUint16)RvUint64CastTo((RvUint64 *)&val, RvIntConst(16), Rv64BitsCalcConst(0xFFFF));
}

RVCOREAPI RvInt16 RVCALLCONV RvInt64ToRvInt16(RvInt64 val)
{
	return (RvInt16)RvUint64CastTo(&val, RvIntConst(16), Rv64BitsCalcConst(0xFFFF));
}

RVCOREAPI RvUint8 RVCALLCONV RvUint64ToRvUint8(RvUint64 val)
{
	return (RvUint8)RvUint64CastTo(&val, RvIntConst(8), Rv64BitsCalcConst(0xFF));
}

RVCOREAPI RvInt8 RVCALLCONV RvUint64ToRvInt8(RvUint64 val)
{
	return (RvInt8)RvUint64CastTo(&val, RvIntConst(8), Rv64BitsCalcConst(0xFF));
}

RVCOREAPI RvUint8 RVCALLCONV RvInt64ToRvUint8(RvInt64 val)
{
	return (RvUint8)RvUint64CastTo((RvUint64 *)&val, RvIntConst(8), Rv64BitsCalcConst(0xFF));
}

RVCOREAPI RvInt8 RVCALLCONV RvInt64ToRvInt8(RvInt64 val)
{
	return (RvInt8)RvUint64CastTo((RvUint64 *)&val, RvIntConst(8), Rv64BitsCalcConst(0xFF));
}

/* Cast to a RvUint from RvUint64. Cal also be used for RvInt64. */
/* Assumes size of RvUint is a multiple of 8 bits. */
static RvUint RvUint64CastToRvUint(RvUint64 *val)
{
	RvInt i, curbits;
	RvUint result;
	RvInt numbits;

	result = RvUintConst(0);
	numbits = Rv64BitsRvUintSize(); /* Find size of RvUint */

	/* Get each word */
	curbits = (RvInt)(RV_64BITS_TOTALBITS - RV_64BITS_WORDSIZE);
	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(curbits < numbits) {
			/* result has room for at least part of this word */
			result = (result << RV_64BITS_WORDSIZE) | (RvUint)val->words[i];
		}
		curbits -= RV_64BITS_WORDSIZE;
	}

	return result;
}

RVCOREAPI RvUint RVCALLCONV RvUint64ToRvUint(RvUint64 val)
{
	return RvUint64CastToRvUint(&val);
}

RVCOREAPI RvInt RVCALLCONV RvUint64ToRvInt(RvUint64 val)
{
	return (RvInt)RvUint64CastToRvUint(&val);
}

RVCOREAPI RvUint RVCALLCONV RvInt64ToRvUint(RvInt64 val)
{
	return RvUint64CastToRvUint((RvUint64 *)&val);
}

RVCOREAPI RvInt RVCALLCONV RvInt64ToRvInt(RvInt64 val)
{
	return (RvInt)RvUint64CastToRvUint((RvUint64 *)&val);
}

/* Cast from to a RvSize_t from a RvUint64. Can also be used for */
/* a RvInt64. Assumes size of RvSize_t is a multiple of 8 bits and */
/* that it is unsigned. */
static RvSize_t RvUint64CastToRvSize_t(RvUint64 *val)
{
	RvSize_t result;
	RvInt i, curbits;
	RvInt numbits;

	result = 0;
	numbits = Rv64BitsRvSize_tSize(); /* Find size of RvSize_t */

	/* Get each word */
	curbits = (RvInt)(RV_64BITS_TOTALBITS - RV_64BITS_WORDSIZE);
	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(curbits < numbits) {
			/* result has room for at least part of this word */
			result = (result << RV_64BITS_WORDSIZE) | (RvSize_t)val->words[i];
		}
		curbits -= RV_64BITS_WORDSIZE;
	}

	return result;
}

RVCOREAPI RvSize_t RVCALLCONV RvUint64ToRvSize_t(RvUint64 val)
{
	return RvUint64CastToRvSize_t(&val);
}

RVCOREAPI RvSize_t RVCALLCONV RvInt64ToRvSize_t(RvInt64 val)
{
	return RvUint64CastToRvSize_t((RvUint64 *)&val);
}

RVCOREAPI RvChar RVCALLCONV RvUint64ToRvChar(RvUint64 val)
{
	return (RvChar)RvUint64ToRvInt(val);
}

RVCOREAPI RvChar RVCALLCONV RvInt64ToRvChar(RvInt64 val)
{
	return (RvChar)RvInt64ToRvInt(val);
}

/***************** Logical Operations *****************/

RVCOREAPI RvBool RVCALLCONV RvUint64IsEqual(RvUint64 val1, RvUint64 val2)
{
	RvInt i;

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i])
			return RV_FALSE;
	}
	return RV_TRUE;
}

RVCOREAPI RvBool RVCALLCONV RvInt64IsEqual(RvInt64 val1, RvInt64 val2)
{
	RvInt i;

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i])
			return RV_FALSE;
	}
	return RV_TRUE;
}

RVCOREAPI RvBool RVCALLCONV RvUint64IsLessThan(RvUint64 val1, RvUint64 val2)
{
	RvInt i;

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i]) {
			if(val1.words[i] < val2.words[i])
				return RV_TRUE; /* val1 < val2 */
			return RV_FALSE;    /* val1 > val2 */
		}
	}
	return RV_FALSE; /* val1 = val2 */
}

RVCOREAPI RvBool RVCALLCONV RvInt64IsLessThan(RvInt64 val1, RvInt64 val2)
{
	RvInt i;
	Rv64BitsWord val1neg, val2neg;

	val1neg = (Rv64BitsWord)(val1.words[0] & RV_64BITS_WORDSIGNBIT);
	val2neg = (Rv64BitsWord)(val2.words[0] & RV_64BITS_WORDSIGNBIT);
	if((val1neg ^ val2neg) != Rv64BitsWordConst(0)) {
		/* Numbers have opposite signs so we must use simple method */
		if(val1neg != Rv64BitsWordConst(0))
			return RV_TRUE; /* val1 is negative */
		return RV_FALSE;    /* val2 is negative */
	}

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i]) {
			if(val1.words[i] < val2.words[i])
				return RV_TRUE; /* val1 < val2 */
			return RV_FALSE;    /* val1 > val2 */
		}
	}
	return RV_FALSE; /* val1 = val2 */
}

RVCOREAPI RvBool RVCALLCONV RvUint64IsGreaterThan(RvUint64 val1, RvUint64 val2)
{
	RvInt i;

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i]) {
			if(val1.words[i] > val2.words[i])
				return RV_TRUE; /* val1 > val2 */
			return RV_FALSE;    /* val1 < val2 */
		}
	}
	return RV_FALSE; /* val1 = val2 */
}

RVCOREAPI RvBool RVCALLCONV RvInt64IsGreaterThan(RvInt64 val1, RvInt64 val2)
{
	RvInt i;
	Rv64BitsWord val1neg, val2neg;

	val1neg = (Rv64BitsWord)(val1.words[0] & RV_64BITS_WORDSIGNBIT);
	val2neg = (Rv64BitsWord)(val2.words[0] & RV_64BITS_WORDSIGNBIT);
	if((val1neg ^ val2neg) != Rv64BitsWordConst(0)) {
		/* Numbers have opposite signs so we must use simple method */
		if(val2neg != Rv64BitsWordConst(0))
			return RV_TRUE; /* val2 is negative */
		return RV_FALSE;    /* val1 is negative */
	}

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i]) {
			if(val1.words[i] > val2.words[i])
				return val1neg == Rv64BitsWordConst(0) ? RV_TRUE:RV_FALSE;
			return val1neg == Rv64BitsWordConst(0) ? RV_FALSE:RV_TRUE;
		}
	}
	return RV_FALSE; /* val1 = val2 */
}

RVCOREAPI RvBool RVCALLCONV RvUint64IsLessThanOrEqual(RvUint64 val1, RvUint64 val2)
{
	RvInt i;

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i]) {
			if(val1.words[i] < val2.words[i])
				return RV_TRUE; /* val1 < val2 */
			return RV_FALSE;    /* val1 > val2 */
		}
	}
	return RV_TRUE; /* val1 = val2 */
}

RVCOREAPI RvBool RVCALLCONV RvInt64IsLessThanOrEqual(RvInt64 val1, RvInt64 val2)
{
	RvInt i;
	Rv64BitsWord val1neg, val2neg;

	val1neg = (Rv64BitsWord)(val1.words[0] & RV_64BITS_WORDSIGNBIT);
	val2neg = (Rv64BitsWord)(val2.words[0] & RV_64BITS_WORDSIGNBIT);
	if((val1neg ^ val2neg) != Rv64BitsWordConst(0)) {
		/* Numbers have opposite signs so we must use simple method */
		if(val1neg != Rv64BitsWordConst(0))
			return RV_TRUE; /* val1 is negative */
		return RV_FALSE;    /* val2 is negative */
	}

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i]) {
			if(val1.words[i] < val2.words[i])
				return val1neg == Rv64BitsWordConst(0) ? RV_TRUE:RV_FALSE;  
			return val1neg == Rv64BitsWordConst(0) ? RV_FALSE:RV_TRUE;  
		}
	}
	return RV_TRUE; /* val1 = val2 */
}

RVCOREAPI RvBool RVCALLCONV RvUint64IsGreaterThanOrEqual(RvUint64 val1, RvUint64 val2)
{
	RvInt i;

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i]) {
			if(val1.words[i] > val2.words[i])
				return RV_TRUE; /* val1 > val2 */
			return RV_FALSE;    /* val1 < val2 */
		}
	}
	return RV_TRUE; /* val1 = val2 */
}

RVCOREAPI RvBool RVCALLCONV RvInt64IsGreaterThanOrEqual(RvInt64 val1, RvInt64 val2)
{
	RvInt i;
	Rv64BitsWord val1neg, val2neg;

	val1neg = (Rv64BitsWord)(val1.words[0] & RV_64BITS_WORDSIGNBIT);
	val2neg = (Rv64BitsWord)(val2.words[0] & RV_64BITS_WORDSIGNBIT);
	if((val1neg ^ val2neg) != Rv64BitsWordConst(0)) {
		/* Numbers have opposite signs so we must use simple method */
		if(val2neg != Rv64BitsWordConst(0))
			return RV_TRUE; /* val2 is negative */
		return RV_FALSE;    /* val1 is negative */
	}

	for(i =	RvIntConst(0); i < RV_64BITS_SIZE; i++) {
		if(val1.words[i] != val2.words[i]) {
			if(val1.words[i] > val2.words[i])
			   return val1neg == Rv64BitsWordConst(0) ? RV_TRUE:RV_FALSE; /* val1 > val2 */
			return val1neg == Rv64BitsWordConst(0) ? RV_FALSE:RV_TRUE;    /* val1 < val2 */
		}
	}
	return RV_TRUE; /* val1 = val2 */
}

#else
int prevent_warning_of_ranlib_has_no_symbols_rv64bits=0;
#endif /* RV_64BITS_TYPE == RV_64BITS_MANUAL */

