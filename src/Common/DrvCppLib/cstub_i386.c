#include "ntifs.h"

#ifndef _M_AMD64

void _CxxThrowEXCEPTION(int, int);
void CPPLIB_ThreadAddRef();

#pragma warning(disable:4276)

__declspec(naked)
VOID
NTAPI
_CxxThrowException (
    IN PVOID Object,
    IN PVOID CxxExceptionType
    )
{
    // EAX should be zero in case of " throw; "
    __asm mov  eax, [esp+0x8]
    __asm test eax, eax
    __asm jz $+0xb
    __asm call CPPLIB_ThreadAddRef
    __asm jmp _CxxThrowEXCEPTION;
}


#endif