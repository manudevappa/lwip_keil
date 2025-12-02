//*****************************************************************************
//
// ei_tm4c_netif.c - EtherNet/IP lwIP Network Interface Adapter for TM4C
//
// This file implements the network interface adaptation layer that hooks
// EtherNet/IP into the existing lwIP network stack on TM4C.
//
// Key Points:
// - Does NOT reinitialize the MAC or PHY (already done by enet_lwip.c)
// - Does NOT create a new network interface (reuses lwIP's netif)
// - Provides a clean API for EtherNet/IP to query network state
// - Handles byte order conversions and validation
//
// Copyright (c) 2025 Texas Instruments Incorporated.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "utils/lwiplib.h"
#include "utils/uartstdio.h"

#include "ei_tm4c_config.h"
#include "ei_tm4c_netif.h"

//*****************************************************************************
//
// Macros for debug output
//
//*****************************************************************************

#if EI_CONFIG_DEBUG_OUTPUT
#define NETIF_DEBUG_PRINT(fmt, ...) \
    UARTprintf("[EI_NETIF] " fmt "\n", ##__VA_ARGS__)
#else
#define NETIF_DEBUG_PRINT(fmt, ...) \
    do { } while(0)
#endif

//*****************************************************************************
//
// Module-level variables
//
//*****************************************************************************

// Pointer to the lwIP network interface
// This will be obtained from lwIP's netif_list
static struct netif *g_pNetif = NULL;

// Initialization flag to prevent duplicate initialization
static bool g_bInitialized = false;

//*****************************************************************************
//
// External declarations
//
// These are defined in enet_lwip.c or lwiplib
//
//*****************************************************************************

extern uint32_t g_ui32SysClock;
extern uint32_t g_ui32IPAddress;

//*****************************************************************************
//
// Helper Functions
//
//*****************************************************************************

/**
 *  \brief Get lwIP network interface
 *
 *  Retrieves the network interface from lwIP's netif list.
 *  The interface should have been created by lwIPInit().
 *
 *  \return struct netif* - Pointer to network interface, or NULL if not found
 */
static struct netif* NetifGetLwipInterface(void)
{
    // lwIP maintains a list of network interfaces in netif_list
    // We want the first (and typically only) interface
    // Note: netif_list is defined by lwIP core
    extern struct netif *netif_list;
    
    if (netif_list != NULL) {
        return netif_list;
    }
    
    return NULL;
}

/**
 *  \brief Convert IP address to string for debug output
 *
 *  \param ui32IP - IP address (network byte order)
 *  \return char* - Pointer to static buffer containing IP string
 */
static const char* NetifIPToString(uint32_t ui32IP)
{
    static char pcIPString[16];
    
    usprintf(pcIPString, "%d.%d.%d.%d",
             (ui32IP >>  0) & 0xFF,
             (ui32IP >>  8) & 0xFF,
             (ui32IP >> 16) & 0xFF,
             (ui32IP >> 24) & 0xFF);
    
    return pcIPString;
}

//*****************************************************************************
//
// Public API Implementation
//
//*****************************************************************************

/**
 *  \brief Initialize EtherNet/IP network interface
 */
EI_NetifStatus_t EI_TM4C_NetifInit(void)
{
    // Prevent duplicate initialization
    if (g_bInitialized) {
        NETIF_DEBUG_PRINT("Already initialized");
        return EI_NETIF_STATUS_OK;
    }
    
    // Get the lwIP network interface
    g_pNetif = NetifGetLwipInterface();
    
    if (g_pNetif == NULL) {
        EI_LOG_ERROR("lwIP network interface not found");
        return EI_NETIF_STATUS_ERR_NO_NETIF;
    }
    
    // Verify the interface is up
    if (!netif_is_up(g_pNetif)) {
        EI_LOG_WARN("Network interface is not up yet");
        // Don't fail - interface might not be up during early initialization
    }
    
    // Log the MAC address
    NETIF_DEBUG_PRINT("MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                      g_pNetif->hwaddr[0], g_pNetif->hwaddr[1],
                      g_pNetif->hwaddr[2], g_pNetif->hwaddr[3],
                      g_pNetif->hwaddr[4], g_pNetif->hwaddr[5]);
    
    // Log IP information if available
    if (g_pNetif->ip_addr.addr != 0) {
        NETIF_DEBUG_PRINT("IP: %s", NetifIPToString(g_pNetif->ip_addr.addr));
    } else {
        NETIF_DEBUG_PRINT("Waiting for IP address assignment");
    }
    
    g_bInitialized = true;
    NETIF_DEBUG_PRINT("Initialization complete");
    
    return EI_NETIF_STATUS_OK;
}

/**
 *  \brief Get current network interface information
 */
EI_NetifStatus_t EI_TM4C_NetifGetInfo(EI_Netif_Info_t *pInfo)
{
    if (pInfo == NULL) {
        return EI_NETIF_STATUS_ERR_INVALID_PARAM;
    }
    
    if (g_pNetif == NULL) {
        g_pNetif = NetifGetLwipInterface();
        if (g_pNetif == NULL) {
            return EI_NETIF_STATUS_ERR_NO_NETIF;
        }
    }
    
    // Copy MAC address
    memcpy(pInfo->ui8MAC, g_pNetif->hwaddr, 6);
    
    // Copy IP address (lwIP stores in host byte order internally, but we convert)
    // Note: lwIP's ip_addr.addr is already in the correct format for our purposes
    pInfo->ui32IP = g_pNetif->ip_addr.addr;
    pInfo->ui32Netmask = g_pNetif->netmask.addr;
    pInfo->ui32Gateway = g_pNetif->gw.addr;
    
    // Status flags
    pInfo->bIsUp = netif_is_up(g_pNetif);
    pInfo->bHasIP = (pInfo->ui32IP != 0 && pInfo->ui32IP != 0xFFFFFFFF);
    pInfo->bUseDHCP = 1;  // TM4C enet_lwip always uses DHCP/AutoIP
    
    return EI_NETIF_STATUS_OK;
}

/**
 *  \brief Get MAC address
 */
EI_NetifStatus_t EI_TM4C_NetifGetMAC(uint8_t *pui8MAC)
{
    if (pui8MAC == NULL) {
        return EI_NETIF_STATUS_ERR_INVALID_PARAM;
    }
    
    if (g_pNetif == NULL) {
        g_pNetif = NetifGetLwipInterface();
        if (g_pNetif == NULL) {
            return EI_NETIF_STATUS_ERR_NO_NETIF;
        }
    }
    
    memcpy(pui8MAC, g_pNetif->hwaddr, 6);
    
    NETIF_DEBUG_PRINT("MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                      pui8MAC[0], pui8MAC[1], pui8MAC[2],
                      pui8MAC[3], pui8MAC[4], pui8MAC[5]);
    
    return EI_NETIF_STATUS_OK;
}

/**
 *  \brief Get IP address
 */
EI_NetifStatus_t EI_TM4C_NetifGetIP(uint32_t *pui32IP)
{
    if (pui32IP == NULL) {
        return EI_NETIF_STATUS_ERR_INVALID_PARAM;
    }
    
    if (g_pNetif == NULL) {
        g_pNetif = NetifGetLwipInterface();
        if (g_pNetif == NULL) {
            return EI_NETIF_STATUS_ERR_NO_NETIF;
        }
    }
    
    *pui32IP = g_pNetif->ip_addr.addr;
    
    // Check if we have a valid IP
    if (*pui32IP == 0 || *pui32IP == 0xFFFFFFFF) {
        return EI_NETIF_STATUS_ERR_NO_IP;
    }
    
    NETIF_DEBUG_PRINT("IP: %s", NetifIPToString(*pui32IP));
    
    return EI_NETIF_STATUS_OK;
}

/**
 *  \brief Get subnet mask
 */
EI_NetifStatus_t EI_TM4C_NetifGetNetmask(uint32_t *pui32Netmask)
{
    if (pui32Netmask == NULL) {
        return EI_NETIF_STATUS_ERR_INVALID_PARAM;
    }
    
    if (g_pNetif == NULL) {
        g_pNetif = NetifGetLwipInterface();
        if (g_pNetif == NULL) {
            return EI_NETIF_STATUS_ERR_NO_NETIF;
        }
    }
    
    *pui32Netmask = g_pNetif->netmask.addr;
    
    NETIF_DEBUG_PRINT("Netmask: %s", NetifIPToString(*pui32Netmask));
    
    return EI_NETIF_STATUS_OK;
}

/**
 *  \brief Get default gateway
 */
EI_NetifStatus_t EI_TM4C_NetifGetGateway(uint32_t *pui32Gateway)
{
    if (pui32Gateway == NULL) {
        return EI_NETIF_STATUS_ERR_INVALID_PARAM;
    }
    
    if (g_pNetif == NULL) {
        g_pNetif = NetifGetLwipInterface();
        if (g_pNetif == NULL) {
            return EI_NETIF_STATUS_ERR_NO_NETIF;
        }
    }
    
    *pui32Gateway = g_pNetif->gw.addr;
    
    NETIF_DEBUG_PRINT("Gateway: %s", NetifIPToString(*pui32Gateway));
    
    return EI_NETIF_STATUS_OK;
}

/**
 *  \brief Check if interface is up
 */
EI_NetifStatus_t EI_TM4C_NetifIsUp(bool *pbIsUp)
{
    if (pbIsUp == NULL) {
        return EI_NETIF_STATUS_ERR_INVALID_PARAM;
    }
    
    if (g_pNetif == NULL) {
        g_pNetif = NetifGetLwipInterface();
        if (g_pNetif == NULL) {
            return EI_NETIF_STATUS_ERR_NO_NETIF;
        }
    }
    
    *pbIsUp = netif_is_up(g_pNetif);
    
    NETIF_DEBUG_PRINT("Interface is %s", (*pbIsUp) ? "UP" : "DOWN");
    
    return EI_NETIF_STATUS_OK;
}

/**
 *  \brief Check if IP address is assigned
 */
EI_NetifStatus_t EI_TM4C_NetifHasIP(bool *pbHasIP)
{
    if (pbHasIP == NULL) {
        return EI_NETIF_STATUS_ERR_INVALID_PARAM;
    }
    
    if (g_pNetif == NULL) {
        g_pNetif = NetifGetLwipInterface();
        if (g_pNetif == NULL) {
            return EI_NETIF_STATUS_ERR_NO_NETIF;
        }
    }
    
    *pbHasIP = (g_pNetif->ip_addr.addr != 0 && g_pNetif->ip_addr.addr != 0xFFFFFFFF);
    
    NETIF_DEBUG_PRINT("IP address is %s", (*pbHasIP) ? "ASSIGNED" : "PENDING");
    
    return EI_NETIF_STATUS_OK;
}
