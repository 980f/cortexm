/*all about interrupts*/
#include "nvic.h"
#include "eztypes.h"
#include "peripheraltypes.h"
#include "cruntime.h"

volatile unsigned CriticalSection::nesting = 0;
/////////////////////////////////

u8 setInterruptPriorityFor(int number, u8 newvalue) { // one byte each, often only some high bits are implemented
  //faults are at ED14 + 4..15, 0..3 are not allowed to be reprioritized, some otehrs are also ignored
  u8 &priorityRegister(*reinterpret_cast<u8 *>(number < 0 ? 0xE000ED14 - number : (0xE000E400 + number)));
  u8 oldvalue = priorityRegister;
  priorityRegister = newvalue << 4;
  return oldvalue >> 4;
}


/////////////////////////////////

struct InterruptController {
  // ICSR
  volatile unsigned int active: 9; // isr we are within
  unsigned int : 2;
  volatile unsigned int unnested: 1; // status: will return to "not-interrupted" if isr ends now.
  volatile unsigned int pending: 9; // highest priority one pending, if>active we are about to nest :)
  volatile unsigned int isrpending: 1; // non-NMI, non-Fault is pending
  volatile unsigned int isrpreempt: 1; // about to be nested, only useful for debug.
  unsigned int : 1;
  unsigned int pendTickClr: 1;
  unsigned int pendTickSet: 1;
  unsigned int pendSVClr: 1;
  unsigned int pendSVSet: 1;
  unsigned int : 2;
  unsigned int pendNMISet: 1;
  unsigned int : 1;

  u32 VectorTableBase; // bits 6..0 better be set to 0!

  struct AIRC {
    unsigned int hardReset: 1; // generate a hardware reset, maybe
    unsigned int eraseIrqState: 1; // abend interrupt handling activity, but doesn't fixup the stack
    unsigned int pleaseReset: 1; // will generate a hardware reset
    unsigned int priorityGrouper: 3; // indirectly the number of interrupt levels
    unsigned int endianNess: 1; // read only
    u16 VectorKey; // write 05FA to write to any of the above fields.
  };

  u32 airc; // must build an object then copy as a u32 to this field.

  unsigned int : 1;
  unsigned int sleepOnExit: 1; //
  unsigned int sleepDeep: 1;
  unsigned int : 1;
  unsigned int sevOnPend: 1; // let interrupts be events for WFE
  unsigned int : 32 - 5;

  // u32 CCR;
  unsigned int allowThreadOnReturn: 1; // only an RTOS would want this available
  unsigned int allowSoftInterruptTriggers: 1; // allow user code to simulate interrupts
  unsigned int : 1;
  unsigned int trapUnaligned: 1;
  unsigned int trapDivBy0: 1; // a heinous thing to do
  unsigned int : 3;
  unsigned int ignoreMisalignmentInFaultHandlers: 1;
  unsigned int : 1; // stkalign, let hardware manage this
  unsigned int : 32 - 10;
  /* priorities for faults. */
  u8 priority[12]; // syscall settable prorities, 4 .. 15, -12 to -1 in our unified numbering
  // volatile u32 SHCSR;
  volatile unsigned int memFault: 1;
  volatile unsigned int busFault: 1;
  unsigned int : 1;
  volatile unsigned int usageFault: 1;
  unsigned int : 3;
  volatile unsigned int serviceCall: 1;
  volatile unsigned int monitorActive: 1;
  unsigned int : 1;
  volatile unsigned int serviceFlagActive: 1;
  volatile unsigned int sysTicked: 1;
  volatile unsigned int usageFaultPending: 1;
  volatile unsigned int memFaultPending: 1;
  volatile unsigned int busFaultPending: 1;
  volatile unsigned int serviceCallPending: 1;

  unsigned int memoryFaultEnable: 1;
  unsigned int busFaultEnable: 1;
  unsigned int usageFaultEnable: 1;
  unsigned int : 32 - 19;

  u8 memFaultInfo;
  u8 busFaultInfo;
  u16 usageFaultInfo;

  u32 hardFaultStatus;
  u32 debugFaultStatus;
  u32 mmFaultAddress;
  u32 busFaultAddress;
  volatile u32 AFSR;
  // cpuid table
  // coprocessor
};

// 0xE000EF00: write an interrupt id number to get it to pend.
// nvic[INT_ENA]=1
// basepri precludes higher numbered interrupts from occuring.
// primask precludes any normal interrupt
// faultmask precludes most faults.
/*
 * void __set_PRIMASK (uint32_t value)  M0, M3  PRIMASK = value  Assign value to Priority Mask Register (using the instruction MSR)
 * uint32_t __get_PRIMASK (void)  M0, M3  return PRIMASK  Return Priority Mask Register (using the instruction MRS)
 * void __enable_fault_irq (void)  M3  FAULTMASK = 0  Global Fault exception and Interrupt enable (using the instruction CPSIE f)
 * void __disable_fault_irq (void)  M3  FAULTMASK = 1  Global Fault exception and Interrupt disable (using the instruction CPSID f)
 */
soliton(InterruptController, 0xE000ED04);


//#if 0 //hal version
//uint32_t reg_value;
//  uint32_t PriorityGroupTmp = (PriorityGroup & (uint32_t)0x07UL);
//
//  reg_value = ((SCB_Type *) ((0xE000E000UL) + 0x0D00UL) )->AIRCR;
//  reg_value &= ~((uint32_t)((0xFFFFUL << 16U) | (7UL << 8U)));
//  reg_value = (reg_value |
//                ((uint32_t)0x5FAUL << 16U) |
//                (PriorityGroupTmp << 8U) );
//  ((SCB_Type *) ((0xE000E000UL) + 0x0D00UL) )->AIRCR = reg_value;
//
//#endif

