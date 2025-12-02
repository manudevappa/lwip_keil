//*****************************************************************************
//
// ei_tm4c_board.c - EtherNet/IP Board Abstraction Layer for TM4C
//
// This file implements board-specific functions for TM4C that EtherNet/IP
// may need, providing a clean interface for hardware control.
//
// Copyright (c) 2025 Texas Instruments Incorporated.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"

#include "ei_tm4c_config.h"
#include "ei_tm4c_board.h"

//*****************************************************************************
//
// Module-level variables
//
//*****************************************************************************

// Board initialization flag
static bool g_bBoardInitialized = false;

// Current LED state
static EI_LEDStatus_t g_eLEDStatus = EI_LED_OFF;

// LED blink rate in Hz (used for BLINK mode)
static uint32_t g_ui32BlinkRateHz = 2;

// LED blink counter (for timing blink pattern)
static uint32_t g_ui32BlinkCounter = 0;

// LED blink threshold (cycles per blink transition)
static uint32_t g_ui32BlinkThreshold = 50;

//*****************************************************************************
//
// External declarations
//
//*****************************************************************************

extern uint32_t g_ui32SysClock;

//*****************************************************************************
//
// Helper Functions
//
//*****************************************************************************

/**
 *  \brief Update LED blink pattern
 *
 *  This function should be called periodically (e.g., from SysTick handler)
 *  to update the LED blink pattern.
 */
static void BoardLEDUpdateBlink(void)
{
    if (g_eLEDStatus != EI_LED_BLINK) {
        return;  // Not in blink mode
    }
    
    g_ui32BlinkCounter++;
    
    if (g_ui32BlinkCounter >= g_ui32BlinkThreshold) {
        g_ui32BlinkCounter = 0;
        
        // Toggle the LED
        uint32_t ui32CurrentState = MAP_GPIOPinRead(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN);
        
        if (ui32CurrentState) {
            // LED is on, turn it off
            MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, 0);
        } else {
            // LED is off, turn it on
            MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, EI_CONFIG_LED_PIN);
        }
    }
}

//*****************************************************************************
//
// Public API Implementation
//
//*****************************************************************************

/**
 *  \brief Initialize board abstraction layer
 */
EI_BoardStatus_t EI_TM4C_BoardInit(void)
{
    if (g_bBoardInitialized) {
        return EI_BOARD_STATUS_OK;
    }
    
    // GPIO should already be initialized by PinoutSet() in enet_lwip.c
    // The LED pin (Port N, Pin 1) should already be configured as output
    
    // Verify by checking if we can read the GPIO
    // (This is a simple sanity check)
    volatile uint32_t ui32Test = MAP_GPIOPinRead(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN);
    (void)ui32Test;  // Suppress unused variable warning
    
    // Initialize LED to OFF
    MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, 0);
    g_eLEDStatus = EI_LED_OFF;
    
    // Calculate blink threshold based on system clock
    // For a 2 Hz blink rate with 100 Hz SysTick:
    // Threshold = (SYSTICKHZ / 2) / 2 for 50% duty cycle
    // = (100 / 2) / 2 = 25, but we use 50 for slower visual effect
    if (g_ui32SysClock > 0) {
        g_ui32BlinkThreshold = (100 / g_ui32BlinkRateHz) / 2;
        if (g_ui32BlinkThreshold == 0) {
            g_ui32BlinkThreshold = 1;
        }
    }
    
    g_bBoardInitialized = true;
    
    EI_LOG("Board abstraction layer initialized");
    
    return EI_BOARD_STATUS_OK;
}

/**
 *  \brief Get board status string
 */
const char* EI_TM4C_BoardGetStatusString(void)
{
    static char pcStatusString[64];
    
    const char *pcLEDStatus = "OFF";
    if (g_eLEDStatus == EI_LED_ON) {
        pcLEDStatus = "ON";
    } else if (g_eLEDStatus == EI_LED_BLINK) {
        pcLEDStatus = "BLINKING";
    }
    
    usprintf(pcStatusString, "EK-TM4C1294XL EtherNet/IP Device - LED: %s, Clock: %u MHz",
             pcLEDStatus, g_ui32SysClock / 1000000);
    
    return pcStatusString;
}

