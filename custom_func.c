/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with
 *Renesas products. No other uses are authorized. This software is owned by Renesas Electronics
 *Corporation and is protected under all applicable laws, including copyright laws. THIS SOFTWARE IS
 *PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 *PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.TO THE
 *MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF
 *ITS AFFILIATED COMPANIES SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR
 *CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES
 *HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. Renesas reserves the right, without notice,
 *to make changes to this software and to discontinue the availability of this software. By using
 *this software, you agree to the additional terms and conditions found by accessing the following
 *link: http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2012, 2021 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

/***********************************************************************************************************************
 * File Name    : custom_func.c
 * Version      : CodeGenerator for RL78/F13 V2.03.07.02 [08 Nov 2021]
 * Device(s)    : R5F10BMG
 * Tool-Chain   : CCRL
 * Description  : User initialization and foreground callbacks.
 * Creation Date: 2025/3/3
 ***********************************************************************************************************************/

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
/* Project/API reading notes: Project-owned initialization and foreground work.
 * The selected main/startup adapter calls hardware_init() once and loop() repeatedly.
 * Keep application changes here so generated entry sources remain replaceable; lower-level APIs
 * live in this project's hal directories.
 */
#include "r_cg_macrodriver.h"
#include "r_cg_cgc.h"
#include "r_cg_port.h"
#include "r_cg_serial.h"
#include "r_cg_timer.h"
/* Start user code for include. Do not edit comment generated here */
#include <stdio.h>

/* Flash self-programming library */
#include "fsl.h"       /* Library header file */
#include "fsl_types.h" /* Library header file */

#include "can_boot_transport.h"
#include "sw_crc32.h"
#include "user_def.h"
#include "fsl_flash_interface.h"
#include "f13_board_support.h"

/* End user code. Do not edit comment generated here */
#include "r_cg_userdefine.h"

/***********************************************************************************************************************
Pragma directive
***********************************************************************************************************************/
/* Start user code for pragma. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Global variables and functions
***********************************************************************************************************************/
/* Start user code for global. Do not edit comment generated here */
volatile bool g_app_required_update = false;
signed long i32Err;
unsigned char g_fsl_init_status = FSL_ERR_PARAMETER;
unsigned char g_update_timeout_enabled = 0U;
unsigned char g_can_init_status = 0U;

enum
{
    RAM_FLAG_INDICATE_BOOT_APP, // 0xAA : boot , 0x55 : app
    RAM_FLAG_RESERVE2,
    RAM_FLAG_RESERVE3,
    RAM_FLAG_RESERVE4,
    RAM_FLAG_RESERVE5,
    RAM_FLAG_RESERVE6,
    RAM_FLAG_RESERVE7,
    RAM_FLAG_RESERVE8,
    RAM_FLAG_RESERVE9,
    RAM_FLAG_RESERVE10,
    RAM_FLAG_RESERVE11,
    RAM_FLAG_RESERVE12,
    RAM_FLAG_RESERVE13,
    RAM_FLAG_RESERVE14,
    RAM_FLAG_RESERVE15,
    RAM_FLAG_RESERVE16,
    RAM_FLAG_MAX,
};

#pragma address(reset_to_bootloader = 0x000FF700)
volatile unsigned long reset_to_bootloader;
#pragma address(_no_init_global = 0x000FF710)
unsigned char _no_init_global[RAM_FLAG_MAX];

void _reset_by_illegal_memory_access(void);

/* Only constant, aligned contract offsets4/8 are used by internal callers. */
#define BootSharedWrite32(offset, value)                                                           \
    (*((unsigned long __far *)(BOOT_APP_SHARED_RAM_ADDRESS + (offset))) = (value))

#if CAN_BOOT_DEBUG_UART
static void BootLogText(const char *text)
{
    while (*text != '\0')
    {
        (void)putchar((int)*text);
        text++;
    }
}

