//*****************************************************************************
//
// ei_app_minimal.c - Minimal EtherNet/IP Application Wrapper for TM4C
//
// This file implements a minimal, TM4C-adapted version of the EtherNet/IP
// application initialization layer. It provides a simple interface for
// bare-metal integration into the enet_lwip project.
//
// This is a placeholder implementation that demonstrates the integration
// points. Full functionality requires the EI_API library.
//
// Copyright (c) 2025 Texas Instruments Incorporated.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "utils/uartstdio.h"

#include "ei_tm4c_config.h"
#include "ei_tm4c_netif.h"
#include "ei_tm4c_board.h"
#include "ei_app_minimal.h"

//*****************************************************************************
//
// Module-level variables
//
//*****************************************************************************

// Application initialization flag
static bool g_bAppInitialized = false;

// Current device configuration
static EI_AppDeviceConfig_t g_AppDeviceConfig = {
    .ui16VendorID = EI_DEVICE_VENDOR_ID,
    .ui16DeviceType = EI_DEVICE_TYPE,
    .ui16ProductCode = EI_DEVICE_PRODUCT_CODE,
    .ui32SerialNumber = EI_DEVICE_SERIAL_NUMBER,
    .ui8RevisionMajor = EI_DEVICE_REVISION_MAJOR,
    .ui8RevisionMinor = EI_DEVICE_REVISION_MINOR,
    .ui8FWRevisionMajor = EI_DEVICE_FIRMWARE_MAJOR,
    .ui8FWRevisionMinor = EI_DEVICE_FIRMWARE_MINOR,
    .pcDeviceName = EI_DEVICE_NAME,
};

// Application status string
static char g_pcAppStatusString[128];

//*****************************************************************************
//
// Helper Functions
//
//*****************************************************************************

/**
 *  \brief Update application status string
 */
static void AppUpdateStatusString(void)
{
    const char *pcNetifStatus = "DOWN";
    bool bIsUp = false;
    bool bHasIP = false;
    
    if (EI_TM4C_NetifIsUp(&bIsUp) == EI_NETIF_STATUS_OK) {
        if (bIsUp) {
            pcNetifStatus = "UP";
            if (EI_TM4C_NetifHasIP(&bHasIP) == EI_NETIF_STATUS_OK) {
                if (bHasIP) {
                    pcNetifStatus = "READY";
                }
            }
        }
    }
    
    usprintf(g_pcAppStatusString, 
             "[EI] %s - Net: %s - SN: 0x%08X",
             g_AppDeviceConfig.pcDeviceName,
             pcNetifStatus,
             g_AppDeviceConfig.ui32SerialNumber);
}

//*****************************************************************************
//
// Public API Implementation
//
//*****************************************************************************

/**
 *  \brief Initialize EtherNet/IP application
 */
EI_AppStatus_t EI_APP_Init(const EI_AppDeviceConfig_t *pDevConfig)
{
    EI_NetifStatus_t netifStatus;
    EI_BoardStatus_t boardStatus;
    
    // Check if already initialized
    if (g_bAppInitialized) {
        EI_LOG("Application already initialized");
        return EI_APP_STATUS_ERR_ALREADY_INIT;
    }
    
    // Use provided config or defaults
    if (pDevConfig != NULL) {
        memcpy(&g_AppDeviceConfig, pDevConfig, sizeof(EI_AppDeviceConfig_t));
    }
    
    // Initialize board abstraction layer
    boardStatus = EI_TM4C_BoardInit();
    if (boardStatus != EI_BOARD_STATUS_OK) {
        EI_LOG_ERROR("Board initialization failed: %d", boardStatus);
        return EI_APP_STATUS_ERR_INVALID_PARAM;
    }
    
    EI_LOG("Board abstraction layer initialized");
    
    // Initialize network interface adapter
    netifStatus = EI_TM4C_NetifInit();
    if (netifStatus != EI_NETIF_STATUS_OK) {
        EI_LOG_ERROR("Network interface initialization failed: %d", netifStatus);
        return EI_APP_STATUS_ERR_NETIF_FAILED;
    }
    
    EI_LOG("Network interface initialized");
    
    // Log device information
    EI_LOG("Device: %s", g_AppDeviceConfig.pcDeviceName);
    EI_LOG("Vendor ID: 0x%04X", g_AppDeviceConfig.ui16VendorID);
    EI_LOG("Device Type: 0x%04X", g_AppDeviceConfig.ui16DeviceType);
    EI_LOG("Product Code: 0x%04X", g_AppDeviceConfig.ui16ProductCode);
    EI_LOG("Serial Number: 0x%08X", g_AppDeviceConfig.ui32SerialNumber);
    EI_LOG("HW Rev: %u.%u", g_AppDeviceConfig.ui8RevisionMajor, 
           g_AppDeviceConfig.ui8RevisionMinor);
    EI_LOG("FW Rev: %u.%u", g_AppDeviceConfig.ui8FWRevisionMajor, 
           g_AppDeviceConfig.ui8FWRevisionMinor);
    
    // Show network status
    EI_Netif_Info_t netifInfo;
    if (EI_TM4C_NetifGetInfo(&netifInfo) == EI_NETIF_STATUS_OK) {
        EI_LOG("MAC: %02x:%02x:%02x:%02x:%02x:%02x",
               netifInfo.ui8MAC[0], netifInfo.ui8MAC[1],
               netifInfo.ui8MAC[2], netifInfo.ui8MAC[3],
               netifInfo.ui8MAC[4], netifInfo.ui8MAC[5]);
        
        if (netifInfo.bHasIP) {
            uint32_t ui32IP = netifInfo.ui32IP;
            EI_LOG("IP: %d.%d.%d.%d",
                   (ui32IP >> 0) & 0xFF,
                   (ui32IP >> 8) & 0xFF,
                   (ui32IP >> 16) & 0xFF,
                   (ui32IP >> 24) & 0xFF);
        } else {
            EI_LOG("Waiting for IP address...");
        }
    }
    
    // Set LED to indicate initialization
    EI_TM4C_LEDSetStatus(EI_LED_BLINK);
    
    // Mark as initialized
    g_bAppInitialized = true;
    
    EI_LOG("EtherNet/IP application initialized successfully");
    
    return EI_APP_STATUS_OK;
}

