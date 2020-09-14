#ifndef _IRQNUMS_H
#define _IRQNUMS_H  "(C) Andy Heilveil (980F) 2020 sep14"
/** this started life as a comprehensive list, but is going to be broken up into the associated modules. */

#define  WWDG_irq 0
#define  PVD_irq 1
#define  FLASH_irq  4
#define  RCC_irq  5
#define  DCMI_irq  78
#define  RNG_irq 80
#define  FPU_irq  81
#define  FSMC_irq  48
#define  SDIO_irq  49


#define  ADC_irq  18
//conflicts with T6
#define  DAC_irq 54

#define  TAMP_STAMP_irq 2
#define  RTC_WKUP_irq  3
#define  RTC_Alarm_irq  41


#define  EXTI0_irq 6
#define  EXTI1_irq 7
#define  EXTI2_irq 8
#define  EXTI3_irq 9
#define  EXTI4_irq 10
#define  EXTI9_5_irq  23
#define  EXTI15_10_irq  40

#define  CAN1_TX_irq 19
#define  CAN1_RX0_irq  20
#define  CAN1_RX1_irq  21
#define  CAN1_SCE_irq  22

#define  CAN2_TX_irq  63
#define  CAN2_RX0_irq  64
#define  CAN2_RX1_irq  65
#define  CAN2_SCE_irq  66

#define  I2C1_EV_irq  31
#define  I2C1_ER_irq  32
#define  I2C2_EV_irq  33
#define  I2C2_ER_irq  34
#define  I2C3_EV_irq  72
#define  I2C3_ER_irq  73

#define  SPI1_irq  35
#define  SPI2_irq  36
#define  SPI3_irq  51

#define  OTG_FS_WKUP_irq  42
#define  OTG_FS_irq  67

#define  ETH_irq  61
#define  ETH_WKUP_irq 62

#define  OTG_HS_EP1_OUT_irq  74
#define  OTG_HS_EP1_IN_irq  75
#define  OTG_HS_WKUP_irq  76
#define  OTG_HS_irq  77


#endif //CDCHOST_IRQNUMS_H