static void BootLogHex8(unsigned char value)
{
    static const char digits[] = "0123456789ABCDEF";

    (void)putchar((int)digits[(value >> 4) & 0x0FU]);
    (void)putchar((int)digits[value & 0x0FU]);
}

static void BootLogCode(const char *label, unsigned char value)
{
    BootLogText(label);
    BootLogHex8(value);
    BootLogText("\r\n");
}
#else
#define BootLogText(text) ((void)0)
#define BootLogHex8(value) ((void)0)
#define BootLogCode(label, value) ((void)0)
#endif

unsigned char Sample_CodeFlashControl_SingleWrite(unsigned long i_u32_start_addr,
                                                  unsigned char __near *inp_u08_write_data)
{
    fsl_u08 dubRetCode = FSL_ERR_PARAMETER; /* Return code                              */
    dubRetCode =
        flash_write_packet_data(i_u32_start_addr, (__near unsigned char *)inp_u08_write_data, 0);
    return dubRetCode;
}

unsigned char Sample_CodeFlashControl_Erase(unsigned long i_u32_start_addr)
{
    fsl_u08 dubRetCode = FSL_ERR_PARAMETER; /* Return code                              */
    dubRetCode = flash_erase_app_area(i_u32_start_addr);
    return dubRetCode;
}

unsigned char InitCodeFlash(void)
{
    unsigned char status;

    DI();
    status = init_fcl();
    EI();
    return status;
}

unsigned char verify_application_checksum(unsigned long image_size)
{
    unsigned long crc_address;
    unsigned long CRC_stored;
    unsigned long CRC_cal;

    crc_address = FLASH_AREA + image_size - APP_CRC_SIZE;
    CRC_stored = *(__far unsigned long *)crc_address;
    CRC_cal = caculate_crc32_checksum(FLASH_AREA, image_size - APP_CRC_SIZE);

#if 1 // remove if need to pass checksum
// printf_tiny("(b)verify checksum\r\n");
// prevent app data corrupt , stay in boot loader
#if 0 // debug
    printf_tiny("(b)app:0x%X",CRC_stored>>16);
    printf_tiny("%X\r\n",CRC_stored&0xFFFF);
    printf_tiny("(b)cal:0x%X",CRC_cal>>16);
    printf_tiny("%X\r\n",CRC_cal&0xFFFF);
#endif
#endif

    return (CRC_stored == CRC_cal) ? (true) : (false);
}

/* Check the complete installed App before handoff: metadata/range, CRC trailer and the fixed App
 * jump-table destinations must all pass.
 */
unsigned char CAN_BootInstalledValid(void)
{
    unsigned long image_size;

    if (*(__far unsigned long *)FLASH_AREA == 0xFFFFFFFFUL)
    {
        BootLogText("(b)empty\r\n");
        return false;
    }

    image_size = 0UL;
    if (ImageMetadataValidate((const unsigned long __far *)APP_MANIFEST_ADDRESS) !=
        IMAGE_METADATA_OK)
    {
        BootLogText("(b)manifest\r\n");
        return false;
    }

    image_size = *(__far unsigned long *)(APP_MANIFEST_ADDRESS + IMAGE_MANIFEST_SIZE_OFFSET);
    return verify_application_checksum(image_size);
}

bool condition_check(bool application_valid)
{
    // if boot sign is there, then stay in bootloader
    if (reset_to_bootloader == BOOT_APP_RESET_TO_BOOT_SIGNATURE)
    {
        BootLogText("(b)requested\r\n");

        reset_to_bootloader = 0;
        return true;
    }

    return !application_valid;
}

// void _BusyWaitDelayMs(void)
// {
//     volatile uint32_t i;
//     BSP_DI();
//     #if 0   // wait delay , 1.0562ms
//     for (i = 0; i < 2000; i++)
//     {
//         __nop();
//     }
//     #else   // timer delay , 1.007ms @ clock src 2500KHz , fCLK/2^4 , 1ms
//     boot_R_TAU0_Channel0_Start();
//     while(!(TCR00 == 0));
//     boot_R_TAU0_Channel0_Stop();
//     #endif
//     BSP_EI();
// }

