/* rvstdio.c - input/output receiving/printing functions */
/************************************************************************
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

#include "rvstdio.h"

 /* Lets make error codes a little easier to type */
#define RvStdioErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_STDIO, (_e))


/********************************************************************************************
 * RvStdioInit - Initializes the stdio module.
 *
 * Must be called once (and only once) before any other functions in the module are called.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : Always RV_OK
 */
RvStatus RvStdioInit(void)
{
    RvStatus ret = RV_OK;

    return ret;
}


/********************************************************************************************
 * RvStdioEnd - De-initializes the stdio module.
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : Always RV_OK
 */
RvStatus RvStdioEnd(void)
{
    RvStatus ret = RV_OK;

    return ret;
}


#if (RV_STDIO_TYPE == RV_STDIO_WINCE_DEBUG)

#define MULTIBYTE_MAX_LENGTH 1024

/********************************************************************************************
 * RvOutputDebugPrintf - De-initializes the stdio module.
 *
 * INPUT   : format     - printf style format
 *           ...        - printf style parameters that match format
 * OUTPUT  : none
 * RETURN  : lenght of string that was printed
 */
RVCOREAPI
RvInt RVCALLCONV RvOutputDebugPrintf(
    IN const RvChar     *format,
    IN                  ...)
{
    RvStatus ret = RV_OK;
    va_list arg;
    static RvChar str[MULTIBYTE_MAX_LENGTH];
    static WCHAR strwc[MULTIBYTE_MAX_LENGTH];
    RvInt stringlen;

    va_start(arg, format);
    stringlen = vsprintf(str, format, arg);
    va_end(arg);

    /* We have to convert the (multibyte) string to wide-character-string */
    MultiByteToWideChar(CP_ACP, 0, str, stringlen, strwc, stringlen);
    strwc[stringlen] = (WCHAR)0;

    OutputDebugString(strwc);

    return ret;
}

#endif

/* Standard char operations for old WinCE versions */
#if (RV_OS_TYPE == RV_OS_TYPE_WINCE) && (RV_OS_VERSION == RV_OS_WINCE_2_11)

void CharToWChar(RvChar CharIn, WCHAR * wCharOut)
{
    MultiByteToWideChar(CP_ACP, 0, &CharIn, 1, wCharOut, 1);
}

/********************************************************************************************
 * isupper - checks if input character is an uper case letter
 *
 * INPUT   : CharIn     - character to be checked
 * OUTPUT  : none
 * RETURN  : returns a non-zero value if CharIn is an uppercase character
 */
RVCOREAPI
RvInt RVCALLCONV __cdecl isupper(
    IN RvInt CharIn)
{
    WCHAR wChar;
    RvChar  c;

    c = (RvChar)CharIn;
    CharToWChar(c, &wChar);
    return iswupper(wChar);
}

/********************************************************************************************
 * islower - checks if input character is an lower case letter
 *
 * INPUT   : CharIn     - character to be checked
 * OUTPUT  : none
 * RETURN  : returns a non-zero value if CharInc is a lowercase character
 */
RVCOREAPI
RvInt RVCALLCONV __cdecl islower(
    IN RvInt CharIn)
{
    WCHAR wChar;
    RvChar  c;

    c = (RvChar)CharIn;
    CharToWChar(c, &wChar);
    return iswlower(wChar);
}

/********************************************************************************************
 * isspace - checks if input character is a white-space character
 *
 * INPUT   : CharIn     - character to be checked
 * OUTPUT  : none
 * RETURN  : returns a non-zero value if CharIn is a white-space character
 */
RVCOREAPI
RvInt RVCALLCONV __cdecl isspace(
    IN RvInt CharIn)
{
    WCHAR wChar;
    RvChar  c;

    c = (RvChar)CharIn;
    CharToWChar(c, &wChar);
    return iswspace(wChar);
}

/********************************************************************************************
 * isdigit - checks if input character is a digit character
 *
 * INPUT   : CharIn     - character to be checked
 * OUTPUT  : none
 * RETURN  : returns a non-zero value if CharIn is a digit character
 */
