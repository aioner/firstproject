/***********************************************************************
Filename   : rvtypes.h
Description: type definitions for all RADVISION modules
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
#ifndef RV_TYPES_H
#define RV_TYPES_H

#include <stddef.h>
#include "rvconfig.h"

/*@*****************************************************************************
* Package: RvCommonPkg (root)
* -----------------------------------------------------------------------------
* Title: Common
*
* General: This package contains data structures and types used throughout the
*          Diameter Stack. These include a hash table and a thread-safe object
*          allocation module.
****************************************************************************@*/

/*@*****************************************************************************
* Package: RvCommonBaseTypesPkg (RvCommonPkg)
* -----------------------------------------------------------------------------
* Title: Base Types
*
* General: This package contains basic types defined in the RADVISION Common
*          Core layer.
****************************************************************************@*/


/* A number of constants need to be defined in the configuration headers
   in order for this file to work properly. The values should be set
   properly for the architecture, os, compiler, memory model, etc. The
   definitions required are:

   RV_SIZET_TYPE          size_t type (should always be size_t)
   RV_PTRDIFFT_TYPE       ptrdiff_t type (should always be ptrdiff_t)
   RV_CHAR_TYPE           standard character type (for char and char * only)

   RV_VAR_INT_TYPE        standard variable (non-fixed) size signed variable
   RV_VAR_UINT_TYPE       standard variable (non-fixed) size unsigned variable
   RV_SIGNED_INT8_TYPE    8 bit signed variable
   RV_UNSIGNED_INT8_TYPE  8 bit unsigned variable
   RV_SIGNED_INT16_TYPE   16 bit signed variable
   RV_UNSIGNED_INT16_TYPE 16 bit unsigned variable
   RV_SIGNED_INT32_TYPE   32 bit signed variable
   RV_UNSIGNED_INT32_TYPE 32 bit unsigned variable
   RV_SIGNED_INT64_TYPE   64 bit signed variable
   RV_UNSIGNED_INT64_TYPE 64 bit unsigned variable

   RV_VAR_INT_SUFFIX(n)        standard variable (non-fixed) size signed constant suffix
   RV_VAR_UINT_SUFFIX(n)       standard variable (non-fixed) size unsigned constant suffix
   RV_SIGNED_INT8_SUFFIX(n)    8 bit signed constant suffix
   RV_UNSIGNED_INT8_SUFFIX(n)  8 bit ubsigned constant suffix
   RV_SIGNED_INT16_SUFFIX(n)   16 bit signed constant suffix
   RV_UNSIGNED_INT16_SUFFIX(n) 16 bit unsigned constant suffix
   RV_SIGNED_INT32_SUFFIX(n)   32 bit signed constant suffix
   RV_UNSIGNED_INT32_SUFFIX(n) 32 bit unsigned constant suffix
   RV_SIGNED_INT64_SUFFIX(n)   64 bit signed constant suffix
   RV_UNSIGNED_INT64_SUFFIX(n) 64 bit unsigned constant suffix

   RV_VAR_INT_MAX  maximum value of the standard variable (non-fixed) size signed variable
   RV_VAR_INT_MIN  minimum value of the standard variable (non-fixed) size signed variable
   RV_VAR_UINT_MAX maximum value of the standard variable (non-fixed) size unsigned variable
*/