/**
 *  \brief Set LED state
 */
EI_BoardStatus_t EI_TM4C_LEDSetStatus(EI_LEDStatus_t eLEDStatus)
{
    if (eLEDStatus > EI_LED_BLINK) {
        return EI_BOARD_STATUS_ERR_INVALID_PARAM;
    }
    
    g_eLEDStatus = eLEDStatus;
    g_ui32BlinkCounter = 0;  // Reset blink counter
    
    switch (eLEDStatus) {
        case EI_LED_OFF:
            MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, 0);
            EI_LOG("LED set to OFF");
            break;
            
        case EI_LED_ON:
            MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, EI_CONFIG_LED_PIN);
            EI_LOG("LED set to ON");
            break;
            
        case EI_LED_BLINK:
            EI_LOG("LED set to BLINK at %u Hz", g_ui32BlinkRateHz);
            // Will be managed by timer/SysTick handler
            break;
            
        default:
            return EI_BOARD_STATUS_ERR_INVALID_PARAM;
    }
    
    return EI_BOARD_STATUS_OK;
}

/**
 *  \brief Get current LED state
 */
EI_BoardStatus_t EI_TM4C_LEDGetStatus(EI_LEDStatus_t *peLEDStatus)
{
    if (peLEDStatus == NULL) {
        return EI_BOARD_STATUS_ERR_INVALID_PARAM;
    }
    
    *peLEDStatus = g_eLEDStatus;
    
    return EI_BOARD_STATUS_OK;
}

/**
 *  \brief Toggle LED state
 */
EI_BoardStatus_t EI_TM4C_LEDToggle(void)
{
    uint32_t ui32CurrentState = MAP_GPIOPinRead(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN);
    
    if (ui32CurrentState) {
        // LED is on, turn it off
        MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, 0);
    } else {
        // LED is off, turn it on
        MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, EI_CONFIG_LED_PIN);
    }
    
    return EI_BOARD_STATUS_OK;
}

/**
 *  \brief Set LED blink rate
 */
EI_BoardStatus_t EI_TM4C_LEDSetBlinkRate(uint32_t ui32BlinkRateHz)
{
    if (ui32BlinkRateHz < 1 || ui32BlinkRateHz > 10) {
        return EI_BOARD_STATUS_ERR_INVALID_PARAM;
    }
    
    g_ui32BlinkRateHz = ui32BlinkRateHz;
    
    // Recalculate blink threshold
    // Assuming 100 Hz SysTick (from enet_lwip.c SYSTICKHZ = 100)
    g_ui32BlinkThreshold = (100 / ui32BlinkRateHz) / 2;
    if (g_ui32BlinkThreshold == 0) {
        g_ui32BlinkThreshold = 1;
    }
    
    g_ui32BlinkCounter = 0;
    
    EI_LOG("LED blink rate set to %u Hz", ui32BlinkRateHz);
    
    return EI_BOARD_STATUS_OK;
}

/**
 *  \brief Get system clock frequency
 */
uint32_t EI_TM4C_GetSysClockHz(void)
{
    return g_ui32SysClock;
}

/**
 *  \brief Get board name
 */
const char* EI_TM4C_GetBoardName(void)
{
    return "EK-TM4C1294XL";
}

/**
 *  \brief Get board revision
 */
const char* EI_TM4C_GetBoardRevision(void)
{
    return "1.0";
}

//*****************************************************************************
//
// Timer Callback (to be called from SysTick handler in enet_lwip.c)
//
// This function updates the LED blink pattern and should be called
// periodically (typically from SysTick at 100 Hz).
//
//*****************************************************************************

/**
 *  \brief Process board periodic updates
 *
 *  This function should be called from the SysTick interrupt handler
 *  or main loop to update LED blinking and other time-based operations.
 *
 *  \param  None
 *
 *  \return None
 */
void EI_TM4C_BoardProcess(void)
{
    if (!g_bBoardInitialized) {
        return;
    }
    
    BoardLEDUpdateBlink();
}