// void delayMearsurement_in_polling(void)
// {
//     unsigned char i = 0;
//     for ( i = 0; i < 20 ; i++)
//     {
//         P7_bit.no3 = ~P7_bit.no3;// P73
//         _BusyWaitDelayMs();
//     }
// }

void boot_Timer_1ms_IRQ(void)
{
    CAN_BootTimerTick();
}

/*
    F13 target board
    LED1 connected to P66, LED2 connected to P67
*/
// void LED_Toggle(void)
// {
//     // PIN_WRITE(6,6) = ~PIN_READ(6,6);
//     // PIN_WRITE(6,7) = ~PIN_READ(6,7);
//     P6_bit.no6 = ~P6_bit.no6;
//     P6_bit.no7 = ~P6_bit.no7;
// }

/*
    Reset Control Flag Register (RESF)
    BIT7 : TRAP
    BIT6 : 0
    BIT5 : 0
    BIT4 : WDCLRF
    BIT3 : 0
    BIT2 : 0
    BIT1 : IAWRF
    BIT0 : LVIRF
*/
// void check_reset_source(void)
// {
//     /*
//         Internal reset request by execution of illegal instruction
//         0  Internal reset request is not generated, or the RESF register is cleared.
//         1  Internal reset request is generated.
//     */
//     volatile uint8_t src = RESF;
//     printf_tiny("Reset Source <0x%08X>\r\n", src);

//     #if 1   //DEBUG , list reset source
//     if (src & BIT0)
//     {
//         printf_tiny("0)voltage detector (LVD)\r\n");
//     }
//     if (src & BIT1)
//     {
//         printf_tiny("1)illegal-memory access\r\n");
//     }
//     if (src & BIT2)
//     {
//         printf_tiny("2)EMPTY\r\n");
//     }
//     if (src & BIT3)
//     {
//         printf_tiny("3)EMPTY\r\n");
//     }
//     if (src & BIT4)
//     {
//         printf_tiny("4)watchdog timer (WDT) or clock monitor\r\n");
//     }
//     if (src & BIT5)
//     {
//         printf_tiny("5)EMPTY\r\n");
//     }
//     if (src & BIT6)
//     {
//         printf_tiny("6)EMPTY\r\n");
//     }
//     if (src & BIT7)
//     {
//         printf_tiny("7)execution of illegal instruction\r\n");
//     }
//     #endif
// }

/*
    7:Internal reset by execution of illegal instruction
    1:Internal reset by illegal-memory access
*/
// perform sofware reset
//  void _reset_by_illegal_instruction(void)
//  {
//      static const unsigned char illegal_Instruction = 0xFF;
//      void (*dummy) (void) = (void (*)(void))&illegal_Instruction;
//      dummy();
//  }

void _reset_by_illegal_memory_access(void)
{
#if 1
    IAWCTL |= 0x80;                         // switch IAWEN on (default off)
    *(__far volatile char *)0x00000 = 0x00; // write illegal address 0x00000(RESET VECTOR)

// const unsigned char ILLEGAL_ACCESS_ON = 0x80;
// IAWCTL |= ILLEGAL_ACCESS_ON;            // switch IAWEN on (default off)
// *(__far volatile char *)0x00000 = 0x00; //write illegal address 0x00000(RESET VECTOR)
#else
    signed char __far *a;                // Create a far-Pointer
    IAWCTL |= _80_CGC_ILLEGAL_ACCESS_ON; // switch IAWEN on (default off)
    a = (signed char __far *)0x0000;     // Point to 0x000000 (FLASH-ROM area)
    *a = 0;
#endif
}

// void RL78_soft_reset(unsigned char flag)
// {
//     switch(flag)
//     {
//         case 7: // do not use under debug mode
//             _reset_by_illegal_instruction();
//             break;
//         case 1:
//             _reset_by_illegal_memory_access();
//             break;
//     }
// }