RVCOREAPI
RvInt RVCALLCONV __cdecl isdigit(
    IN RvInt CharIn)
{
    WCHAR wChar;
    RvChar  c;

    c = (RvChar)CharIn;
    CharToWChar(c, &wChar);
    return iswdigit(wChar);
}

/********************************************************************************************
 * isalnum - checks if input character is a alphanumeric character
 *
 * INPUT   : CharIn     - character to be checked
 * OUTPUT  : none
 * RETURN  : returns a non-zero value if CharIn is a alphanumeric character
 */
RVCOREAPI
RvInt RVCALLCONV __cdecl isalnum(
    IN RvInt CharIn)
{
    WCHAR wChar;
    RvChar  c;

    c = (RvChar)CharIn;
    CharToWChar(c, &wChar);
    return iswalnum(wChar);
}

/********************************************************************************************
 * strspn - Find the first substring
 *
 * INPUT   : s1     - Null-terminated string to search
 *           s2     - Null-terminated character set
 * OUTPUT  : none
 * RETURN  : an integer value specifying the length of the substring in s1
 */
RVCOREAPI
RvSize_t RVCALLCONV __cdecl strspn(
    IN const RvChar * s1,
    IN const RvChar * s2)
{
    RvChar * srchs2;
    RvInt   len;

    for (len = 0; *s1; s1++, len++)
    {
        for (srchs2 = (RvChar *)s2; *srchs2; srchs2++)
            if (*s1 == *srchs2)
                break;
            if (*srchs2 == 0)
                break;
    }
    return len;
}


#endif


#if (RV_STDIO_TYPE == RV_STDIO_STUB)
/**********************************************************************
 * ANSI stdio implementations
 **********************************************************************/

static RvInt RvVfscanf(RvFILE* stream, const RvChar* format, va_list arg);


/********************************************************************************************
 * RvFprintf - This function implements an ANSI fprintf style function.
 *
 * INPUT   : stream - The stream to which to write
 *           format - A fprintf style format string.
 *           ...    - A variable argument list.
 * OUTPUT  : none
 * RETURN  : The number of characters written, or negative if an error occured.
 */
RVCOREAPI RvInt RVCALLCONV RvFprintf(
    IN RvFILE           * stream,
    IN const RvChar     *format,
    IN                  ...)
{
    RvInt ret;
    va_list arg;

    va_start(arg, format);
    ret = RvVfprintf(stream, format, arg);
    va_end(arg);

    return ret;
}


/********************************************************************************************
 * RvPrintf - This function implements an ANSI printf style function.
 *
 * INPUT   : format - A fprintf style format string.
 *           ...    - A variable argument list.
 * OUTPUT  : none
 * RETURN  : The number of characters written, or negative if an error occured.
 */
RVCOREAPI
RvInt RVCALLCONV RvPrintf(
    IN const RvChar *format,
    INT             ...)
{
    RvInt ret;
    va_list arg;

    va_start(arg, format);
    ret = RvVfprintf(RvStdout, format, arg);
    va_end(arg);

    return ret;
}


/********************************************************************************************
 * RvVprintf - This function implements an ANSI vprintf style function.
 *
 * INPUT   : format - A fprintf style format string.
 *           ...    - A variable argument list.
 * OUTPUT  : none
 * RETURN  : The number of characters written, or negative if an error occured.
 */
RVCOREAPI RvInt RVCALLCONV RvVprintf(
    IN const RvChar *format,
    IN va_list      arg)
{
    return RvVfprintf(RvStdout, format, arg);
}

/********************************************************************************************
 * RvFscanf - This function implements an ANSI fscanf style function.
 *
 * INPUT   : stream - The stream from which to read input.
 *           format - A sscanf style format string.
 *           ...    - A variable argument list.
 * OUTPUT  : none
 * RETURN  : The number input items converted, or RvEOF if an error occured.
 */