void configurePriorityGrouping(unsigned code) {
  SFRint<unsigned, 0xE000ED0C>() = ((~code & 7) << 8) | (0x05FA << 16); //5FA is a guard against random writes.
}

extern "C" { // to keep names simple for "alias" processor
void unhandledFault(void) {
  int num = theInterruptController.active;

  if (num >= 4) {
    theInterruptController.priority[num - 4] = 0xFF; // lower them as much as possible
  }
  switch (num) {
    default://added to stifle compiler warning.
    case 0: // surreal: stack pointer init rather than an interrupt
    case 1: // reset: also would be surreal to get here.
      break;
    case 2: // NMI
      // nothing to do, but pin doesn't exist on chip of interest to me
      break;
    case 3: // hard Fault
      /** infinite recursion gets here, stack trashing, I've had vptr's go bad...*/
      generateHardReset(); // since we usually get into an infinite loop.
      /* used hard reset rather than soft as my hardware module interfaces expect it.*/
      break;//# leave this here in case generateHardReset loses its 'never returns' attribute.
    case 4: // memmanage
      theInterruptController.memoryFaultEnable = 0;
      break;
    case 5: // bus
      theInterruptController.busFaultEnable = 0;
      break;
    case 6: // usage
      theInterruptController.usageFaultEnable = 0;
      break;
    case 7: // nothing
    case 8: // nothing
    case 9: // nothing
    case 10: // nothing
    case 11: // sv call
      // do nothing
      break;
    case 12: // debug mon
    case 13: // none
    case 14: // pend SV (service requested by bit set rather than instruction
      break;
    case 15: // systick
      // todo:M disable systick interrupts, but don't include systick module, annoying include loop forms.
      break;
  } /* switch */
} /* unhandledFault */


void disableInterrupt(unsigned irqnum) {
  ControlWord(Irq::biasFor(irqnum) | 0x180) = bitMask(Irq::bitFor(irqnum));
}


void unhandledInterruptHandler(void) {//#used by linker's vctor table support.
  /* turn it off so it doesn't happen again, and a handy breakpoint */
  disableInterrupt(theInterruptController.active - 16);
}
} // end extern "C"

// the stubs declare handler routines that deFault to unhandledInterruptHandler or unhandledFault if not otherwise declared.


#define stubFault(index) void FAULT ## index(void) __attribute__((weak, alias("unhandledFault")))

//typedef void (*Handler)(void);
stubFault(0);
stubFault(1);
stubFault(2);
stubFault(3);
stubFault(4);
stubFault(5);
stubFault(6);
stubFault(7);
stubFault(8);
stubFault(9);
stubFault(10);
stubFault(11);
stubFault(12);
stubFault(13);
stubFault(14);
stubFault(15);

//const in front of the below omitted it from the link, didn't even make it into the .o file as best I can tell.
Handler FaultTable[] __attribute__((section(".vectors.2"))) = {//0 is stack top, 1 is reset vector, 2 for faults, 3 for plain irqs.
  FaultName(2),
  FaultName(3),
  FaultName(4),
  FaultName(5),
  FaultName(6),
  FaultName(7), //NB: the lpc guys have a tendency to shove a checksum here.
  FaultName(8),
  FaultName(9),
  FaultName(10),
  FaultName(11),
  FaultName(12),
  FaultName(13),
  FaultName(14),
  FaultName(15),
};

//used by nvicTable.link
#define stub(irq) void IRQ ## irq(void) __attribute__((weak, alias("unhandledInterruptHandler")))

//if the following table doesn't exist use mkIrqs to build it for your processor
#include "nvicTable.link" //table in parent directory as it is project specific.
/* I've named the above .link as I am prebuilding tables for various processors and using a soft link to pick one.
 * Other people insert a massive switch here on #defined symbols for the processor, but my system only needs to know the number of the highest interrupt you will be using.
 * The file above creates names for interrupts using the Irqname( ) macro where the argument must be a preprocessor resolved decimal number.
 * Instead of dedicated names for each interrupt request you name your isr whatever pleases you then mention that it is a strong reference to Irqname( xx).
 * This does mean that the point of use needs to know the actual irq number, but that is also needed in order to control the nvic bits so you will have that at hand.
 * Unfortunately this is all c preprocessor magic so you can't use constexpr stuff to compute an interrupt request from some configuration knowledge.
 * */

void Irq::setAllPriorties(u8 prio) {
  //todo:1 do by groups of 4.
  u8 *priors=reinterpret_cast<u8*>(0xE000E400);
  for(int i=countof(VectorTable);i-->0;){
    priors[i]=prio<<4;
  }
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

//trying to get good assembler code on this one :)
void generateHardReset() {
  //maydo: DSB before and after the reset
  //lsdigit: 1 worked on stm32, 4 should have worked but looped under the debugger.
  unsigned pattern = 0x5FA0005U | (theInterruptController.airc & bitMask(8, 3));//retain priority group setting, JIC we don't reset that during startup
  do {//keep on hitting the bit until we reset.
    theInterruptController.airc = pattern;
    //probably should try 5 instead of bit 3 above in case different vendors misread the arm spec differently.
  } while (true);
}

#pragma clang diagnostic pop


#ifdef __linux__ //just compiling for syntax checking
bool IRQEN;
#else
//shared instances need this treatment.
const CPSI_i IRQEN;    //cmsis name
//now in core_cmfunc: const CPSI_i IrqEnable;//legacy name

#endif