// retarget printf
int __far putchar(int c)
{
    // F13 , UART0
    STMK0 = 1U; /* disable INTST0 interrupt */
    SDR00L = (unsigned char)c;
    while (STIF0 == 0)
    {
    }
    STIF0 = 0U; /* clear INTST0 interrupt flag */
    return c;
}

#pragma interrupt f13_boot_timer_isr
void __near f13_boot_timer_isr(void)
{
    /* Assembly owns the physical vector; this is a normal RETI ISR. */
    boot_Timer_1ms_IRQ();
}

void boot_R_TMR_RJ0_Stop(void)
{
    TRJCR0 &= (uint8_t)~_01_TMRJ_COUNT_START; /* disable TMRJ operation */
    TRJMK0 = 1U;                              /* disable INTTRJ0 interrupt */
    TRJIF0 = 0U;                              /* clear INTTRJ0 interrupt flag */
}

void boot_R_TMR_RJ0_Start(void)
{
    TRJIF0 = 0U;                    /* clear INTTRJ0 interrupt flag */
    TRJMK0 = 0U;                    /* enable INTTRJ0 interrupt */
    TRJCR0 |= _01_TMRJ_COUNT_START; /* enable TMRJ operation */
}

void boot_R_TMR_RJ0_Create(void)
{
    TRJ0EN = 1U;                              /* enable input clock supply */
    TRJCR0 &= (uint8_t)~_01_TMRJ_COUNT_START; /* disable TMRJ0 operation */
    TRJMK0 = 1U;                              /* disable INTTRJ0 interrupt */
    TRJIF0 = 0U;                              /* clear INTTRJ0 interrupt flag */
    /* Set INTTRJ0 low priority */
    TRJPR10 = 1U;
    TRJPR00 = 1U;
    TRJMR0 = _00_TMRJ_MODE_TIMER | _00_TMRJ_COUNT_SOURCE_FCLK;
    TRJIOC0 = _00_TMRJ_TRJIOC_INITIAL_VALUE;
    TRJ0 = _7CFF_TMRJ_TRJ0_VALUE;
}

void boot_R_UART0_Start(void)
{
    SO0 |= _0001_SAU_CH0_DATA_OUTPUT_1;  /* output level normal */
    SOE0 |= _0001_SAU_CH0_OUTPUT_ENABLE; /* enable UART0 output */
    SS0 |= _0002_SAU_CH1_START_TRG_ON |
           _0001_SAU_CH0_START_TRG_ON; /* enable UART0 receive and transmit */
    STIF0 = 0U;                        /* clear INTST0 interrupt flag */
    SRIF0 = 0U;                        /* clear INTSR0 interrupt flag */
    /* Boot UART is polled logging only. App-only UART vectors must not run
     * before the application RAM and jump-table context are initialized. */
    STMK0 = 1U;
    SRMK0 = 1U;
}

