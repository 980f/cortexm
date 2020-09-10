//dregs, good pieces moved to cortexm/cortexm3.s

//declares an asm routine that can be accessed via C without much pain. C prototype will need 'extern "C"' treatment.
.macro Cfunction fname
  .global \fname
  .thumb
  .thumb_func

\fname :
  .endm

  



  //copyObject(u32 source,u32 target,u32 length)
  Cfunction copyObject
  cmp r0, r1
  beq 2f
  cmp r2,#0
  beq 2f
   //todo:3 if more than say 8 bytes then recognize aligment and use larger moves
1:
  ldrb r3, [r0]
  add r0, r0, #1
  strb r3, [r1]
  add r1, r1, #1
  sub r2, r2, #1
  bne 1b
2:
  bx lr

//C startup uses start to end rather than start and length:
  Cfunction memory_copy
  sub r2, r2, r1  //end(+1)-start=length
  bne copyObject
  bx lr
  
  
  Cfunction memory_set
  cmp r0, r1
  beq 1f
  strb r2, [r0]
  add r0, r0, #1
  b memory_set
1:
  bx lr

  Cfunction  fillObject  //(void* target,u32 length,u8 fill);
  strb r2, [r0]
  add r0, r0, #1
  sub r1, r1, #1
  bne fillObject
  bx lr

