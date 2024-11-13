//cortexm0 doesn't have much worth optimizing via asm

//declares an asm routine that can be accessed via C without much pain. C prototype will need 'extern "C"' treatment.
.macro Cfunction fname
  .global \fname
  .thumb
  .thumb_func

\fname :
  .endm

  .align 4   //align nanoSpin so that flash cache works predictably
  //r0 is nano seconds, r1 is number of nanoseconds per iteration of this function. That was 83ns for a 72MHz M3.
  //caller must pre-tweak value for function call overhead.
Cfunction nanoSpin
  sub r0,r1
  bpl nanoSpin
  bx lr
.end
