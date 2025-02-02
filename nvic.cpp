/*all about interrupts*/
#include "nvic.h"
#include "eztypes.h"
#include "peripheraltypes.h"
#include "cruntime.h"

constexpr unsigned PriorityShift=4;//todo: this '4' is ST's value, may need to make dependent upon processor defines.
// volatile unsigned CriticalSection::nesting = 0;
/////////////////////////////////

u8 setInterruptPriorityFor(int number, u8 newvalue) { // one byte each, often only some high bits are implemented
  //fault priorities are at ED14 + 4..15, 0..3 are not allowed to be reprioritized, some others are also ignored
  //irq priorities are a byte array at E400
  u8 &priorityRegister(*reinterpret_cast<u8 *>(number < 0 ? 0xE000'ED14 - number : (0xE000'E400 + number)));
  u8 oldvalue = priorityRegister;
  priorityRegister = newvalue << PriorityShift;
  return oldvalue >> PriorityShift;
}

/////////////////////////////////

void configurePriorityGrouping(unsigned code){
  SFRint<unsigned,SCB(0x0C)>()= ((~code & 7) << 8) | (0x05FA<<16); //5FA is a guard against random writes.
}

extern "C" { // to keep names simple for "alias" processor
  void unhandledFault(void){
    unsigned num = SFRfield<SCB(0x04),0,9>();

    if(num >= 4) {
      ControlItem<uint8_t >(SCB(0x18+num-4)) = 0xFF; // lower its priority as much as possible in case stifling it fails to stick
    }
    switch(num) {
    default://added to stifle compiler warning.
    case 0: // surreal: stack pointer init rather than an interrupt
    case 1: // reset: also would be surreal to get here.
      break;
    case 2: // NMI
      // nothing to do, but pin doesn't exist on chip of interest to me
      break;
    case 3: // hard Fault
      /** infinite recursion gets here, stack trashing, I've had vptr's go bad...*/
      //usage fault coprocessor gets you here, try turning the FPU on before you use it!
      generateHardReset(); // since we usually get into an infinite loop.
      /* used hard reset rather than soft as my hardware module interfaces expect it.*/
      break;//# leave this here in case generateHardReset loses its 'never returns' attribute.
    case 4: // memmanage
      SFRbit<SCB(0x24),16>()= false;
      break;
    case 5: // bus
      SFRbit<SCB(0x24),17>()= false;
      break;
    case 6: // usage
      SFRbit<SCB(0x24),18>()= false;
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


  void disableInterrupt(unsigned irqnum){
    ControlWord(Irq::biasFor(irqnum)|0x180)=bitMask(Irq::bitFor(irqnum));
  }


  void unhandledInterruptHandler(void) {//#used by linker's vctor table support.
    /* turn it off so it doesn't happen again, and also is a handy breakpoint */
    unsigned num = SFRfield<SCB(0x04),0,9>();
    disableInterrupt(num - 16);
  }
} // end extern "C"

// the stubs declare handler routines that deFault to unhandledInterruptHandler or unhandledFault if not otherwise declared.
#define stubFault(index) void FAULT ## index(void) __attribute__((weak, alias("unhandledFault")))
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

//used by nvicTable.inc
#define stub(irq) void IRQ ## irq(void) __attribute__((weak, alias("unhandledInterruptHandler")))

//if the following table doesn't exist use mkIrqs to build it for your processor

#include "nvicTable.inc" //this table is expected in parent directory as it is project specific.
/* nvicTable.inc is creatable by the cortexm/mkIrqs script which is invoked by the CMake setup included in 980f cortexm repo.
 * The file above creates names for interrupts using the Irqname( ) macro where the argument must be a preprocessor resolved decimal number.
 * Instead of dedicated names for each interrupt request you name your isr whatever pleases you then mention that it is a strong reference to Irqname( xx).
 * This does mean that the point of use needs to know the actual irq number, but that is also needed in order to control the nvic bits so you will have that at hand.
 * Unfortunately this is all c preprocessor magic so you can't use constexpr stuff to compute an interrupt request from some configuration knowledge.
 * */

void Irq::setAllPriorties(u8 prio) {
  //todo:1 do by groups of 4.
  u8 *priors=reinterpret_cast<u8*>(0xE000'E400);
  for(int i=countof(VectorTable);i-->0;){//#VectorTable is declared in nvicTable.inc
    priors[i]=prio<<PriorityShift;
  }
}



///* ##########################   NVIC functions  #################################### */
///** \ingroup  CMSIS_Core_FunctionInterface
//    \defgroup CMSIS_Core_NVICFunctions CMSIS Core NVIC Functions
//  @{
// */

///** \brief  Set Priority Grouping

//  This function sets the priority grouping field using the required unlock sequence.
//  The parameter PriorityGroup is assigned to the field SCB->AIRCR [10:8] PRIGROUP field.
//  Only values from 0..7 are used.
//  In case of a conflict between priority grouping and available
//  priority bits (__NVIC_PRIO_BITS) the smallest possible priority group is set.

//    \param [in]      PriorityGroup  Priority grouping field
// */
// static __INLINE void NVIC_SetPriorityGrouping(uint32_t PriorityGroup)
// {
//  uint32_t reg_value;
//  uint32_t PriorityGroupTmp = (PriorityGroup & 0x07);                         /* only values 0..7 are used          */

//  reg_value  =  SCB->AIRCR;                                                   /* read old register configuration    */
//  reg_value &= ~(SCB_AIRCR_VECTKEY_Msk | SCB_AIRCR_PRIGROUP_Msk);             /* clear bits to change               */
//  reg_value  =  (reg_value                       |
//                (0x5FA << SCB_AIRCR_VECTKEY_Pos) |
//                (PriorityGroupTmp << 8));                                     /* Insert write key and priorty group */
//  SCB->AIRCR =  reg_value;
// }
/*@} end of CMSIS_Core_NVICFunctions */

#ifdef __linux__ //just compiling for syntax checking
bool IRQEN;
#else
//shared instances need this treatment.
const IrqEnabler IRQEN;    //cmsis name
#endif
