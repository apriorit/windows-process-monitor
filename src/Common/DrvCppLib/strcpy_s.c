/***
*strcpy_s.c - contains strcpy_s()
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*   strcpy_s() copies one string onto another.
*
*******************************************************************************/

#include <string.h>


#define EINVAL          22
#define ERANGE          34
#define EILSEQ          42
#define STRUNCATE       80


int __cdecl strcpy_s(char *_Dst, 
                     size_t _SizeInBytes, 
                     const char *_Src)
{
    char * p;
    size_t available;

    /* validation section */

    p = _Dst;
    available = _SizeInBytes;
    while ((*p++ = *_Src++) != 0 && --available > 0)
    {
    }

    if (available == 0)
    {
        *(_Dst) = 0; 
        return EINVAL;
    }
    return 0;
}


