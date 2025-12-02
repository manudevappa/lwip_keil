# EtherNet/IP Integration to TM4C enet_lwip Project

## Overview

This document provides a step-by-step guide to integrate the EtherNet/IP stack into the existing TM4C `enet_lwip` project at:
```
C:\ti\TivaWare_C_Series-2.2.0.295\examples\boards\ek-tm4c1294xl\enet_lwip
```

## Current enet_lwip Project Structure

```
enet_lwip/
├── enet_lwip.c           (Main application - ~374 lines)
├── enet_lwip.h
├── enet_lwip.ld          (Linker script)
├── startup_*.c           (Startup code)
├── lwipopts.h            (lwIP configuration)
├── Makefile              (GCC build)
├── ccs/                  (CCS project files)
├── ewarm/                (IAR project files)
├── rvmdk/                (Keil project files)
├── gcc/                  (GCC-specific files)
├── fs/                   (File system for web server)
└── httpserver_raw/       (Web server)
```

**Current Functionality:**
- lwIP stack running (basic TCP/IP)
- DHCP/AutoIP IP address assignment
- Simple HTTP web server
- LED blinking via SysTick

## EtherNet/IP Stack Requirements (from AM64x project)

**Key Components Needed:**
```
From: c:\ti\ind_comms_sdk_am64x_2025_00_00_08\examples\industrial_comms\ethernetip_adapter_demo

├── app.c / app.h                      (Main EtherNet/IP app initialization)
├── app_task.c / app_task.h            (EtherNet/IP task handling)
├── config/                            (Configuration files)
│   ├── cfg_example.h
│   ├── app/cfg_application.h
│   └── stack/cfg_stack.h
├── device_profiles/                   (Generic & Discrete I/O profiles)
│   ├── generic_device/
│   ├── discrete_io_device/
│   └── common/
├── custom/                            (Custom drivers - mostly unused for TM4C)
└── drivers/                           (Hardware drivers - must adapt)
```

## Integration Challenges

### Challenge 1: No PRU Support on TM4C
- ❌ Can't use PRU-ICSS firmware
- ✅ Replace with standard lwIP netif adapter

### Challenge 2: Different Board Support
- ❌ AM64x uses PRU-ICSS with MDIO
- ✅ TM4C already has working lwIP + Ethernet MAC driver

### Challenge 3: Missing APIs
- ❌ AM64x has `EI_API_ADP_pruicssInit()`
- ✅ Replace with TM4C Ethernet MAC initialization

### Challenge 4: Task/RTOS Differences
- ❌ AM64x uses FreeRTOS with specific configurations
- ⚠️ TM4C can use FreeRTOS OR bare-metal (needs assessment)

## Implementation Approach

### Phase 1: Create TM4C-Specific Abstraction Layer

Create new files in the enet_lwip project:

```
enet_lwip/
├── ethernetip/                    (NEW - EtherNet/IP integration)
│   ├── ei_tm4c_config.h           (TM4C-specific configuration)
│   ├── ei_tm4c_netif.c            (lwIP network interface adapter)
│   ├── ei_tm4c_netif.h
│   ├── ei_tm4c_board.c            (Board-specific functions)
│   ├── ei_tm4c_board.h
│   ├── ei_app.c                   (Adapted from AM64x app.c)
│   ├── ei_app.h
│   ├── ei_app_task.c              (Adapted from AM64x app_task.c)
│   └── ei_app_task.h
│
├── device_profiles/               (COPY from AM64x - portable)
│   ├── generic_device/
│   ├── discrete_io_device/
│   └── common/
│
├── config/                        (COPY/ADAPT from AM64x)
│   ├── cfg_tm4c.h                 (TM4C board config)
│   └── ...
│
└── enet_lwip_with_eip.c           (Modified main that uses EtherNet/IP)
```

### Phase 2: Remove PRU Dependencies

Files to create TM4C equivalents:
- `EI_API_ADP_pruicssInit()` → `ei_tm4c_eth_init()`
- `EI_API_ADP_pruicssStart()` → Already running in lwIP
- `DRV_PRUICSS_getCfgParams()` → `ei_tm4c_get_config()`

### Phase 3: Integrate with Existing lwIP

The existing `enet_lwip.c` initializes lwIP:
- ✅ MAC already configured
- ✅ DHCP/AutoIP already running
- ✅ lwIP timer running
- ✅ Ethernet interrupt handler working

