//*****************************************************************************
//
// ei_tm4c_config.h - EtherNet/IP Configuration for TM4C1294XL
//
// This file contains TM4C-specific configuration for the EtherNet/IP stack
// adaptation. It defines device parameters, memory settings, and hardware
// abstractions needed to integrate EtherNet/IP with TM4C's lwIP stack.
//
// Copyright (c) 2025 Texas Instruments Incorporated.
//
//*****************************************************************************

#ifndef EI_TM4C_CONFIG_H
#define EI_TM4C_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

//*****************************************************************************
//
// EtherNet/IP Device Identity
//
// These values define how the device identifies itself to EtherNet/IP 
// scanners and other devices on the network.
//
//*****************************************************************************

// Vendor ID: Texas Instruments (0x0186)
// Reference: http://www.odva.org/
#define EI_DEVICE_VENDOR_ID             0x0186

// Device Type: Generic Device (0x002B)
// Other options:
//   0x0002 = AC Drive
//   0x0007 = Programmable Logic Controller
//   0x000E = Sensor
//   0x002B = Generic Device
#define EI_DEVICE_TYPE                  0x002B

// Product Code: TM4C EtherNet/IP Device (user-defined)
#define EI_DEVICE_PRODUCT_CODE          0x1294

// Serial Number (unique per device)
#define EI_DEVICE_SERIAL_NUMBER         0x00000001

// Device Name (must be <= 32 characters, used in identity attribute)
#define EI_DEVICE_NAME                  "TM4C1294 EtherNet/IP"

// Hardware Revision
#define EI_DEVICE_REVISION_MAJOR        1
#define EI_DEVICE_REVISION_MINOR        0

// Firmware Revision
#define EI_DEVICE_FIRMWARE_MAJOR        1
#define EI_DEVICE_FIRMWARE_MINOR        0

//*****************************************************************************
//
// Memory Configuration
//
// TM4C1294 has:
//   - Flash: 1024 KB (0x100000)
//   - SRAM:  256 KB (0x40000)
//
// Current lwIP setup uses approximately:
//   - Flash: ~100 KB
//   - RAM: ~50 KB
//
// EtherNet/IP stack will add approximately:
//   - Flash: 200-300 KB (depends on configuration)
//   - RAM: 30-50 KB
//
//*****************************************************************************

// Maximum number of concurrent EtherNet/IP instances
#define EI_CONFIG_MAX_INSTANCES         256

// Maximum number of concurrent connections/assemblies
#define EI_CONFIG_MAX_CONNECTIONS       32

// Maximum number of attributes per class
#define EI_CONFIG_MAX_ATTRIBUTES        128

// Maximum number of services per class
#define EI_CONFIG_MAX_SERVICES          32

// Memory pools
#define EI_CONFIG_PACKET_BUFFER_SIZE    (1024 * 2)    // 2KB per packet buffer
#define EI_CONFIG_NUM_PACKET_BUFFERS    4             // 4 buffers = 8KB

//*****************************************************************************
//
// Task Configuration
//
// For bare-metal implementation on TM4C (no RTOS)
// These settings are placeholders for potential future RTOS use.
//
//*****************************************************************************

#define EI_CONFIG_TASK_PRIORITY_CYCLIC  2
#define EI_CONFIG_TASK_PRIORITY_PACKET  3

// Stack size for EtherNet/IP tasks (if using RTOS)
#define EI_CONFIG_TASK_STACK_SIZE       (4 * 1024)    // 4 KB per task

//*****************************************************************************
//
// lwIP Integration Settings
//
// These settings configure how EtherNet/IP integrates with the
// existing lwIP stack on TM4C.
//
//*****************************************************************************

// EtherNet/IP uses UDP port 2222 for encapsulation
#define EI_CONFIG_EIP_PORT              2222

// EtherNet/IP uses TCP port 44818 for connected mode (optional)
#define EI_CONFIG_EIP_TCP_PORT          44818

// lwIP network interface handle
// This will be initialized by ei_tm4c_netif.c
// The netif is created by lwIPInit() in enet_lwip.c
extern struct netif g_ei_netif;

//*****************************************************************************
//
// Hardware Configuration
//
// TM4C1294XL specific settings
//
//*****************************************************************************

// System clock frequency (set by enet_lwip.c)
extern uint32_t g_ui32SysClock;

// UART instance for debug output
// Using UART0 (same as lwIP example)
#define EI_CONFIG_UART_INSTANCE         0

// LED GPIO port and pin for status indication
#define EI_CONFIG_LED_PORT              GPIO_PORTN_BASE
#define EI_CONFIG_LED_PIN               GPIO_PIN_1

// Enable/disable debug output
#define EI_CONFIG_DEBUG_OUTPUT          1

// Enable/disable assertions in debug builds
#define EI_CONFIG_DEBUG_ASSERTIONS      1

//*****************************************************************************
//
// MAC Address Configuration
//
// The MAC address is read from Flash USER registers (same as lwIP)
// and will be available through the lwIP netif structure.
//
//*****************************************************************************

// We retrieve MAC from lwIP netif instead of duplicating
// lwIP already reads it from Flash USER0/USER1 registers
#define EI_CONFIG_USE_LWIP_MAC          1

//*****************************************************************************
//
// Debug Macros
//
// These macros provide conditional debug output
//
//*****************************************************************************

#if EI_CONFIG_DEBUG_OUTPUT
  #define EI_LOG(fmt, ...) \
    do { \
        if (1) { \
            UARTprintf("[EI] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)

  #define EI_LOG_ERROR(fmt, ...) \
    do { \
        if (1) { \
            UARTprintf("[EI_ERR] " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__); \
        } \
    } while(0)

  #define EI_LOG_WARN(fmt, ...) \
    do { \
        if (1) { \
            UARTprintf("[EI_WARN] " fmt "\n", ##__VA_ARGS__); \
        } \
    } while(0)
#else
  #define EI_LOG(fmt, ...)       do { } while(0)
  #define EI_LOG_ERROR(fmt, ...) do { } while(0)
  #define EI_LOG_WARN(fmt, ...)  do { } while(0)
#endif

#if EI_CONFIG_DEBUG_ASSERTIONS
  #define EI_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            EI_LOG_ERROR("Assertion failed: %s", #cond); \
            while(1) { } \
        } \
    } while(0)
#else
  #define EI_ASSERT(cond) do { } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif // EI_TM4C_CONFIG_H
