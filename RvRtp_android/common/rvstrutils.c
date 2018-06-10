/***********************************************************************
Filename   : rvstrutils.c
Description: string manipulation utilities
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
#include "rvstrutils.h"

#ifdef NEED_STRCASECMP

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static const RvUint8 charmap[] = {
    0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007,
    0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
    0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027,
    0030, 0031, 0032, 0033, 0034, 0035, 0036, 0037,
    0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
    0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
    0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
    0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
    0100, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
    0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
    0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167,
    0170, 0171, 0172, 0133, 0134, 0135, 0136, 0137,
    0140, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
    0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
    0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167,
    0170, 0171, 0172, 0173, 0174, 0175, 0176, 0177,
    0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
    0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
    0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
    0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
    0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
    0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
    0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
    0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
    0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
    0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
    0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
    0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
    0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
    0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
    0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
    0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377
};


/********************************************************************************************
 * strcasecmp
 * Performs case insensitive comparison of two null terminated strings
 * INPUT   : s1
 *           s2 - strings to compare
 * OUTPUT  : None
 * RETURN  : 0 if strings are the same or lexical difference in the
 *           first differing character.
 */
RVCOREAPI 
int RVCALLCONV strcasecmp(
    IN const char *s1,
    IN const char *s2)
{
    const RvUint8 *cm = charmap;
    const RvUint8 *us1 = (const RvUint8 *)s1;
    const RvUint8 *us2 = (const RvUint8 *)s2;
    
    while (cm[*us1] == cm[*us2++]) {
        if (*us1++ == '\0')
            return (0);
    }
    return (cm[*us1] - cm[*--us2]);
}

/********************************************************************************************
 * strncasecmp
 * Performs case insensitive comparison of first 'n' bytes of two strings
 * INPUT   : s1
 *           s2 - strings to compare
 *           n  - number of bytes to compare
 * OUTPUT  : None
 * RETURN  : 0 if strings are the same or lexical difference in the
 *           first differing character.
 */
RVCOREAPI 
int RVCALLCONV strncasecmp(
    IN const char *s1,
    IN const char *s2,
    IN size_t n)
{
    const RvUint8 *cm = charmap;
    const RvUint8 *us1 = (const RvUint8 *)s1;
    const RvUint8 *us2 = (const RvUint8 *)s2;
    
    if (n != 0)
    {
        do {
            if (cm[*us1] != cm[*us2++])
                return (cm[*us1] - cm[*--us2]);
            if (*us1++ == '\0')
                break;
        } while (--n != 0);
    }
    return (0);
}

#else
int prevent_warning_of_ranlib_has_no_symbols_rvstrutils=0;
#endif /* #ifdef NEED_STRCASECMP */