We REUSE this and add EtherNet/IP on top!

---

## Implementation Steps

### STEP 1: Create TM4C Configuration Header

**File: `ethernetip/ei_tm4c_config.h`**

```c
#ifndef EI_TM4C_CONFIG_H
#define EI_TM4C_CONFIG_H

// EtherNet/IP Device Configuration
#define EI_DEVICE_VENDOR_ID        0x0186    // Texas Instruments
#define EI_DEVICE_TYPE             0x002B    // Generic Device
#define EI_DEVICE_PRODUCT_CODE     0x1337    // Custom (change as needed)
#define EI_DEVICE_SERIAL_NUMBER    0x00000001

// Device Identity
#define EI_DEVICE_NAME             "TM4C EtherNet/IP Device"
#define EI_DEVICE_REVISION_MAJOR   1
#define EI_DEVICE_REVISION_MINOR   0

// Stack Configuration
#define EI_MAX_INSTANCES           256
#define EI_MAX_CONNECTIONS         32

// Task Priorities (lwIP is already running, we add on top)
#define EI_TASK_PRIORITY_CYCLIC    2
#define EI_TASK_PRIORITY_PACKET    3

// MAC Address (will be read from Flash user registers like original)
// MAC is already handled by enet_lwip.c

#endif
```

### STEP 2: Create MAC Initialization Adapter

**File: `ethernetip/ei_tm4c_netif.c`**

```c
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/err.h"

#include "ei_tm4c_netif.h"

// IMPORTANT: The MAC/PHY are already initialized by enet_lwip.c!
// We don't need to do that again.
// We just need to hook into the existing network interface.

extern struct netif gNetif;  // Defined in enet_lwip.c or lwiplib

/**
 * Initialize EtherNet/IP with existing lwIP network interface
 * 
 * This function is called AFTER lwIP is fully initialized
 * to integrate the EtherNet/IP stack.
 */
uint32_t EI_TM4C_NetifInit(void)
{
    // The network interface is already initialized by enet_lwip.c
    // We just need to verify it's up
    
    if (&gNetif == NULL || !netif_is_up(&gNetif)) {
        printf("ERROR: Network interface not ready\n");
        return 1;
    }
    
    printf("Network interface ready for EtherNet/IP\n");
    printf("  MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           gNetif.hwaddr[0], gNetif.hwaddr[1], gNetif.hwaddr[2],
           gNetif.hwaddr[3], gNetif.hwaddr[4], gNetif.hwaddr[5]);
    
    if (gNetif.ip_addr.addr != 0) {
        printf("  IP:  %d.%d.%d.%d\n",
               (gNetif.ip_addr.addr) & 0xff,
               (gNetif.ip_addr.addr >> 8) & 0xff,
               (gNetif.ip_addr.addr >> 16) & 0xff,
               (gNetif.ip_addr.addr >> 24) & 0xff);
    }
    
    return 0;  // Success
}

/**
 * Get MAC address (already handled by enet_lwip.c)
 */
uint32_t EI_TM4C_GetMAC(uint8_t *pMac)
{
    if (pMac == NULL) {
        return 1;
    }
    
    // Copy from lwIP network interface
    for (int i = 0; i < 6; i++) {
        pMac[i] = gNetif.hwaddr[i];
    }
    
    return 0;
}

/**
 * Get IP address
 */
uint32_t EI_TM4C_GetIP(uint32_t *pIp)
{
    if (pIp == NULL) {
        return 1;
    }
    
    *pIp = gNetif.ip_addr.addr;
    return 0;
}
```

**File: `ethernetip/ei_tm4c_netif.h`**

```c
#ifndef EI_TM4C_NETIF_H
#define EI_TM4C_NETIF_H

#include <stdint.h>

uint32_t EI_TM4C_NetifInit(void);
uint32_t EI_TM4C_GetMAC(uint8_t *pMac);
uint32_t EI_TM4C_GetIP(uint32_t *pIp);

#endif
```

### STEP 3: Adapt app_task.c for TM4C

**File: `ethernetip/ei_app_task.c` (simplified version)**

This is adapted from the AM64x version. Key changes:
- Remove PRU initialization
- Use lwIP netif that's already running
- Simplify hardware initialization

(This file is large - you would copy from AM64x and modify the `stackInit()` function)

