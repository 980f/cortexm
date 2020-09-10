#pragma once

/**
 * floating point coprocessor init and config operations.
 * */

/** faster execution by simpler Nan and denorm handling*/
void fpu_fast();
/** more ieee754 correct */
void fpu_correct();
/** actually turn on the coprocessor*/
void fpu_enable();
/** if  you will never use floating point in an isr then you can disable stacking its registers */
void fpu_noisr() ;
/** traditional combination of the config functions, will need to add option bits to make it live up to its name */
void fpu_init();