void boot_R_UART0_Create(void)
{
    ST0 |= _0002_SAU_CH1_STOP_TRG_ON |
           _0001_SAU_CH0_STOP_TRG_ON; /* disable UART0 receive and transmit */
    STMK0 = 1U;                       /* disable INTST0 interrupt */
    STIF0 = 0U;                       /* clear INTST0 interrupt flag */
    SRMK0 = 1U;                       /* disable INTSR0 interrupt */
    SRIF0 = 0U;                       /* clear INTSR0 interrupt flag */
    /* Set INTST0 low priority */
    STPR10 = 1U;
    STPR00 = 1U;
    /* Set INTSR0 low priority */
    SRPR10 = 1U;
    SRPR00 = 1U;
    SMR00 = _0020_SAU_SMRMN_INITIALVALUE | _0000_SAU_CLOCK_SELECT_CK00 |
            _0000_SAU_TRIGGER_SOFTWARE | _0002_SAU_MODE_UART | _0000_SAU_TRANSFER_END;
    SCR00 = _8000_SAU_TRANSMISSION | _0000_SAU_PARITY_NONE | _0080_SAU_LSB | _0010_SAU_STOP_1 |
            _0007_SAU_LENGTH_8;
    SDR00 = _8800_UART0_TRANSMIT_DIVISOR;
    NFEN0 |= _01_SAU_RXD0_FILTER_ON;
    SIR01 = _0004_SAU_SIRMN_FECTMN | _0002_SAU_SIRMN_PECTMN |
            _0001_SAU_SIRMN_OVCTMN; /* clear error flag */
    SMR01 = _0020_SAU_SMRMN_INITIALVALUE | _0000_SAU_CLOCK_SELECT_CK00 | _0100_SAU_TRIGGER_RXD |
            _0000_SAU_EDGE_FALL | _0002_SAU_MODE_UART | _0000_SAU_TRANSFER_END;
    SCR01 = _4000_SAU_RECEPTION | _0000_SAU_PARITY_NONE | _0080_SAU_LSB | _0010_SAU_STOP_1 |
            _0007_SAU_LENGTH_8;
    SDR01 = _8800_UART0_RECEIVE_DIVISOR;
    SO0 |= _0001_SAU_CH0_DATA_OUTPUT_1;
    SOL0 |= _0000_SAU_CHANNEL0_NORMAL;   /* output level normal */
    SOE0 |= _0001_SAU_CH0_OUTPUT_ENABLE; /* enable UART0 output */
    /* Set RxD0 pin */
    PM1 |= 0x40U;
    /* Set TxD0 pin */
    P1 |= 0x20U;
    PM1 &= 0xDFU;
}

void boot_R_SAU0_Create(void)
{
    SAU0EN = 1U; /* supply SAU0 clock */
    NOP();
    NOP();
    NOP();
    NOP();
    SPS0 = _0001_SAU_CK00_FCLK_1 | _0010_SAU_CK01_FCLK_1;
    boot_R_UART0_Create();
}

void boot_R_CGC_Create(void)
{
    F13_BoardClockPrepare();
}

void boot_R_Systeminit(void)
{
    /* Set periperal I/O redirection */
    PIOR0 = 0x00U;
    PIOR1 = 0x00U;
    PIOR2 = 0x00U;
    PIOR3 = 0x00U;
    PIOR4 = 0x00U;
    PIOR5 = 0x00U;
    PIOR6 = 0x00U;
    PIOR7 = 0x00U;
    // R_CGC_Get_ResetSource();
    boot_R_CGC_Create();
    // boot_R_PORT_Create();
#if CAN_BOOT_DEBUG_UART
    boot_R_SAU0_Create();
#endif
    boot_R_TMR_RJ0_Create(); // for timer interrupt

    /* Set invalid memory access detection control */
    IAWCTL = 0x00U;
}

void boot_hdwinit(void)
{
    DI();
    boot_R_Systeminit();
}

/* Initialize the role selected by this project before foreground processing.
 * Preserve clock, image/handoff and peripheral/IRQ enable order when adding application
 * initialization.
 */