/**
 *  \brief Run EtherNet/IP application processing
 */
void EI_APP_Process(void)
{
    if (!g_bAppInitialized) {
        return;  // Not initialized yet
    }
    
    // Update board (e.g., LED blinking)
    EI_TM4C_BoardProcess();
    
    // Update status string periodically
    static uint32_t ui32UpdateCounter = 0;
    ui32UpdateCounter++;
    
    if (ui32UpdateCounter >= 100) {  // Update every 100 calls (~1 second at 100 Hz)
        ui32UpdateCounter = 0;
        AppUpdateStatusString();
        
        // Check network status
        bool bHasIP = false;
        if (EI_TM4C_NetifHasIP(&bHasIP) == EI_NETIF_STATUS_OK) {
            if (bHasIP) {
                // Network is ready - could transition LED to solid ON
                // For now, keep blinking to show activity
            }
        }
    }
    
    // TODO: Add EI_API stack processing here when library is integrated
    // For now, this is just a placeholder
}

/**
 *  \brief Shutdown EtherNet/IP application
 */
EI_AppStatus_t EI_APP_Shutdown(void)
{
    if (!g_bAppInitialized) {
        return EI_APP_STATUS_OK;  // Already shutdown
    }
    
    EI_LOG("Shutting down EtherNet/IP application");
    
    // Turn off LED
    EI_TM4C_LEDSetStatus(EI_LED_OFF);
    
    // TODO: Clean up EI_API resources when integrated
    
    g_bAppInitialized = false;
    
    return EI_APP_STATUS_OK;
}

/**
 *  \brief Get application status string
 */
const char* EI_APP_GetStatusString(void)
{
    if (!g_bAppInitialized) {
        return "[EI] Not initialized";
    }
    
    return g_pcAppStatusString;
}

/**
 *  \brief Check if application is initialized
 */
bool EI_APP_IsInitialized(void)
{
    return g_bAppInitialized;
}

/**
 *  \brief Set device name
 */
EI_AppStatus_t EI_APP_SetDeviceName(const char *pcName)
{
    if (pcName == NULL) {
        return EI_APP_STATUS_ERR_INVALID_PARAM;
    }
    
    // Check length (EtherNet/IP limit is typically 32 characters)
    if (strlen(pcName) > 32) {
        EI_LOG_ERROR("Device name too long (max 32 characters)");
        return EI_APP_STATUS_ERR_INVALID_PARAM;
    }
    
    g_AppDeviceConfig.pcDeviceName = pcName;
    
    EI_LOG("Device name updated to: %s", pcName);
    
    return EI_APP_STATUS_OK;
}

/**
 *  \brief Get device name
 */
const char* EI_APP_GetDeviceName(void)
{
    return g_AppDeviceConfig.pcDeviceName;
}

/**
 *  \brief Get device serial number
 */
uint32_t EI_APP_GetSerialNumber(void)
{
    return g_AppDeviceConfig.ui32SerialNumber;
}
