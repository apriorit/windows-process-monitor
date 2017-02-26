; How to compile:
; ml64.exe /Fostub_amd64 /c /Zi /Cx /Cp stub_amd64.asm
;

extrn CPPLIB_ThreadRelease: PROC
extrn CPPLIB_ThreadAddRef: PROC


.data

.code

jumpToCatchCode PROC public
     add     rsp, 8

     pop     r15
     pop     r14
     pop     r13
     pop     r12
     pop     rdi
     pop     rbx

     push rbp
     mov rbp, rsp

     push rdi
     push rax

     push rbx
     push RBP
     push RDI
     push RSI
     push R12
     push R13
     push R14
     push R15

     push rdx
     push rcx

     sub rsp, 20h
     call CPPLIB_ThreadRelease
     add rsp, 20h

     pop rcx
     pop rdx

     pop R15
     pop R14
     pop R13
     pop R12

     pop RSI
     pop RDI
     pop RBP
     pop rbx

     pop rax
     pop rdi

     mov rsp, rbp
     pop rbp

     ret
jumpToCatchCode ENDP

CPPLIB_ThreadAddRef_stub PROC
     push rbp
     mov rbp, rsp

     push rdi
     push rax

     push rbx
     push RBP
     push RDI
     push RSI
     push R12
     push R13
     push R14
     push R15

     push rdx
     push rcx

     sub rsp, 20h
     call CPPLIB_ThreadAddRef
     add rsp, 20h

     pop rcx
     pop rdx

     pop R15
     pop R14
     pop R13
     pop R12

     pop RSI
     pop RDI
     pop RBP
     pop rbx

     pop rax
     pop rdi

     mov rsp, rbp
     pop rbp
     ret
CPPLIB_ThreadAddRef_stub ENDP

jumpToCatchCode_stub PROC PUBLIC
     test rcx, rcx
     jz no_call
     call jumpToCatchCode
 no_call:
     ret
jumpToCatchCode_stub ENDP


; (KIRQL  * OldIrql)
CPPLIB_DisableKernelDefence PROC PUBLIC
    cli
    mov     rax, cr0
    push    rax
    and     rax, 0FFFFFFFFFFFEFFFFh
    mov     cr0, rax
    pop     rax
    ret
CPPLIB_DisableKernelDefence ENDP

; (ULONG OldCr0, KIRQL OldIrql)
CPPLIB_EnableKernelDefence PROC PUBLIC
     mov  rax, rcx
     mov  cr0, rax
     sti
     ret
CPPLIB_EnableKernelDefence ENDP


CPPLIB_Throw PROC PUBLIC
     mov     qword ptr [rsp+18h],rbx
     test     rcx, rcx
     jz continue_exception

     call CPPLIB_ThreadAddRef_stub
continue_exception:
     ret


CPPLIB_Throw ENDP

END