void hardware_init(void)
{
    bool application_valid;
    bool boot_condition;
    unsigned char clock_status;

    clock_status = F13_BoardClockStart();
    (void)clock_status;
    // const unsigned char indicator[] = "hardware_init";
    EI(); // BSP_EI();
#if CAN_BOOT_DEBUG_UART
    boot_R_UART0_Start();
#endif
    g_can_init_status = CAN_BootInit(); // CAN , P10(CTXD0) , P11(CRXD0)
    BootLogCode("(b)clk=", clock_status);
    BootLogText("(b)X1=4M CPU32 CAN500K EVDD=5V\r\n");
    BootLogCode("(b)CAN=", g_can_init_status);

    g_app_required_update = false;
    _no_init_global[RAM_FLAG_INDICATE_BOOT_APP] = BOOT_APP_RAM_OWNER_BOOT;
    BootSharedWrite32(BOOT_APP_RAM_HANDOFF_OFFSET, 0UL);
    boot_R_TMR_RJ0_Start(); // TIMER

    application_valid = (CAN_BootInstalledValid() != 0U) ? true : false;
    boot_condition = condition_check(application_valid);

    if (boot_condition)
    {
        BootSharedWrite32(BOOT_APP_RAM_UPDATE_OFFSET, 0UL);
        if (g_can_init_status == 0U)
        {
            /* Never return to an invalid application when CAN is unavailable. */
            g_app_required_update = (application_valid == false) ? true : false;
            g_update_timeout_enabled = 0U;
            return;
        }

        g_app_required_update = true;
        g_update_timeout_enabled = (application_valid != false) ? 1U : 0U;
        g_fsl_init_status = InitCodeFlash();
        if (g_fsl_init_status != FSL_OK)
        {
            BootLogText("(b)fsl:0x");
            BootLogHex8(g_fsl_init_status);
            BootLogText("\r\n");
        }

#if 0 // debug
        printf_tiny("(b)stay\r\n");
#endif
    }
    else
    {
#if 0 // debug
        printf_tiny("(b)jump\r\n");
#endif
    }
}

/* End user code. Do not edit comment generated here */
void boot_R_MAIN_UserInit(void);

/***********************************************************************************************************************
 * Function Name: loop
 * Description  : Run the Boot foreground session, then return for App handoff.
 * Arguments    : None
 * Return Value : None
 ***********************************************************************************************************************/
/* Foreground service point called repeatedly by the selected main adapter.
 * Keep protocol/application work and text logging bounded so pending traffic and deferred actions
 * can continue.
 */
void loop(void)
{

    /* Start user code. Do not edit comment generated here */
    while (1U)
    {
        if (g_app_required_update)
        {
            if (g_can_init_status == 0U)
            {
                BootLogText("(b)CAN reset\r\n");
                _reset_by_illegal_memory_access();
                continue;
            }
            i32Err = CAN_BootReceive(FLASH_AREA, g_fsl_init_status, g_update_timeout_enabled);

            if (i32Err == CAN_BOOT_RESULT_TIMEOUT)
            {
                if (g_fsl_init_status == FSL_OK)
                {
                    FSL_Close();
                }
                g_app_required_update = false;
                BootLogText("(b)timeout\r\n");
            }
            else if (i32Err < 0)
            {
                g_update_timeout_enabled = 0U;
                BootLogCode("(b)fail:0x", (unsigned char)(-i32Err));
            }
            else
            {
                BootLogText("(b)updated\r\n");

                //
                // In order to verify the checksum in the application,
                // do CHIP_RST to enter bootloader again.
                //

                FSL_Close();
                g_app_required_update = false;
                BootSharedWrite32(BOOT_APP_RAM_UPDATE_OFFSET, BOOT_APP_UPDATE_COMPLETE_SIGNATURE);
                BootLogText("(b)RST\r\n");

                _reset_by_illegal_memory_access();
            }
        }
        else
        {
            // printf_tiny("(b)break\r\n");
            break;
        }
    }
    BootLogText("(b)leave\r\n");
    DI();
    BootSharedWrite32(BOOT_APP_RAM_HANDOFF_OFFSET, BOOT_APP_HANDOFF_SIGNATURE);
    _no_init_global[RAM_FLAG_INDICATE_BOOT_APP] = BOOT_APP_RAM_OWNER_APPLICATION;
    return;

    /* End user code. Do not edit comment generated here */
}

/***********************************************************************************************************************
 * Function Name: R_MAIN_UserInit
 * Description  : This function adds user code before implementing main function.
 * Arguments    : None
 * Return Value : None
 ***********************************************************************************************************************/
void boot_R_MAIN_UserInit(void)
{
    /* Start user code. Do not edit comment generated here */
    EI();
    hardware_init();
    /* End user code. Do not edit comment generated here */
}

/* Start user code for adding. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
