//*****************************************************************************
//
// ei_app_minimal.h - Minimal EtherNet/IP Application Wrapper for TM4C
//
// This header provides a minimal, TM4C-adapted version of the EtherNet/IP
// application initialization layer. It removes PRU dependencies and RTOS
// requirements, providing a simple API for integration into the enet_lwip
// bare-metal application.
//
// Copyright (c) 2025 Texas Instruments Incorporated.
//
//*****************************************************************************

#ifndef EI_APP_MINIMAL_H
#define EI_APP_MINIMAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//*****************************************************************************
//
// Application Status Codes
//
//*****************************************************************************

typedef enum {
    EI_APP_STATUS_OK = 0,                    // Success
    EI_APP_STATUS_ERR_ALREADY_INIT = 1,      // Already initialized
    EI_APP_STATUS_ERR_NETIF_FAILED = 2,      // Network interface not ready
    EI_APP_STATUS_ERR_INVALID_PARAM = 3,     // Invalid parameter
} EI_AppStatus_t;

//*****************************************************************************
//
// Device Configuration Parameters
//
// These parameters define the EtherNet/IP device identity and capabilities.
//
//*****************************************************************************

typedef struct {
    uint16_t ui16VendorID;                   // Vendor ID (0x0186 for TI)
    uint16_t ui16DeviceType;                 // Device type
    uint16_t ui16ProductCode;                // Product code
    uint32_t ui32SerialNumber;               // Serial number
    uint8_t ui8RevisionMajor;                // Hardware revision major
    uint8_t ui8RevisionMinor;                // Hardware revision minor
    uint8_t ui8FWRevisionMajor;              // Firmware revision major
    uint8_t ui8FWRevisionMinor;              // Firmware revision minor
    const char *pcDeviceName;                // Device name string (<=32 chars)
} EI_AppDeviceConfig_t;

//*****************************************************************************
//
// Application Initialization and Control API
//
//*****************************************************************************

/**
 *  \brief Initialize EtherNet/IP application
 *
 *  This function initializes the EtherNet/IP stack for TM4C.
 *  Must be called AFTER:
 *  1. System clock and GPIO are configured
 *  2. lwIPInit() has completed
 *  3. Network interface is up
 *
 *  \param  pDevConfig - Pointer to device configuration structure,
 *                       or NULL to use defaults from ei_tm4c_config.h
 *
 *  \return EI_AppStatus_t
 *    \retval EI_APP_STATUS_OK - Initialization successful
 *    \retval EI_APP_STATUS_ERR_NETIF_FAILED - Network interface not ready
 */
EI_AppStatus_t EI_APP_Init(const EI_AppDeviceConfig_t *pDevConfig);

/**
 *  \brief Run EtherNet/IP application processing
 *
 *  This function should be called periodically from the main loop
 *  (e.g., after every LED blink cycle) to process EtherNet/IP stack
 *  operations.
 *
 *  \param  None
 *
 *  \return None
 */
void EI_APP_Process(void);

/**
 *  \brief Shutdown EtherNet/IP application
 *
 *  Cleanly shuts down the EtherNet/IP stack. Useful before device reset.
 *
 *  \param  None
 *
 *  \return EI_AppStatus_t
 *    \retval EI_APP_STATUS_OK - Shutdown successful
 */
EI_AppStatus_t EI_APP_Shutdown(void);

/**
 *  \brief Get application status
 *
 *  Returns a human-readable status string about the EtherNet/IP application.
 *
 *  \param  None
 *
 *  \return const char* - Pointer to status string
 */
const char* EI_APP_GetStatusString(void);

/**
 *  \brief Check if application is initialized
 *
 *  \param  None
 *
 *  \return bool - true if initialized, false otherwise
 */
bool EI_APP_IsInitialized(void);

//*****************************************************************************
//
// Device Attribute Access API
//
// These functions provide access to device attributes that may be
// modified at runtime.
//
//*****************************************************************************

/**
 *  \brief Set device name
 *
 *  Updates the device name attribute. Name must be <= 32 characters.
 *
 *  \param  pcName - Pointer to device name string
 *
 *  \return EI_AppStatus_t
 *    \retval EI_APP_STATUS_OK - Name updated
 *    \retval EI_APP_STATUS_ERR_INVALID_PARAM - Name too long or NULL
 */
EI_AppStatus_t EI_APP_SetDeviceName(const char *pcName);

/**
 *  \brief Get device name
 *
 *  Retrieves the current device name.
 *
 *  \param  None
 *
 *  \return const char* - Pointer to device name string
 */
const char* EI_APP_GetDeviceName(void);

/**
 *  \brief Get device serial number
 *
 *  \param  None
 *
 *  \return uint32_t - Device serial number
 */
uint32_t EI_APP_GetSerialNumber(void);

#ifdef __cplusplus
}
#endif

#endif // EI_APP_MINIMAL_H
