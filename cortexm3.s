
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

Cfunction log2Exponent
  clz r0,r0    //count leading zeroes
  rsb r0,#31   //we wanted position of leading '1' = 31 - number of leading zeroes
  bx lr


Cfunction  flog2
  //todo:2 push temps
  mov r1,r0  //r1 is unshifted mantissa
  bl log2Exponent //ro is integer part
  lsr r1,r0  //r1 is ms aligned mantissa
  //todo:2 much more code!
  //cmp r1  ; add #1
  //todo:2 pop temps
  bx lr


Cfunction splitteru2
  //r0 is address of 64 bit number
  //we are only going to accept 32 bit returns, could write another version for 65 bits.
 
  //push r0
  //ldm r0,{r2,r3} //r2 is lower bits, r3 is higher
  ////todo: clear the sign bit before the shift
  //lsr r1,r3,#20  //r1 is sign and exponent, we are presuming the sign bit is 0 for now
  
  //subs r1,#1075   // -1023 for the bias, -52 as that is where integers begin, a negative value means we have to remove some mantissa bits from the integer we are producing.
  
  bx lr

//this routine will fail if the product of first two operands exceeds 2^32.
Cfunction muldivide
  umull r0,r1,r0,r1  //in typical use this is u16 stretched to u32 times the same
  lsr r1,r2,#1       //add 1/2 denom, only unsigned denoms are supported
  add r0,r0,r1       //to get a rounded quotient.
  udiv r0,r0,r2      //and now divide by another such stretched u16
  bx lr


//u16 fractionallyScale(u16 number,u16 fraction,u16 numbits)
Cfunction fractionallyScale
  umull r0,r1,r0,r1  //in typical use this is u16 stretched to u32 times the same
  //shift ms part right by r2 and return that.
  lsr r0,r2
  bx lr


CFunction shiftScale //float (float eff,int pow2){
  lsl r1,r1,#23
  sub r0,r0,r1
  bx lr

//include core_atomic.h to use the following atomic access routines
// in the atomic_* routines we can't use the ITF instruction mechanism (?why not?), we have to do old fashion branches.<<M4 manual suggests that we should!
//"If a store-exclusive instruction performs the store, it writes 0 to its destination register.
// If it does not perform the store, it writes 1 to its destination register.
// If the store-exclusive instruction writes 0 to the destination register, it is guaranteed that no
// other process in the system has accessed the memory location between the load-exclusive
// and store-exclusive instructions."

//assign r1 and return 'failed' in r0
.macro Strexit1
  strex r2,r1,[r0]
  mov r0,r2
  bx lr
.endm


//assign r2 and return 'failed' in r0
.macro Strexit2
  strex r1,r2,[r0]
  mov r0,r1
  bx lr
.endm



//r0 address, trusted to be 32-bit aligned. r1 is scratched.
Cfunction atomic_increment
  ldrex r1,[r0]
  add r1, r1, #1
  Strexit1

//r0 address, trusted to be 32-bit aligned. r1 scratched.
Cfunction atomic_decrement
  ldrex r1,[r0]
  sub r1, r1, #1
  Strexit1

//r0 address, trusted to be 32-bit aligned. r1 scratched.
Cfunction atomic_decrementNotZero
  ldrex r1,[r0]
  cbz r1, 1f
  sub r1, r1, #1
  Strexit1
1: //if 0 we still need to remove our lock, and return false
  clrex
  mov r0,r1 //r1 is conveniently the value we need to return==0
  bx lr

//returns 1 if *arg!=0 decrement it, return true if arg is zero.
//r0 address, trusted to be 32-bit aligned. r1 and r2 scratched.
Cfunction atomic_decrementNowZero
  ldrex r1,[r0]
  cbz r1, 1f
  sub r1, r1, #1
  strex r2,r1,[r0]
  cmp r2,#1
  beq atomic_decrementNowZero
  //if r1 is zero return 1 else 0
  cbz r1, 1f
  mov r0,#0
  bx lr
1: //was 0, we still need to remove our lock, and return
  clrex
  add r0,r1,#1 //r1 is zero
  bx lr

//r0 address, trusted to be 32-bit aligned. r1 scratched.
Cfunction atomic_incrementNotMax
  ldrex r1,[r0]
  add r1, r1,  #1
  cbz r1, 1f
  Strexit1
1: //if 0 we still need to remove our lock, and return false
  clrex
  mov r0,#0
  bx lr


//r0 address, trusted to be 32-bit aligned. r1,r2 scratched.
Cfunction atomic_incrementWasZero
  ldrex r1,[r0]
  add r1, r1, #1
  cbz r1, 1f
  strex r2,r1,[r0]
  cmp r2,#1
  beq atomic_incrementWasZero
  cmp r1,#1
  bne 1f
  mov r0,#1
  bx lr
1: //if 0 we still need to remove our lock, and return true
  clrex
  mov r0,#0
  bx lr


//r0 address, trusted to be 32-bit aligned. r1 new value, r2 scratched.
Cfunction  atomic_setIfZero
  //lock and load
  ldrex r2,[r0]
  //if not zero bail out.
  cbnz r2,1f
  //is zero, write over it.
  Strexit1
1: //we still need to remove our lock, and return 'success'
  clrex
  mov r0,#1
  bx lr

  //r0 address, r1 increment, r2 scratched
Cfunction atomic_add
  ldrex r2,[r0]
  add r2, r2, r1
  Strexit2

.end