RvInt RvFscanf(
    IN RvFILE       *stream,
    IN const RvChar *format,
    INT             ...)
{
    RvInt ret;
    va_list arg;

    va_start(arg, format);
    ret = RvVfscanf(stream, format, arg);
    va_end(arg);

    return ret;
}


/********************************************************************************************
 * RvScanf - This function implements an ANSI scanf style function.
 *
 * INPUT   : format - A sscanf style format string.
 *           ...    - A variable argument list.
 * OUTPUT  : none
 * RETURN  : The number input items converted, or RvEOF if an error occured.
 */
RvInt RvScanf(
    IN const RvChar *format,
    INT             ...)
{
    RvInt ret;
    va_list arg;

    va_start(arg, format);
    ret = RvVfscanf(RvStdout, format, arg);
    va_end(arg);

    return ret;
}


/********************************************************************************************
 * RvGetchar - Character Input Function
 *
 * INPUT   : none
 * OUTPUT  : none
 * RETURN  : retirved character
 */
RvInt RvGetchar(void)
{
    return RvGetc(RvStdin);
}

/********************************************************************************************
 * RvPutchar - Character Output Function
 *
 * INPUT   : c  - character to write
 * OUTPUT  : none
 * RETURN  : written character
 */
RvInt RvPutchar(
    IN RvInt c)
{
    return RvPutc(c,RvStdout);
}

/**********************************************************************
 * ANSI stdio stubs that do nothing
 **********************************************************************/

/********************************************************************************************
 * RvFopen - This function implements an ANSI fopen style function.
 *
 * INPUT   : filename   - The name of the file to open
 *           mode       - The mode in which to open the file.
 * OUTPUT  : none
 * RETURN  : A handle to the opened file stream, or NULL if an error occured.
 */
RvFILE* RvFopen(
    IN const RvChar* filename,
    IN const RvChar* mode)
{
    return NULL;
}

/********************************************************************************************
 * RvFreopen - This function implements an ANSI freopen style function.
 *
 * INPUT   : filename   - The name of the file to open
 *           mode       - The mode in which to open the file.
 *           stream
 * OUTPUT  : none
 * RETURN  : A handle to the opened file stream, or NULL if an error occured.
 */
RvFILE* RvFreopen(
    IN  const RvChar    *filename,
    IN  const RvChar    *mode,
    IN  RvFILE          *stream)
{
    return NULL;
}

/********************************************************************************************
 * RvFlush - This function implements an ANSI fflush style function.
 *
 * INPUT   : stream
 * OUTPUT  : none
 * RETURN  : returns 0 if the buffer was successfully flushed.
 */
RvInt RvFlush(RvFILE* stream)
{
    return RvEOF;
}


/********************************************************************************************
 * RvFclose - This function implements an ANSI fclose style function.
 *
 * INPUT   : stream - The handle to the file stream to close
 * OUTPUT  : none
 * RETURN  : Zero if successful, or RvEOF if an error occured.
 */
RvInt RvFclose(
    IN RvFILE* stream)
{
    return RvEOF;
}

RvInt RvRemove(const RvChar* filename)
{
    return -1;
}

RvInt RvRename(const RvChar* oldname, const RvChar* newname)
{
    return -1;
}

RvFILE* RvTmpfile(void)
{
    return NULL;
}

RvChar* RvTmpnam(RvChar s[RvL_tmpnam])
{
    return NULL;
}

RvInt RvSetvbuf(RvFILE stream, RvChar* buf, RvInt mode, RvSize_t size)
{
    return -1;
}

void RvSetbuf(RvFILE stream, RvChar* buf)
{
}


/********************************************************************************************
 * RvVfprintf - This function implements an ANSI vfprintf style function.
 *
 * INPUT   : stream - The stream to which to write
 *           format - A vfprintf style format string.
 *           arg    - A variable argument list.
 * OUTPUT  : none
 * RETURN  : The number of characters written, or negative if an error occured.
 */
RVCOREAPI RvInt RVCALLCONV RvVfprintf(
    IN RvFILE       *stream,
    IN const RvChar *format,
    IN va_list      arg)
{
    return -1;
}

