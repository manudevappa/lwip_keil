//*****************************************************************************
//
// ei_tm4c_netif.h - EtherNet/IP lwIP Network Interface Adapter for TM4C
//
// This header defines the network interface adaptation layer that hooks
// EtherNet/IP into the existing lwIP network stack on TM4C without
// requiring re-initialization of the MAC or PHY.
//
// Copyright (c) 2025 Texas Instruments Incorporated.
//
//*****************************************************************************

#ifndef EI_TM4C_NETIF_H
#define EI_TM4C_NETIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//*****************************************************************************
//
// Network Interface Status Codes
//
//*****************************************************************************

typedef enum {
    EI_NETIF_STATUS_OK = 0,              // Success
    EI_NETIF_STATUS_ERR_NO_NETIF = 1,    // Network interface not found
    EI_NETIF_STATUS_ERR_NOT_UP = 2,      // Network interface not up
    EI_NETIF_STATUS_ERR_NO_IP = 3,       // No IP address assigned
    EI_NETIF_STATUS_ERR_INVALID_PARAM = 4, // Invalid parameter
} EI_NetifStatus_t;

//*****************************************************************************
//
// Network Interface Information Structure
//
// This structure provides information about the current network interface
// state that EtherNet/IP needs.
//
//*****************************************************************************

typedef struct {
    uint8_t     ui8MAC[6];                // MAC address (6 bytes)
    uint32_t    ui32IP;                   // IP address (network byte order)
    uint32_t    ui32Netmask;              // Subnet mask (network byte order)
    uint32_t    ui32Gateway;              // Default gateway (network byte order)
    bool        bIsUp;                    // Network interface is up
    bool        bHasIP;                   // IP address is assigned
    bool        bUseDHCP;                 // Using DHCP or AutoIP
} EI_Netif_Info_t;

//*****************************************************************************
//
// Network Interface Public API
//
//*****************************************************************************

/**
 *  \brief Initialize EtherNet/IP network interface
 *
 *  This function initializes the EtherNet/IP network interface adapter,
 *  verifying that lwIP is running and configured. It should be called
 *  AFTER lwIPInit() has completed in enet_lwip.c.
 *
 *  \param  None
 *
 *  \return EI_NetifStatus_t
 *    \retval EI_NETIF_STATUS_OK           - Success
 *    \retval EI_NETIF_STATUS_ERR_NO_NETIF - lwIP network interface not found
 *    \retval EI_NETIF_STATUS_ERR_NOT_UP   - Network interface not up
 */
EI_NetifStatus_t EI_TM4C_NetifInit(void);

/**
 *  \brief Get current network interface information
 *
 *  Retrieves the current state of the network interface including MAC,
 *  IP address, netmask, and gateway.
 *
 *  \param  pInfo  Pointer to EI_Netif_Info_t structure to fill
 *
 *  \return EI_NetifStatus_t
 *    \retval EI_NETIF_STATUS_OK               - Success
 *    \retval EI_NETIF_STATUS_ERR_INVALID_PARAM - NULL pointer passed
 */
EI_NetifStatus_t EI_TM4C_NetifGetInfo(EI_Netif_Info_t *pInfo);

/**
 *  \brief Get MAC address
 *
 *  Retrieves the hardware MAC address from the lwIP network interface.
 *
 *  \param  pui8MAC  Pointer to 6-byte array for MAC address
 *
 *  \return EI_NetifStatus_t
 *    \retval EI_NETIF_STATUS_OK               - Success
 *    \retval EI_NETIF_STATUS_ERR_INVALID_PARAM - NULL pointer passed
 */
EI_NetifStatus_t EI_TM4C_NetifGetMAC(uint8_t *pui8MAC);

/**
 *  \brief Get IP address
 *
 *  Retrieves the current IPv4 address assigned to the interface.
 *
 *  \param  pui32IP  Pointer to store IP address (network byte order)
 *
 *  \return EI_NetifStatus_t
 *    \retval EI_NETIF_STATUS_OK               - Success
 *    \retval EI_NETIF_STATUS_ERR_INVALID_PARAM - NULL pointer passed
 *    \retval EI_NETIF_STATUS_ERR_NO_IP       - No IP address assigned yet
 */
EI_NetifStatus_t EI_TM4C_NetifGetIP(uint32_t *pui32IP);

/**
 *  \brief Get subnet mask
 *
 *  Retrieves the subnet mask for the current IP address.
 *
 *  \param  pui32Netmask  Pointer to store subnet mask (network byte order)
 *
 *  \return EI_NetifStatus_t
 *    \retval EI_NETIF_STATUS_OK               - Success
 *    \retval EI_NETIF_STATUS_ERR_INVALID_PARAM - NULL pointer passed
 */
EI_NetifStatus_t EI_TM4C_NetifGetNetmask(uint32_t *pui32Netmask);

/**
 *  \brief Get default gateway
 *
 *  Retrieves the default gateway address.
 *
 *  \param  pui32Gateway  Pointer to store gateway address (network byte order)
 *
 *  \return EI_NetifStatus_t
 *    \retval EI_NETIF_STATUS_OK               - Success
 *    \retval EI_NETIF_STATUS_ERR_INVALID_PARAM - NULL pointer passed
 */
EI_NetifStatus_t EI_TM4C_NetifGetGateway(uint32_t *pui32Gateway);

/**
 *  \brief Check if interface is up
 *
 *  Determines if the network interface is currently up and ready.
 *
 *  \param  pbIsUp  Pointer to store boolean result
 *
 *  \return EI_NetifStatus_t
 *    \retval EI_NETIF_STATUS_OK               - Success
 *    \retval EI_NETIF_STATUS_ERR_INVALID_PARAM - NULL pointer passed
 */
EI_NetifStatus_t EI_TM4C_NetifIsUp(bool *pbIsUp);

/**
 *  \brief Check if IP address is assigned
 *
 *  Determines if a valid IP address has been assigned to the interface
 *  (either via DHCP, AutoIP, or static configuration).
 *
 *  \param  pbHasIP  Pointer to store boolean result
 *
 *  \return EI_NetifStatus_t
 *    \retval EI_NETIF_STATUS_OK               - Success
 *    \retval EI_NETIF_STATUS_ERR_INVALID_PARAM - NULL pointer passed
 */
EI_NetifStatus_t EI_TM4C_NetifHasIP(bool *pbHasIP);

#ifdef __cplusplus
}
#endif

#endif // EI_TM4C_NETIF_H
