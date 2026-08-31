/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the PSOC Control C1 MCU: WDT Prewarning example
*              for ModusToolbox.
*              The watchdog timer needs to be fed every second for proper servicing.
*              A prewarning alarm is triggered if the watchdog is not serviced within
*              the serving window.
*              After reset, the MCU checks the reason for the last reset.
*              User LED1 blinks at a faster rate after a watchdog reset.
*
* Related Document: See README.md
*
*******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

#include "cybsp.h"
#include "cy_utils.h"
#include <stdio.h>
#include "cy_retarget_io.h"

/*******************************************************************************
* Macros
*******************************************************************************/

#define COUNTS_DELAY                      (500000U)
#define WDT_Prewarning_Interrupt_Handler  IRQ1_Handler
#define INTERRUPT_PRIORITY_NODE_ID        IRQ1_IRQn

#define TICKS_PER_SECOND                  (1000U)
#define TICKS_WAIT                        (1000U)
#define MAX_NUM_FEEDS                     (10U)
#define WDT_WINDOW_LOWER_BOUND            (0x7530U)
#define WDT_WINDOW_UPPER_BOUND            (0x88B8U)

/* Define macro to enable/disable printing of debug messages */
#define ENABLE_DEBUG_PRINT                (1)

/* Define macro to set the loop count before printing debug messages */
#if ENABLE_DEBUG_PRINT
#define DEBUG_LOOP_COUNT_MAX                    2
static bool WDT_SERVICE_DONE = false;
#endif

/*******************************************************************************
* Function Name: SysTick_Handler
********************************************************************************
* Summary:
* This is the interrupt handler function for the System Tick interrupt.
*
* Parameters:
*  none
*
* Return:
*  void
*
*******************************************************************************/
void SysTick_Handler(void)
{
    static uint32_t ticks = 0;
    static uint32_t feeds = 0;

    ticks++;
    /* Feed the watchdog 10 times inside the SysTick ISR */
    if (ticks == TICKS_WAIT && feeds < MAX_NUM_FEEDS)
    {
        /*User LED1 blinks 5 times*/
        Cy_GPIO_ToggleOutput(CYBSP_USER_LED_PORT, CYBSP_USER_LED1_PIN);
        /*Service watchdog when count value of watchdog timer is between lower and upper window bounds*/
        Cy_WDT_Service();
        #if ENABLE_DEBUG_PRINT
        WDT_SERVICE_DONE = true;
        #endif

        ticks = 0;

        feeds++;
    }
}

/*******************************************************************************
 * Function Name: WDT_Prewarning_Interrupt_Handler
 ********************************************************************************
 * Summary:
 * This is the interrupt handler function for the watchdog prewarning interrupt.
 *
 * Parameters:
 *  none
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void WDT_Prewarning_Interrupt_Handler(void)
{
    Cy_SCU_IRQHandler((uint32_t)INTERRUPT_PRIORITY_NODE_ID);
}


/*******************************************************************************
* Function Name: Watchdog_Handler
********************************************************************************
* Summary:
* This is the callback function for the watchdog prewarning interrupt.
*
* Parameters:
*  none
*
* Return:
*  void
*
*******************************************************************************/
void Watchdog_Handler(void)
{
    /* Toggle User LED2 due to watchdog prewarning */
    Cy_GPIO_ToggleOutput(CYBSP_USER_LED_PORT, CYBSP_USER_LED2_PIN);
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function.
* The example feeds the watchdog 10 times (see blinking User LED1).
* The first crossing of the upper bound triggers an alarm.
* Since prewarning is enabled, the alarm signal is routed as a request to the SCU,
* where it is promoted to an interrupt event.
* In the prewarning interrupt handler, User LED2 is toggled.
* Only the next overflow results in a reset request.
* After the reset the device checks the reason for the last reset.
* If it was due to a failure to feed the WDT the User LED1 will blink rapidly.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    #if ENABLE_DEBUG_PRINT
    /* Assign false to disable printing of debug messages */
    static volatile bool debug_printf = true;
    #endif

    /*Initialize the device and board peripherals*/
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize printf retarget */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_HW);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    #if ENABLE_DEBUG_PRINT
    printf("Initialization done\r\n");
    #endif


    /*Check for the value representing the reason for device reset*/
    if ((Cy_SCU_RESET_GetDeviceResetReason() & CY_SCU_RESET_REASON_WATCHDOG) != 0)
    {

        #if ENABLE_DEBUG_PRINT
        printf("Device reset\r\n");
        #endif

        /*Clear system reset status*/
        Cy_SCU_RESET_ClearDeviceResetReason();
        while(1)
        {
            /*Toggle User LED1 faster due to watchdog reset*/
            Cy_GPIO_ToggleOutput(CYBSP_USER_LED_PORT, CYBSP_USER_LED1_PIN);
            for (int i = 0; i < COUNTS_DELAY; ++i)
            {
                __NOP();
            }
        }
    }

    /*Clear system reset status*/
    Cy_SCU_RESET_ClearDeviceResetReason();

    /* Set the WDT window bounds. */
    Cy_WDT_SetWindowBounds(WDT_WINDOW_LOWER_BOUND, WDT_WINDOW_UPPER_BOUND);

    /*Enable Interrupt*/
    NVIC_EnableIRQ(INTERRUPT_PRIORITY_NODE_ID);
   
    /*Start the watchdog timer*/
    Cy_WDT_Start();
    #if ENABLE_DEBUG_PRINT
    printf("WDT Started\r\n");
    #endif

    /*Feed the watchdog periodically every 1s*/
    SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);

    #if ENABLE_DEBUG_PRINT
    debug_printf = false;
    #endif

    while(1)
    {
     /* Infinite loop */
        #if ENABLE_DEBUG_PRINT
        if(WDT_SERVICE_DONE && !debug_printf)
        {
            printf("WDT Serviced\r\n");
            debug_printf = true;
        }
        #endif

    }
}

/* [] END OF FILE */