#if defined(__cplusplus)
extern "C" {
#endif

typedef double RvDouble;


/* Our types for size_t and ptrdiff_t, should never be anything */
/* different than ANSI standard but we define it just in case. */
/*@*****************************************************************************
* Type: RvSize_t (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* A variable that represents size. Usually equivalent to (size_t).
****************************************************************************@*/

typedef RV_SIZET_TYPE RvSize_t;
typedef RV_PTRDIFFT_TYPE RvPtrdiff_t;

/* Character type specifically for characters, not values */
/*@*****************************************************************************
* Type: RvChar (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* A char type.
 ****************************************************************************@*/
typedef RV_CHAR_TYPE RvChar;

/* Variable size types, set to optimum variable size */
typedef RV_VAR_INT_TYPE RvInt;
#define RvIntConst(_n) (RV_VAR_INT_SUFFIX(_n))
#define RV_INT_MAX RV_VAR_INT_MAX
#define RV_INT_MIN RV_VAR_INT_MIN

typedef RV_VAR_UINT_TYPE RvUint;
#define RvUintConst(_n) (RV_VAR_UINT_SUFFIX(_n))
#define RV_UINT_MAX RV_VAR_UINT_MAX
#define RV_UINT_MIN RvUintConst(0)

typedef RV_VAR_LONG_TYPE RvLong;
typedef RV_VAR_ULONG_TYPE RvUlong;

/*@*****************************************************************************
* Type: RvInt8 (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* A signed char.
****************************************************************************@*/
/* Fixed size types */
typedef RV_SIGNED_INT8_TYPE RvInt8;
#define RvInt8Const(_n) (RV_SIGNED_INT8_SUFFIX(_n))
#define RV_INT8_MAX RvInt8Const(127)
#define RV_INT8_MIN (RvInt8Const(-127) - RvInt8Const(1))

/*@*****************************************************************************
* Type: RvUint8 (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* An unsigned char.
****************************************************************************@*/
typedef RV_UNSIGNED_INT8_TYPE RvUint8;
#define RvUint8Const(_n) (RV_UNSIGNED_INT8_SUFFIX(_n))
#define RV_UINT8_MAX RvUint8Const(255)
#define RV_UINT8_MIN RvUint8Const(0)

/*@*****************************************************************************
* Type: RvInt16 (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* A signed 16-bit integer (signed RvUint16).
****************************************************************************@*/
typedef RV_SIGNED_INT16_TYPE RvInt16;
#define RvInt16Const(_n) (RV_SIGNED_INT16_SUFFIX(_n))
#define RV_INT16_MAX RvInt16Const(32767)
#define RV_INT16_MIN (RvInt16Const(-32767) - RvInt16Const(1))

/*@*****************************************************************************
* Type: RvUint16 (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* An unsigned 16-bit integer (unsigned RvInt16).
****************************************************************************@*/
typedef RV_UNSIGNED_INT16_TYPE RvUint16;
#define RvUint16Const(_n) (RV_UNSIGNED_INT16_SUFFIX(_n))
#define RV_UINT16_MAX RvUint16Const(65535)
#define RV_UINT16_MIN RvUint16Const(0)

/*@*****************************************************************************
* Type: RvInt32 (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* A signed 32-bit integer (signed RvUint32).
****************************************************************************@*/
typedef RV_SIGNED_INT32_TYPE RvInt32;
#define RvInt32Const(_n) (RV_SIGNED_INT32_SUFFIX(_n))
#define RV_INT32_MAX RvInt32Const(2147483647)
#define RV_INT32_MIN (RvInt32Const(-2147483647) - RvInt32Const(1))

/*@*****************************************************************************
* Type: RvUint32 (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* An unsigned 32-bit integer (unsigned RvInt32).
****************************************************************************@*/
typedef RV_UNSIGNED_INT32_TYPE RvUint32;
#define RvUint32Const(_n) (RV_UNSIGNED_INT32_SUFFIX(_n))
#define RV_UINT32_MAX RvUint32Const(4294967295)
#define RV_UINT32_MIN RvUint32Const(0)

/*@*****************************************************************************
* Type: RvUint64 (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* An unsigned 64-bit integer (unsigned RvInt64).
****************************************************************************@*/

/*@*****************************************************************************
* Type: RvInt64 (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* A signed 64-bit integer (signed RvUint64).
****************************************************************************@*/

#if (RV_64BITS_TYPE == RV_64BITS_STANDARD)
/* standard 64 bit usage */
typedef RV_SIGNED_INT64_TYPE RvInt64;
#define RvInt64ShortConst(_n) (RV_SIGNED_INT64_SUFFIX(_n))
#define RvInt64Const2(_n)     (RV_SIGNED_INT64_SUFFIX(_n))
#define RvInt64Const(_s, _m, _l) ((RvInt64)(_s) * (((RvInt64)(_m) * RV_SIGNED_INT64_SUFFIX(0x100000000)) + (RvInt64)(_l)))
#define RV_INT64_MAX RvInt64Const(1,0x7FFFFFFF,0xFFFFFFFF)
#define RV_INT64_MIN RvInt64Const(-1,0x80000000,0)

typedef RV_UNSIGNED_INT64_TYPE RvUint64;
#define RvUint64ShortConst(_n) (RV_UNSIGNED_INT64_SUFFIX(_n))
#define RvUint64Const2(_n)     (RV_UNSIGNED_INT64_SUFFIX(_n))
#define RvUint64Const(_m, _l) (((RvUint64)(_m) * RV_UNSIGNED_INT64_SUFFIX(0x100000000)) + (RvUint64)(_l))
#define RV_UINT64_MAX RvUint64Const(0xFFFFFFFF,0xFFFFFFFF)
#define RV_UINT64_MIN RvUint64Const(0,0)

#else
/* manual 64 bit usage */
typedef struct {
    RvUint16 words[4];
} Rv64BitsBase;

typedef Rv64BitsBase RvInt64;
#define RvInt64ShortConst(_n) (RvInt64ShortAssign((_n)))
#define RvInt64Const2(_n)     (RvInt64ShortAssign((_n)))
#define RvInt64Const(_s, _m, _l) (RvInt64Assign((_s), (_m), (_l)))
#define RV_INT64_MAX RvInt64Const(1,0x7FFFFFFF,0xFFFFFFFF)
#define RV_INT64_MIN RvInt64Const(-1,0x80000000,0)

typedef Rv64BitsBase RvUint64;
#define RvUint64ShortConst(_n) (RvUint64Assign(0, (_n)))
#define RvUint64Const2(_n)     (RvUint64Assign(0, (_n)))
#define RvUint64Const(_m, _l) (RvUint64Assign((_m), (_l)))
#define RV_UINT64_MAX RvUint64Const(0xFFFFFFFF,0xFFFFFFFF)
#define RV_UINT64_MIN RvUint64Const(0,0)
#endif

#define RV_UINT64_ZERO RvUint64Const(0, 0)
#define RV_UINT64_ONE  RvUint64Const(0, 1)

#define RvUint64Incr(n) RvUint64Add(n, RV_UINT64_ONE)


/* RvIntPtr, RvUintPtr is defined as integer types capable to accommodate
 *  pointer data type without loosing 
 */


#if (RV_ARCH_BITS == RV_ARCH_BITS_32)
typedef RvInt32 RvIntPtr;
typedef RvUint32 RvUintPtr;
#elif (RV_ARCH_BITS == RV_ARCH_BITS_64)
typedef RvInt64 RvIntPtr;
typedef RvUint64 RvUintPtr;
#endif

/* the macro used to cast the pointer to integer 
 * gets the special meaning in the 64 bits world where such casting
 * truncates the pointer */
#if (RV_ARCH_BITS == RV_ARCH_BITS_64) && (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#define RvPtrToInt32(_n)    PtrToLong((_n))
#define RvPtrToUint32(_n)   PtrToUlong((_n))
#define RvPtrToLong(_n)     PtrToLong((_n))
#define RvPtrToUlong(_n)    PtrToUlong((_n))
 /*
The other way to define those macros:
    #define RvPtrToInt32(_n) ((RvInt32)(RvIntPtr)(_n))
    #define RvPtrToUint32(_n) ((RvUint32)(RvUintPtr)(_n))
    #define RvPtrToInt32(_n) ((RvInt32)(RvIntPtr)(_n))
    #define RvPtrToUint32(_n) ((RvUint32)(RvUintPtr)(_n))
fits not only the windows case but any.
*/
#else
#define RvPtrToInt32(_n)    _n
#define RvPtrToUint32(_n)   _n
#define RvPtrToLong(_n)     _n
#define RvPtrToUlong(_n)    _n
#endif


typedef RvIntPtr RvOffset;

/* Other standard types */
/*@*****************************************************************************
* Type: RvBool (RvCommonBaseTypesPkg)
* -----------------------------------------------------------------------------
* A boolean type. Its possible values are RV_FALSE (zero) and RV_TRUE 
* (one).
****************************************************************************@*/
typedef RvUint RvBool;
#if (!defined(RV_FALSE))
#define RV_FALSE  RvUintConst(0)
#endif
#if (!defined(RV_TRUE))
#define RV_TRUE RvUintConst(1)
#endif

/* Misc Macros. Perhaps these should go elsewhere. */
#define RvMin(_a, _b) (((_a) < (_b)) ? (_a) : (_b))
#define RvMax(_a, _b)   (((_a) > (_b)) ? (_a) : (_b))

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#pragma warning(push)
#pragma warning(disable : 4121)	/* alignment of a member was sensitive to packing */
#endif
/* Alignment macros. Perhaps these should go elsewhere. */
struct RvAlign_s { /* used only for natural data alignment macro */
	/* DO NOT CHANGE THE ORDER OF THE FIELDS */
    char x;
    double y;
};

typedef double RvAlignmentType;

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#pragma warning(pop)
#endif

/* Not pretty, but allows numbytes to be any valid size_t value which is */
/* a power of 2 and works no matter what the relationship between pointer */
/* and data sizes. Pay attention: argument to these macros may be evaluated */
/* multiple times, so no side effects, please */
 
#define RvAlignTo(_ptr, _numbytes) ((void *)((RvInt8 *)(_ptr) + (((RvSize_t)(_numbytes) - ((RvSize_t)(_ptr) & (RvSize_t)((_numbytes) - 1))) & (RvSize_t)((_numbytes) - 1))))

#define RvAlignValueTo(x, p2) ((x) + ( ((p2) - 1) - (((x) - 1) & ((p2) - 1)) ))


/* Natural alignment sizes and macros */
#define RV_ALIGN_DATASIZE (offsetof(struct RvAlign_s, y))
#define RV_ALIGN_PTRSIZE (sizeof(void *))
#define RV_ALIGN_SIZE /*lint -save -e506 */(RvMax(RV_ALIGN_DATASIZE, RV_ALIGN_PTRSIZE)) /*lint -restore*/
#define RvAlignPtr(_x) (RvAlignTo((_x), RV_ALIGN_PTRSIZE))
#define RvAlignData(_x) (RvAlignTo((_x), RV_ALIGN_DATASIZE))
#define RvAlign(_x) (RvAlignTo((_x), RV_ALIGN_SIZE))
#define RvAlignValue(x) RvAlignValueTo(x, RV_ALIGN_SIZE)

/* Fixed width alignment macros */
#define RvAlign16(_x) (RvAlignTo((_x), 2))
#define RvAlign32(_x) (RvAlignTo((_x), 4))
#define RvAlign64(_x) (RvAlignTo((_x), 8))

/* Round up size_t size to a multiple of size_t numbytes (size must be > 0 and <= maxsize - numbytes) */
#define RvRoundToSize(_size, _numbytes) ((RvSize_t)((_size) + (_numbytes) - 1 - ((RvSize_t)((_size) - 1) % (RvSize_t)(_numbytes))))


/*******************/
/* Unused arguments macro */
#define RV_UNUSED_ARG(_arg) (void)(_arg);

/* For more than 1 argument in argList it should be placed between parenthesis, e.g.
 * RV_UNUSED_ARGS((a, b, c, d));
 *
 */
#define RV_UNUSED_ARGS(argList)   ((void)(argList))

/* to get an offset of the field within the structure */
#define RV_OFFSETOF(_type,_field) /*lint -e(413) */ ((char*)&(((_type*)0)->_field)-(char*)0)



/* Some "empty" definitons that we can use for readability of the code */
#define IN
#define OUT
#define INOUT

/************************************************************************
 * RV_DECLARE_HANDLE
 * Definition used to create a general purpose handle. It is declared
 * in this weird way to allow better type checking with C compilers.
 ************************************************************************/
#define RV_DECLARE_HANDLE(_handleName) \
                                      typedef struct { int unused; } _handleName##__ ; \
                                      typedef const _handleName##__ * _handleName; \
                                      typedef _handleName*  LP##_handleName


/* Macro used to calculate the pointer position of a struct, given one of its
   internal fields */
#define RV_GET_STRUCT(_structTypedef, _fieldName, _fieldPtr) \
    ((_structTypedef *)( ((char *)(_fieldPtr)) - offsetof(_structTypedef, _fieldName) ))


/*****************************************/
/* Macro used for useful pragma messages */
#define Value2String(m)     #m
#define Macro2String(m)     Value2String(m)

#define RvReminder          __FILE__ "(" Macro2String(__LINE__) ") : Reminder: "
#define RvWarning           __FILE__ "(" Macro2String(__LINE__) ") : warning XXXX: "
#define RvError             __FILE__ "(" Macro2String(__LINE__) ") : error XXXX: "

/* Usage:
#pragma message(RvReminder "Remove these definitions before release")
*/

#define RvFMT(fmt) Value2String(fmt)

#define FMT_TSTAMP <%d:%9.9d>

#define FMT_TSTAMP_VAL(x) RvTimestampGetSecs(x), RvTimestampGetNsecs(x)

#include "rv64bits.h"

#if defined(__cplusplus)
}
#endif

#endif /* RV_TYPES_H */
