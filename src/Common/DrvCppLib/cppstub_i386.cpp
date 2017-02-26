#ifndef _M_AMD64

void __stdcall _JumpToContinuation(void *,struct EHRegistrationNode *);
struct EHRegistrationNode;

extern "C" void CPPLIB_ThreadRelease();

_declspec(naked)
void __stdcall _JumpToCONTINUATION(void *,struct EHRegistrationNode *)
{
    __asm call CPPLIB_ThreadRelease
    __asm jmp _JumpToContinuation;
}


#endif