### STEP 4: Integrate into Main

**File: `enet_lwip_with_eip.c` (or modify existing `enet_lwip.c`)**

```c
// At top, after existing includes:

#include "ethernetip/ei_tm4c_config.h"
#include "ethernetip/ei_tm4c_netif.h"
#include "ethernetip/ei_app_task.h"

// In main() function, AFTER lwIP is initialized:

int main(void)
{
    // ... existing enet_lwip.c initialization code ...
    
    // lwIP is now running, ethernet MAC is initialized
    
    // Now initialize EtherNet/IP stack
    if (EI_TM4C_NetifInit() != 0) {
        UARTprintf("Failed to initialize EtherNet/IP network interface\n");
        while(1);
    }
    
    // Initialize EtherNet/IP application
    if (EI_APP_TASK_init() != 0) {
        UARTprintf("Failed to initialize EtherNet/IP application\n");
        while(1);
    }
    
    UARTprintf("EtherNet/IP stack initialized and ready!\n");
    
    // Main loop - call both lwIP and EtherNet/IP processing
    while (1) {
        // lwIP processing (already happening via interrupt)
        
        // EtherNet/IP stack processing
        EI_APP_TASK_run();
        
        // Blinking LED
        if (g_bLED) {
            g_bLED = false;
            // ... LED toggle code ...
        }
    }
}
```

---

## Files Needed from AM64x Project

### Copy (100% Portable - No Changes)
```
✅ device_profiles/generic_device/*
✅ device_profiles/discrete_io_device/*
✅ device_profiles/common/*
✅ config/profile/*
```

### Adapt (Needs Modification)
```
⚠️  app.c / app.h          → Remove PRU references
⚠️  app_task.c / app_task.h → Replace stackInit() function
⚠️  config/cfg_example.h   → Create TM4C-specific version
```

### Don't Copy
```
❌ drivers/pruicss/*       → TM4C has no PRU
❌ custom/ethphy/*         → TM4C already has PHY driver
❌ custom/cust_drivers.c   → Use TM4C's existing drivers
```

---

## Build Considerations

### Makefile Updates
- Add `-I ethernetip/` for include path
- Add `-I device_profiles/` 
- Add `-I config/`
- Link against EtherNet/IP API library (if available)

### Library Dependencies
- EI_API library (must be obtained separately or built)
- lwIP (already in TivaWare)
- TivaWare driverlib (already in project)

### Memory Requirements
- RAM: Need to allocate for EtherNet/IP structures (~10-50KB)
- Flash: EtherNet/IP code (~200-500KB depending on features)

**TM4C1294 Resources:**
- Flash: 1MB (should be sufficient)
- RAM: 256KB (tight, may need optimization)

---

## Testing Checklist

After integration, verify:

- [ ] Project compiles without errors
- [ ] lwIP still initializes (can see "Waiting for link" message)
- [ ] EtherNet/IP stack initializes
- [ ] Device gets IP address (DHCP/AutoIP)
- [ ] `ping` works from PC
- [ ] EtherNet/IP scanner discovers device
- [ ] Device responds to CIP queries
- [ ] Web server still works (if enabled)

---

## Risks & Limitations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| **Memory shortage** | Stack initialization fails | Disable web server, reduce buffers |
| **Real-time performance** | Missed packets | Use bare-metal instead of RTOS |
| **Linker conflicts** | Build fails | Check for symbol duplicates |
| **Interrupt priority** | System instability | Verify interrupt priority setup |

---

## Next Steps

1. **Get EtherNet/IP API library** - You need the compiled EI_API library
2. **Create directory structure** - Set up `ethernetip/` and `device_profiles/` folders
3. **Copy portable files** - Move generic code from AM64x project
4. **Create TM4C adaptations** - Implement TM4C-specific files
5. **Update build system** - Modify Makefile/project files
6. **Test incrementally** - Verify each step before moving to next

---

## References

- **Original AM64x Project:** `c:\ti\ind_comms_sdk_am64x_2025_00_00_08\examples\industrial_comms\ethernetip_adapter_demo`
- **TM4C Project:** `C:\ti\TivaWare_C_Series-2.2.0.295\examples\boards\ek-tm4c1294xl\enet_lwip`
- **Porting Guide:** See `PORTING_ANALYSIS.md` in the AM64x project

