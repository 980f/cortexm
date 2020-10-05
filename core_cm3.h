#pragma once
/**************************************************************************//**
* operationally replaces CMSIS Cortex-M3 Core Peripheral Access Layer Header File
*/

#include "peripheraltypes.h"

#define DeclareCore(regname) extern CM3:: regname & the ## regname;

/* Memory mapping of Cortex-M3 Hardware */
#define SCS_BASE            (0xE000E000UL)                            /*!< System Control Space Base Address */
#define ITM_BASE            (0xE0000000UL)                            /*!< ITM Base Address                  */


namespace CM3 {
/*******************************************************************************
 *                 CMSIS definitions
 ******************************************************************************/
/** \defgroup CMSIS_core_definitions CMSIS Core Definitions
 *  This file defines all structures and symbols for CMSIS core:
 *  - CMSIS version number
 *  - Cortex-M core
 *  - Cortex-M core Revision Number
 *  @{
 */

/*  CMSIS CM3 definitions */
#define __CM3_CMSIS_VERSION_MAIN  (2)                                                       /*!< [31:16] CMSIS HAL main version */
#define __CM3_CMSIS_VERSION_SUB   (0)                                                       /*!< [15:0]  CMSIS HAL sub version  */
#define __CM3_CMSIS_VERSION       ((__CM3_CMSIS_VERSION_MAIN << 16) | __CM3_CMSIS_VERSION_SUB) /*!< CMSIS HAL version number       */

#ifndef __CORTEX_M
#define __CORTEX_M                (3)                                                       /*!< Cortex core                    */
#endif

#include <stdint.h>                      /*!< standard types definitions                      */
#include "core_cmInstr.h"                /*!< Core Instruction Access                         */
#include "core_cmFunc.h"                 /*!< Core Function Access                            */

#ifndef __CMSIS_GENERIC

#ifndef CORE_CM3_H_DEPENDANT
#define CORE_CM3_H_DEPENDANT

/*******************************************************************************
 *                 Register Abstraction
 ******************************************************************************/
/** \defgroup CMSIS_core_register CMSIS Core Register
 *  Core Register contain:
 *  - Core Register
 *  - Core SCB Register
 *  - Core Debug Register
 *  - Core MPU Register
 */


/** \brief  Union type to access the Application Program Status Register (APSR). */
union APSR {
//putting the word entity first allows us to initialize with {}.
  uint32_t w;                            /*!< Type      used for word access                  */
  struct {
#if (__CORTEX_M == 4)
    unsigned : 16;              /*!< bit:  0..15  Reserved                           */
    unsigned GE : 4;                       /*!< bit: 16..19  Greater than or Equal flags        */
    unsigned : 7;               /*!< bit: 20..26  Reserved                           */
#else
    unsigned : 27;              /*!< bit:  0..26  Reserved                           */
#endif
    unsigned Q : 1;                        /*!< bit:     27  Saturation condition flag          */
    unsigned V : 1;                        /*!< bit:     28  Overflow condition code flag       */
    unsigned C : 1;                        /*!< bit:     29  Carry condition code flag          */
    unsigned Z : 1;                        /*!< bit:     30  Zero condition code flag           */
    unsigned N : 1;                        /*!< bit:     31  Negative condition code flag       */
  } b;                                   /*!< Structure used for bit  access                  */
 
};


/** \brief  Union type to access the Interrupt Program Status Register (IPSR).
 */
union IPSR {
  uint32_t w;                            /*!< Type      used for word access                  */
  struct {
    unsigned ISR : 9;                      /*!< bit:  0.. 8  Exception number                   */
    unsigned  : 23;              /*!< bit:  9..31  Reserved                           */
  } b;                                   /*!< Structure used for bit  access                  */
  
};


/** \brief  Union type to access the Special-Purpose Program Status Registers (xPSR).
 */
union xPSR{
  uint32_t w;                            /*!< Type      used for word access                  */
  struct {
    unsigned ISR : 9;                      /*!< bit:  0.. 8  Exception number                   */
#if (__CORTEX_M == 4)
    unsigned  : 7;               /*!< bit:  9..15  Reserved                           */
    unsigned GE : 4;                       /*!< bit: 16..19  Greater than or Equal flags        */
    unsigned  : 4;               /*!< bit: 20..23  Reserved                           */
#else
    unsigned : 15;              /*!< bit:  9..23  Reserved                           */
#endif
    unsigned T : 1;                        /*!< bit:     24  Thumb bit        (read 0)          */
    unsigned IT : 2;                       /*!< bit: 25..26  saved IT state   (read 0)          */
    unsigned Q : 1;                        /*!< bit:     27  Saturation condition flag          */
    unsigned V : 1;                        /*!< bit:     28  Overflow condition code flag       */
    unsigned C : 1;                        /*!< bit:     29  Carry condition code flag          */
    unsigned Z : 1;                        /*!< bit:     30  Zero condition code flag           */
    unsigned N : 1;                        /*!< bit:     31  Negative condition code flag       */
  } b;                                   /*!< Structure used for bit  access                  */
 
} ;


/** \brief  Union type to access the Control Registers (CONTROL).
 */
union CONTROL{
  uint32_t w;                            /*!< Type      used for word access                  */
  struct {
    unsigned nPRIV : 1;                    /*!< bit:      0  Execution privilege in Thread mode */
    unsigned SPSEL : 1;                    /*!< bit:      1  Stack to be used                   */
    unsigned FPCA : 1;                     /*!< bit:      2  FP extension active flag           */
    unsigned : 29;              /*!< bit:  3..31  Reserved                           */
  } b;                                   /*!< Structure used for bit  access                  */
};

/*@} end of group CMSIS_CORE */


///** \ingroup  CMSIS_core_register


/** \ingroup  CMSIS_core_register
 *   \defgroup CMSIS_SCB CMSIS SCB
 *  Type definitions for the Cortex-M System Control Block Registers
 *  @{
 */

/** \brief  Structure type to access the System Control Block (SCB).
 */
struct SCB {
  const SFR CPUID;                  /*!< Offset: 0x000 (R/ )  CPU ID Base Register                                  */
  SFR ICSR;                    /*!< Offset: 0x004 (R/W)  Interrupt Control State Register                      */
  SFR VTOR;                   /*!< Offset: 0x008 (R/W)  Vector Table Offset Register                          */
  SFR AIRCR;                  /*!< Offset: 0x00C (R/W)  Application Interrupt / Reset Control Register        */
  SFR SCR;                    /*!< Offset: 0x010 (R/W)  System Control Register                               */
  SFR CCR;                    /*!< Offset: 0x014 (R/W)  Configuration Control Register                        */
  SFR SHP[12];                /*!< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
  SFR SHCSR;                  /*!< Offset: 0x024 (R/W)  System Handler Control and State Register             */
  SFR CFSR;                   /*!< Offset: 0x028 (R/W)  Configurable Fault Status Register                    */
  SFR HFSR;                   /*!< Offset: 0x02C (R/W)  Hard Fault Status Register                            */
  SFR DFSR;                   /*!< Offset: 0x030 (R/W)  Debug Fault Status Register                           */
  SFR MMFAR;                  /*!< Offset: 0x034 (R/W)  Mem Manage Address Register                           */
  SFR BFAR;                   /*!< Offset: 0x038 (R/W)  Bus Fault Address Register                            */
  SFR AFSR;                   /*!< Offset: 0x03C (R/W)  Auxiliary Fault Status Register                       */
  const SFR PFR[2];                 /*!< Offset: 0x040 (R/ )  Processor Feature Register                            */
  const SFR DFR;                   /*!< Offset: 0x048 (R/ )  Debug Feature Register                                */
  const SFR ADR;                   /*!< Offset: 0x04C (R/ )  Auxiliary Feature Register                            */
  const SFR MMFR[4];               /*!< Offset: 0x050 (R/ )  Memory Model Feature Register                         */
  const SFR ISAR[5];               /*!< Offset: 0x060 (R/ )  ISA Feature Register                                  */
};

/*the following are used to assemble and tear apart values that are to be read or written in a single access.
 * they are an alternative to structs with bitfields as those are not portable across compilers.
 * For example:
 * uint32_t cpuinfo= SCB.CPUID;
 * unsigned implementer= SCB_CPUID_IMPLEMENTER()(cpuinfo);
*/
/* SCB CPUID Register Definitions */
typedef BitFielder<24,8> SCB_CPUID_IMPLEMENTER ; /*!< SCB CPUID: IMPLEMENTER */
typedef BitFielder<20,4> SCB_CPUID_VARIANT;      /*!< SCB CPUID: VARIANT */
typedef BitFielder<4,12> SCB_CPUID_PARTNO;       /*!< SCB CPUID: PARTNO */
typedef BitFielder<0,4>  SCB_CPUID_REVISION;     /*!< SCB CPUID: REVISION*/

/* SCB Interrupt Control State Register Definitions */
typedef BitPicker<31> SCB_ICSR_NMIPENDSET;  /*!< SCB ICSR: NMIPENDSET */
typedef BitPicker<28> SCB_ICSR_PENDSVSET;   /*!< SCB ICSR: PENDSVSET Mask */
typedef BitPicker<27> SCB_ICSR_PENDSVCLR;   /*!< SCB ICSR: PENDSVCLR Mask */
typedef BitPicker<26> SCB_ICSR_PENDSTSET;   /*!< SCB ICSR: PENDSTSET Mask */
typedef BitPicker<25> SCB_ICSR_PENDSTCLR;   /*!< SCB ICSR: PENDSTCLR Mask */
typedef BitPicker<23> SCB_ICSR_ISRPREEMPT;  /*!< SCB ICSR: ISRPREEMPT Mask */
typedef BitPicker<22> SCB_ICSR_ISRPENDING;  /*!< SCB ICSR: ISRPENDING Mask */
typedef BitFielder<12,9> SCB_ICSR_VECTPENDING; /*!< SCB ICSR: VECTPENDING Mask */
typedef BitPicker<11> SCB_ICSR_RETTOBASE;   /*!< SCB ICSR: RETTOBASE Mask */
typedef BitFielder<0,9>  SCB_ICSR_VECTACTIVE;  /*!< SCB ICSR: VECTACTIVE Mask */

/* SCB Interrupt Control State Register Definitions */
typedef BitPicker<29> SCB_VTOR_TBLBASE;     /*!< SCB VTOR: TBLBASE Mask */
typedef BitFielder<7,23> SCB_VTOR_TBLOFF;      /*!< SCB VTOR: TBLOFF Mask */

/* SCB Application Interrupt and Reset Control Register Definitions */
typedef BitFielder<16,16> SCB_AIRCR_VECTKEY;
typedef BitFielder<16,16> SCB_AIRCR_VECTKEYSTAT;

//typedef BitPicker<#define SCB_AIRCR_ENDIANESS_Pos            15                                             /*!< SCB AIRCR: ENDIANESS Position */
//#define SCB_AIRCR_ENDIANESS_Msk            (1UL << SCB_AIRCR_ENDIANESS_Pos)               /*!< SCB AIRCR: ENDIANESS Mask */

//typedef BitFielder<8,3> SCB_AIRCR_PRIGROUP;
//typedef BitPicker<#define SCB_AIRCR_SYSRESETREQ_Pos           2                                             /*!< SCB AIRCR: SYSRESETREQ Position */
//#define SCB_AIRCR_SYSRESETREQ_Msk          (1UL << SCB_AIRCR_SYSRESETREQ_Pos)             /*!< SCB AIRCR: SYSRESETREQ Mask */

//typedef BitPicker<#define SCB_AIRCR_VECTCLRACTIVE_Pos         1                                             /*!< SCB AIRCR: VECTCLRACTIVE Position */
//#define SCB_AIRCR_VECTCLRACTIVE_Msk        (1UL << SCB_AIRCR_VECTCLRACTIVE_Pos)           /*!< SCB AIRCR: VECTCLRACTIVE Mask */

//typedef BitPicker<#define SCB_AIRCR_VECTRESET_Pos             0                                             /*!< SCB AIRCR: VECTRESET Position */
//#define SCB_AIRCR_VECTRESET_Msk            (1UL << SCB_AIRCR_VECTRESET_Pos)               /*!< SCB AIRCR: VECTRESET Mask */

///* SCB System Control Register Definitions */
//typedef BitPicker<#define SCB_SCR_SEVONPEND_Pos               4                                             /*!< SCB SCR: SEVONPEND Position */
//#define SCB_SCR_SEVONPEND_Msk              (1UL << SCB_SCR_SEVONPEND_Pos)                 /*!< SCB SCR: SEVONPEND Mask */

//typedef BitPicker<#define SCB_SCR_SLEEPDEEP_Pos               2                                             /*!< SCB SCR: SLEEPDEEP Position */
//#define SCB_SCR_SLEEPDEEP_Msk              (1UL << SCB_SCR_SLEEPDEEP_Pos)                 /*!< SCB SCR: SLEEPDEEP Mask */

//typedef BitPicker<#define SCB_SCR_SLEEPONEXIT_Pos             1                                             /*!< SCB SCR: SLEEPONEXIT Position */
//#define SCB_SCR_SLEEPONEXIT_Msk            (1UL << SCB_SCR_SLEEPONEXIT_Pos)               /*!< SCB SCR: SLEEPONEXIT Mask */

///* SCB Configuration Control Register Definitions */
//typedef BitPicker<#define SCB_CCR_STKALIGN_Pos                9                                             /*!< SCB CCR: STKALIGN Position */
//#define SCB_CCR_STKALIGN_Msk               (1UL << SCB_CCR_STKALIGN_Pos)                  /*!< SCB CCR: STKALIGN Mask */

//typedef BitPicker<#define SCB_CCR_BFHFNMIGN_Pos               8                                             /*!< SCB CCR: BFHFNMIGN Position */
//#define SCB_CCR_BFHFNMIGN_Msk              (1UL << SCB_CCR_BFHFNMIGN_Pos)                 /*!< SCB CCR: BFHFNMIGN Mask */

//typedef BitPicker<#define SCB_CCR_DIV_0_TRP_Pos               4                                             /*!< SCB CCR: DIV_0_TRP Position */
//#define SCB_CCR_DIV_0_TRP_Msk              (1UL << SCB_CCR_DIV_0_TRP_Pos)                 /*!< SCB CCR: DIV_0_TRP Mask */

//typedef BitPicker<#define SCB_CCR_UNALIGN_TRP_Pos             3                                             /*!< SCB CCR: UNALIGN_TRP Position */
//#define SCB_CCR_UNALIGN_TRP_Msk            (1UL << SCB_CCR_UNALIGN_TRP_Pos)               /*!< SCB CCR: UNALIGN_TRP Mask */

//typedef BitPicker<#define SCB_CCR_USERSETMPEND_Pos            1                                             /*!< SCB CCR: USERSETMPEND Position */
//#define SCB_CCR_USERSETMPEND_Msk           (1UL << SCB_CCR_USERSETMPEND_Pos)              /*!< SCB CCR: USERSETMPEND Mask */

//typedef BitPicker<#define SCB_CCR_NONBASETHRDENA_Pos          0                                             /*!< SCB CCR: NONBASETHRDENA Position */
//#define SCB_CCR_NONBASETHRDENA_Msk         (1UL << SCB_CCR_NONBASETHRDENA_Pos)            /*!< SCB CCR: NONBASETHRDENA Mask */

///* SCB System Handler Control and State Register Definitions */
typedef BitPicker<18> SCB_SHCSR_USGFAULTENA;
typedef BitPicker<17> SCB_SHCSR_BUSFAULTENA;
typedef BitPicker<16> SCB_SHCSR_MEMFAULTENA;

typedef BitPicker<15> SCB_SHCSR_SVCALLPENDED;
typedef BitPicker<14> SCB_SHCSR_BUSFAULTPENDED;
typedef BitPicker<13> SCB_SHCSR_MEMFAULTPENDED;
typedef BitPicker<12> SCB_SHCSR_USGFAULTPENDED;
typedef BitPicker<11> SCB_SHCSR_SYSTICKACT;
typedef BitPicker<10> SCB_SHCSR_PENDSVACT;
typedef BitPicker<8> SCB_SHCSR_MONITORACT;
typedef BitPicker<7> SCB_SHCSR_SVCALLACT;

typedef BitPicker<3> SCB_SHCSR_USGFAULTACT;
typedef BitPicker<1> SCB_SHCSR_BUSFAULTACT;
typedef BitPicker<0> SCB_SHCSR_MEMFAULTACT;

/* SCB Configurable Fault Status Registers Definitions */
typedef BitFielder<16,16> SCB_CFSR_USGFAULTSR;
typedef BitFielder<8,8>   SCB_CFSR_BUSFAULTSR;
typedef BitFielder<0,8> SCB_CFSR_MEMFAULTSR;
/* SCB Hard Fault Status Registers Definitions */
typedef BitPicker<31> SCB_HFSR_DEBUGEVT;
typedef BitPicker<30> SCB_HFSR_FORCED;
typedef BitPicker<1> SCB_HFSR_VECTTBL;
/* SCB Debug Fault Status Register Definitions */
typedef BitPicker<4> SCB_DFSR_EXTERNAL;
typedef BitPicker<3> SCB_DFSR_VCATCH;
typedef BitPicker<2> SCB_DFSR_DWTTRAP;
typedef BitPicker<1> SCB_DFSR_BKPT;
typedef BitPicker<0> SCB_DFSR_HALTED;                                            /*!< SCB DFSR: HALTED Position */

/*@} end of group CMSIS_SCB */


/** \brief  Structure type to access the Interrupt Type Register.
 */
struct InterruptType {
  uint32_t RESERVED0;
  const SFR ICTR;                  /*!< Offset: 0x004 (R/ )  Interrupt Control Type Register */
#if (__CM3_REV >= 0x200)
  SFR ACTLR;                  /*!< Offset: 0x008 (R/W)  Auxiliary Control Register      */
#else
  uint32_t RESERVED1;
#endif
};

/* Interrupt Controller Type Register Definitions */
typedef BitFielder<0,5> IntType_ICTR_INTLINESNUM;

/* Auxiliary Control Register Definitions */
#define IntType_ACTLR_DISFOLD_Pos     2                                                   /*!< InterruptType ACTLR: DISFOLD Position */
#define IntType_ACTLR_DISFOLD_Msk    (1UL << IntType_ACTLR_DISFOLD_Pos)                   /*!< InterruptType ACTLR: DISFOLD Mask */

#define IntType_ACTLR_DISDEFWBUF_Pos  1                                                   /*!< InterruptType ACTLR: DISDEFWBUF Position */
#define IntType_ACTLR_DISDEFWBUF_Msk (1UL << IntType_ACTLR_DISDEFWBUF_Pos)                /*!< InterruptType ACTLR: DISDEFWBUF Mask */

#define IntType_ACTLR_DISMCYCINT_Pos  0                                                   /*!< InterruptType ACTLR: DISMCYCINT Position */
#define IntType_ACTLR_DISMCYCINT_Msk (1UL << IntType_ACTLR_DISMCYCINT_Pos)                /*!< InterruptType ACTLR: DISMCYCINT Mask */

/*@}*/ /* end of group CMSIS_InterruptType */


#if (__MPU_PRESENT == 1)
/** \ingroup  CMSIS_core_register
 *   \defgroup CMSIS_MPU CMSIS MPU
 *  Type definitions for the Cortex-M Memory Protection Unit (MPU)
 *  @{
 */

/** \brief  Structure type to access the Memory Protection Unit (MPU).
 */
struct MPU {
  const SFR TYPE;                  /*!< Offset: 0x000 (R/ )  MPU Type Register                              */
  SFR CTRL;                   /*!< Offset: 0x004 (R/W)  MPU Control Register                           */
  SFR RNR;                    /*!< Offset: 0x008 (R/W)  MPU Region RNRber Register                     */
  