/* Non-Ansi Formatted Input */
static RvInt RvVfscanf(RvFILE* stream, const RvChar* format, va_list arg)
{
    return -1;
}


/* Character Input ans Output Functions */
RvInt RvFgetc(RvFILE* stream)
{
    return RvEOF;
}

RvChar* RvFgets(RvChar* s, RvInt n, RvFILE* stream)
{
    return NULL;
}

RvInt RvFputc(RvInt c, RvFILE* stream)
{
    return RvEOF;
}

RvInt RvFputs(const RvChar* s, RvFILE* stream)
{
    return RvEOF;
}

RvInt RvGetc(RvFILE* stream)
{
    return RvEOF;
}

RvChar* RvGets(RvChar* s)
{
    return NULL;
}

RvInt RvPutc(RvInt c, RvFILE* stream)
{
    return RvEOF;
}

RvInt RvPuts(const RvChar* s, RvFILE* stream)
{
    return RvEOF;
}

RvInt RvUngetc(RvInt c, RvFILE* stream)
{
    return RvEOF;
}

/* Direct Input and Output Functions */
RvSize_t RvFread(void* ptr, RvSize_t size, RvSize_t nobj, RvFILE* stream)
{
    return 0;
}

RvSize_t RvFwrite(const void* ptr, RvSize_t size, RvSize_t nobj, RvFILE* stream)
{
    return 0;
}

/* File Positioning Functions */
RvInt RvFseek(RvFILE* stream, long offset, RvInt origin)
{
    return -1;
}

long RvFtell(RvFILE* stream)
{
    return -1L;
}

void RvRewind(RvFILE* stream)
{
}

RvInt RvFgetpos(RvFILE* stream, RvFpos_t* ptr)
{
    return -1;
}

RvInt RvFsetpos(RvFILE* stream, const RvFpos_t* ptr)
{
    return -1;
}

/* Error Functions */
void RvClearerror(RvFILE* stream)
{
}

RvInt RvFeof(RvFILE* stream)
{
    return -1;
}

RvInt RvFerror(RvFILE* stream)
{
    return -1;
}

void RvPerror(const RvChar* s)
{
}

#endif  /* (RV_STDIO_TYPE == RV_STDIO_STUB) */

#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS) || (RV_OS_TYPE == RV_OS_TYPE_PSOS)
RvInt RvFseek(RvFILE *stream, long offset, RvInt origin)
{
	RV_UNUSED_ARG(stream);
	RV_UNUSED_ARG(offset);
	RV_UNUSED_ARG(origin);
	return -1;
}
#endif

/********************************************************************************************
 * RvBsearch - Binary search function
 *
 * INPUT   : key
 *           base
 *           numOfElements
 *           elementSize
 *           compareFunc
 * OUTPUT  : none
 * RETURN  : pointer to found element or NULL
 */
RVCOREAPI void* RVCALLCONV RvBsearch(
    IN const void*     key,
    IN const void*     base,
    IN RvSize_t        numOfElements,
    IN RvSize_t        elementSize,
    IN RvInt (*compareFunc)(const void* key, const void* ))
{
    RvUint8 *lo = (RvUint8 *)base;
    RvUint8 *hi = (RvUint8 *)base + (numOfElements - 1) * elementSize;
    RvUint8 *mid;
    RvSize_t half;
    int result;

    while (lo <= hi)
    {
        half = numOfElements / 2;
        if (half)
        {
            mid = lo + (numOfElements & 1 ? half : (half - 1)) * elementSize;
            result = (*compareFunc)(key,mid);
            if (!result)
                return(mid);
            else if (result < 0)
            {
                hi = mid - elementSize;
                numOfElements = numOfElements & 1 ? half : half-1;
            }
            else
            {
                lo = mid + elementSize;
                numOfElements = half;
            }
        }
        else if (numOfElements)
            return((*compareFunc)(key,lo) ? NULL : lo);
        else
            break;
    }

    return NULL;
}

