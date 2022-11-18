extern IrqHandler
extern ExcHandler

global kIntelIsrTable

%macro PushAll 0
  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8
  push rbp
  push rsi
  push rdi
  push rdx
  push rcx
  push rbx
  push rax
%endmacro

%macro PopAll 0
  pop rax
  pop rbx
  pop rcx
  pop rdx
  pop rdi
  pop rsi
  pop rbp
  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15
%endmacro

%macro MaybeSwapGS 1
  cmp word [rsp + %1], 0x28
  je %%skip
  swapgs
  %%skip:
%endmacro

%macro GenerateInterruptStub 1
  %%start:
  %if %1 < 32
    %if (1 << %1) & 0x27D00 == 0
      push qword 0
    %endif
    push qword %1
    jmp ExceptionHandler
  %else
    push qword %1
    jmp InterruptHandler
  %endif
  times 32 - ($ - %%start) db 0
%endmacro

kIntelIsrTable:
  %assign i 0
  %rep 256
    GenerateInterruptStub i
  %assign i i +  1
  %endrep

InterruptHandler:
  MaybeSwapGS 16
  PushAll

  mov rdi, [rsp + 0x78]

  cld
  call IrqHandler

  PopAll
  MaybeSwapGS 16

  add rsp, 8
  iretq

ExceptionHandler:
  MaybeSwapGS 24
  PushAll

  mov rdi, [rsp + 0x78]
  mov dword rsi, [rsp + 0x80]
  mov rdx, rsp
  add rdx, 0x88
  mov rcx, rsp

  cld
  call ExcHandler

  PopAll
  MaybeSwapGS 24

  add rsp, 16
  iretq