  SFR RBAR;                   /*!< Offset: 0x00C (R/W)  MPU Region Base Address Register               */
  SFR RASR;                   /*!< Offset: 0x010 (R/W)  MPU Region Attribute and Size Register         */
  
  SFR RBAR_A1;                /*!< Offset: 0x014 (R/W)  MPU Alias 1 Region Base Address Register       */
  SFR RASR_A1;                /*!< Offset: 0x018 (R/W)  MPU Alias 1 Region Attribute and Size Register */

  SFR RBAR_A2;                /*!< Offset: 0x01C (R/W)  MPU Alias 2 Region Base Address Register       */
  SFR RASR_A2;                /*!< Offset: 0x020 (R/W)  MPU Alias 2 Region Attribute and Size Register */
  
  SFR RBAR_A3;                /*!< Offset: 0x024 (R/W)  MPU Alias 3 Region Base Address Register       */
  SFR RASR_A3;                /*!< Offset: 0x028 (R/W)  MPU Alias 3 Region Attribute and Size Register */
};

/* MPU Type Register */
#define MPU_TYPE_IREGION_Pos               16                                             /*!< MPU TYPE: IREGION Position */
#define MPU_TYPE_IREGION_Msk               (0xFFUL << MPU_TYPE_IREGION_Pos)               /*!< MPU TYPE: IREGION Mask */

#define MPU_TYPE_DREGION_Pos                8                                             /*!< MPU TYPE: DREGION Position */
#define MPU_TYPE_DREGION_Msk               (0xFFUL << MPU_TYPE_DREGION_Pos)               /*!< MPU TYPE: DREGION Mask */

#define MPU_TYPE_SEPARATE_Pos               0                                             /*!< MPU TYPE: SEPARATE Position */
#define MPU_TYPE_SEPARATE_Msk              (1UL << MPU_TYPE_SEPARATE_Pos)                 /*!< MPU TYPE: SEPARATE Mask */

/* MPU Control Register */
#define MPU_CTRL_PRIVDEFENA_Pos             2                                             /*!< MPU CTRL: PRIVDEFENA Position */
#define MPU_CTRL_PRIVDEFENA_Msk            (1UL << MPU_CTRL_PRIVDEFENA_Pos)               /*!< MPU CTRL: PRIVDEFENA Mask */

#define MPU_CTRL_HFNMIENA_Pos               1                                             /*!< MPU CTRL: HFNMIENA Position */
#define MPU_CTRL_HFNMIENA_Msk              (1UL << MPU_CTRL_HFNMIENA_Pos)                 /*!< MPU CTRL: HFNMIENA Mask */

#define MPU_CTRL_ENABLE_Pos                 0                                             /*!< MPU CTRL: ENABLE Position */
#define MPU_CTRL_ENABLE_Msk                (1UL << MPU_CTRL_ENABLE_Pos)                   /*!< MPU CTRL: ENABLE Mask */

/* MPU Region Number Register */
#define MPU_RNR_REGION_Pos                  0                                             /*!< MPU RNR: REGION Position */
#define MPU_RNR_REGION_Msk                 (0xFFUL << MPU_RNR_REGION_Pos)                 /*!< MPU RNR: REGION Mask */

/* MPU Region Base Address Register */
#define MPU_RBAR_ADDR_Pos                   5                                             /*!< MPU RBAR: ADDR Position */
#define MPU_RBAR_ADDR_Msk                  (0x7FFFFFFUL << MPU_RBAR_ADDR_Pos)             /*!< MPU RBAR: ADDR Mask */

#define MPU_RBAR_VALID_Pos                  4                                             /*!< MPU RBAR: VALID Position */
#define MPU_RBAR_VALID_Msk                 (1UL << MPU_RBAR_VALID_Pos)                    /*!< MPU RBAR: VALID Mask */

#define MPU_RBAR_REGION_Pos                 0                                             /*!< MPU RBAR: REGION Position */
#define MPU_RBAR_REGION_Msk                (0xFUL << MPU_RBAR_REGION_Pos)                 /*!< MPU RBAR: REGION Mask */

/* MPU Region Attribute and Size Register */
#define MPU_RASR_XN_Pos                    28                                             /*!< MPU RASR: XN Position */
#define MPU_RASR_XN_Msk                    (1UL << MPU_RASR_XN_Pos)                       /*!< MPU RASR: XN Mask */

#define MPU_RASR_AP_Pos                    24                                             /*!< MPU RASR: AP Position */
#define MPU_RASR_AP_Msk                    (7UL << MPU_RASR_AP_Pos)                       /*!< MPU RASR: AP Mask */

#define MPU_RASR_TEX_Pos                   19                                             /*!< MPU RASR: TEX Position */
#define MPU_RASR_TEX_Msk                   (7UL << MPU_RASR_TEX_Pos)                      /*!< MPU RASR: TEX Mask */

#define MPU_RASR_S_Pos                     18                                             /*!< MPU RASR: Shareable bit Position */
#define MPU_RASR_S_Msk                     (1UL << MPU_RASR_S_Pos)                        /*!< MPU RASR: Shareable bit Mask */

#define MPU_RASR_C_Pos                     17                                             /*!< MPU RASR: Cacheable bit Position */
#define MPU_RASR_C_Msk                     (1UL << MPU_RASR_C_Pos)                        /*!< MPU RASR: Cacheable bit Mask */

#define MPU_RASR_B_Pos                     16                                             /*!< MPU RASR: Bufferable bit Position */
#define MPU_RASR_B_Msk                     (1UL << MPU_RASR_B_Pos)                        /*!< MPU RASR: Bufferable bit Mask */

#define MPU_RASR_SRD_Pos                    8                                             /*!< MPU RASR: Sub-Region Disable Position */
#define MPU_RASR_SRD_Msk                   (0xFFUL << MPU_RASR_SRD_Pos)                   /*!< MPU RASR: Sub-Region Disable Mask */

#define MPU_RASR_SIZE_Pos                   1                                             /*!< MPU RASR: Region Size Field Position */
#define MPU_RASR_SIZE_Msk                  (0x1FUL << MPU_RASR_SIZE_Pos)                  /*!< MPU RASR: Region Size Field Mask */

#define MPU_RASR_ENA_Pos                     0                                            /*!< MPU RASR: Region enable bit Position */
#define MPU_RASR_ENA_Msk                    (0x1UL << MPU_RASR_ENA_Pos)                   /*!< MPU RASR: Region enable bit Disable Mask */

/*@} end of group CMSIS_MPU */
#endif // if (__MPU_PRESENT == 1)


/** \ingroup  CMSIS_core_register
 *  @{
 */



// #define SCB_BASE            (SCS_BASE + 0x0D00UL)                   /*!< System Control Block Base Address */

// DeclareCore(InterruptType);        /*!< Interrupt Type Register           */
// DeclareCore(SCB);       /*!< SCB configuration struct          */
// #define SysTick             ((SysTick_Type *) SysTick_BASE)    /*!< SysTick configuration struct      */
// #define NVIC                ((NVIC_Type *) NVIC_BASE)       /*!< NVIC configuration struct         */
// DeclareCore(ITM);

#if (__MPU_PRESENT == 1)
// DeclareCore(MPU)       /*!< Memory Protection Unit            */
#endif

/*@} */


#endif /* __CORE_CM3_H_DEPENDANT */

#endif /* __CMSIS_GENERIC */
}
