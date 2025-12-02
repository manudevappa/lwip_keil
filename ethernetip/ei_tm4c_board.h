//*****************************************************************************
//
// ei_tm4c_board.h - EtherNet/IP Board Abstraction Layer for TM4C
//
// This header defines board-specific functions for TM4C that EtherNet/IP
// may need, such as LED control, GPIO operations, and other hardware
// abstractions.
//
// Copyright (c) 2025 Texas Instruments Incorporated.
//
//*****************************************************************************

#ifndef EI_TM4C_BOARD_H
#define EI_TM4C_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//*****************************************************************************
//
// Board Status Codes
//
//*****************************************************************************

typedef enum {
    EI_BOARD_STATUS_OK = 0,                  // Success
    EI_BOARD_STATUS_ERR_INVALID_PARAM = 1,   // Invalid parameter
    EI_BOARD_STATUS_ERR_NOT_INITIALIZED = 2, // Board not initialized
} EI_BoardStatus_t;

//*****************************************************************************
//
// LED Status Enumeration
//
//*****************************************************************************

typedef enum {
    EI_LED_OFF = 0,      // LED is off
    EI_LED_ON = 1,       // LED is on
    EI_LED_BLINK = 2,    // LED is blinking
} EI_LEDStatus_t;

//*****************************************************************************
//
// Board Initialization and Status API
//
//*****************************************************************************

/**
 *  \brief Initialize board abstraction layer
 *
 *  This function initializes the board-specific abstraction layer.
 *  It should be called once during startup, AFTER system clocks and
 *  basic GPIO have been initialized.
 *
 *  \param  None
 *
 *  \return EI_BoardStatus_t
 *    \retval EI_BOARD_STATUS_OK - Initialization successful
 */
EI_BoardStatus_t EI_TM4C_BoardInit(void);

/**
 *  \brief Get board status string
 *
 *  Returns a human-readable status string about the board.
 *  Useful for debug output.
 *
 *  \param  None
 *
 *  \return const char* - Pointer to status string
 */
const char* EI_TM4C_BoardGetStatusString(void);

//*****************************************************************************
//
// LED Control API
//
// The EK-TM4C1294XL has one user-controllable LED on Port N, Pin 1.
// The original enet_lwip example uses this for blinking in the main loop.
// EtherNet/IP can override this to show device status.
//
//*****************************************************************************

/**
 *  \brief Set LED state
 *
 *  Controls the state of the status LED.
 *
 *  \param  eLEDStatus - Desired LED state (ON, OFF, or BLINK)
 *
 *  \return EI_BoardStatus_t
 *    \retval EI_BOARD_STATUS_OK - LED state set successfully
 *    \retval EI_BOARD_STATUS_ERR_INVALID_PARAM - Invalid LED state
 */
EI_BoardStatus_t EI_TM4C_LEDSetStatus(EI_LEDStatus_t eLEDStatus);

/**
 *  \brief Get current LED state
 *
 *  Retrieves the current state of the status LED.
 *
 *  \param  peLEDStatus - Pointer to store current LED state
 *
 *  \return EI_BoardStatus_t
 *    \retval EI_BOARD_STATUS_OK - LED state retrieved successfully
 *    \retval EI_BOARD_STATUS_ERR_INVALID_PARAM - Invalid pointer
 */
EI_BoardStatus_t EI_TM4C_LEDGetStatus(EI_LEDStatus_t *peLEDStatus);

/**
 *  \brief Toggle LED state
 *
 *  Toggles the LED between ON and OFF (ignores BLINK state).
 *
 *  \param  None
 *
 *  \return EI_BoardStatus_t
 *    \retval EI_BOARD_STATUS_OK - LED toggled successfully
 */
EI_BoardStatus_t EI_TM4C_LEDToggle(void);

/**
 *  \brief Set LED blink rate
 *
 *  If LED is set to BLINK mode, this function controls the blink rate.
 *
 *  \param  ui32BlinkRateHz - Blink rate in Hz (1-10 recommended)
 *
 *  \return EI_BoardStatus_t
 *    \retval EI_BOARD_STATUS_OK - Blink rate set successfully
 *    \retval EI_BOARD_STATUS_ERR_INVALID_PARAM - Invalid blink rate
 */
EI_BoardStatus_t EI_TM4C_LEDSetBlinkRate(uint32_t ui32BlinkRateHz);

//*****************************************************************************
//
// System Information API
//
//*****************************************************************************

/**
 *  \brief Get system clock frequency
 *
 *  Returns the current system clock frequency in Hz.
 *
 *  \param  None
 *
 *  \return uint32_t - System clock frequency in Hz
 */
uint32_t EI_TM4C_GetSysClockHz(void);

/**
 *  \brief Get board name
 *
 *  Returns the name of the EtherNet/IP device board.
 *
 *  \param  None
 *
 *  \return const char* - Pointer to board name string
 */
const char* EI_TM4C_GetBoardName(void);

/**
 *  \brief Get board revision
 *
 *  Returns the hardware revision string.
 *
 *  \param  None
 *
 *  \return const char* - Pointer to revision string
 */
const char* EI_TM4C_GetBoardRevision(void);

#ifdef __cplusplus
}
#endif

#endif // EI_TM4C_BOARD_H
