# EtherNet/IP Abstraction Layer for TM4C

## Overview

This directory contains a **TM4C-specific abstraction layer** for integrating the EtherNet/IP stack into the existing `enet_lwip` project. The abstraction layer provides:

- **Clean API** for EtherNet/IP to access network services
- **No MAC re-initialization** - reuses existing lwIP network interface
- **Bare-metal compatible** - works without RTOS
- **Hardware abstractions** - GPIO, LED, system info
- **Minimal footprint** - ~5-10 KB of code

## What's in This Directory

```
ethernetip/
├── ei_tm4c_config.h              # Configuration and debug macros
├── ei_tm4c_netif.h               # Network interface adapter API
├── ei_tm4c_netif.c               # Network interface adapter implementation
├── ei_tm4c_board.h               # Board abstraction layer API
├── ei_tm4c_board.c               # Board abstraction layer implementation
├── ei_app_minimal.h              # Minimal app wrapper API
├── ei_app_minimal.c              # Minimal app wrapper implementation
├── ei_integration_example.h      # Integration guide and examples
└── README.md                     # This file
```

## How It Works

### Architecture Diagram

```
┌─────────────────────────────────────────┐
│   EtherNet/IP Stack (EI_API library)    │ (to be integrated)
├─────────────────────────────────────────┤
│  Abstraction Layer (this directory)     │
│  - ei_app_minimal.c                     │ Provides high-level API
│  - ei_tm4c_config.h                     │
├─────────────────────────────────────────┤
│  Board/Hardware Adapters                │
│  - ei_tm4c_board.c      (GPIO, LED)     │
│  - ei_tm4c_netif.c      (network)       │
├─────────────────────────────────────────┤
│  Existing TM4C Infrastructure           │
│  - enet_lwip.c          (lwIP, MAC)     │
│  - driverlib            (GPIO, UART)    │
│  - utils/lwiplib        (network stack) │
└─────────────────────────────────────────┘
```

### Key Design Decisions

1. **No MAC Re-initialization**
   - The MAC and PHY are already initialized by `lwIPInit()` in `enet_lwip.c`
   - The abstraction layer simply queries the existing `netif` structure
   - This avoids conflicts and saves code space

2. **Bare-Metal First**
   - Designed to work without FreeRTOS or any RTOS
   - Can be adapted to work with RTOS later if needed
   - Processing happens in the main loop via `EI_APP_Process()`

3. **Modular Design**
   - Each component is independent and can be updated separately
   - Low-level components (netif, board) don't depend on app layer
   - Easy to replace parts when EI_API library is integrated

## File Descriptions

### ei_tm4c_config.h
**Purpose:** Configuration header with device IDs, memory settings, and debug macros

**Key Settings:**
- Device identity: VendorID, DeviceType, ProductCode, SerialNumber
- Device name and revision info
- Memory pool sizes
- Debug output controls
- Assertion handling

**Usage:** Include in all files that need configuration or debug output

### ei_tm4c_netif.c / ei_tm4c_netif.h
**Purpose:** Network interface adapter that bridges EtherNet/IP to lwIP

**Functions:**
- `EI_TM4C_NetifInit()` - Initialize network interface
- `EI_TM4C_NetifGetInfo()` - Get MAC, IP, netmask, gateway
- `EI_TM4C_NetifGetMAC()` - Get hardware MAC address
- `EI_TM4C_NetifGetIP()` - Get current IP address
- `EI_TM4C_NetifIsUp()` - Check if interface is up
- `EI_TM4C_NetifHasIP()` - Check if IP is assigned

**Key Implementation Details:**
- Gets `netif` from `lwIP's netif_list`
- Validates interface is up before returning data
- Handles byte order conversions
- Provides status codes for error handling

**No Modification Needed:** This file works as-is with lwIP

### ei_tm4c_board.c / ei_tm4c_board.h
**Purpose:** Board-specific hardware abstractions

**Functions:**
- LED control: `EI_TM4C_LEDSetStatus()`, `EI_TM4C_LEDToggle()`, `EI_TM4C_LEDSetBlinkRate()`
- System info: `EI_TM4C_GetSysClockHz()`, `EI_TM4C_GetBoardName()`
- `EI_TM4C_BoardProcess()` - Called periodically to update LED blink

**Hardware Details:**
- LED: Port N, Pin 1 (same as original enet_lwip.c)
- System clock from `g_ui32SysClock` (set by enet_lwip.c)
- Blink rate configurable from 1-10 Hz

**Future Expansion:** Can add more GPIO, buttons, analog inputs, etc.

### ei_app_minimal.c / ei_app_minimal.h
**Purpose:** Main EtherNet/IP application API wrapper

**Key Functions:**
- `EI_APP_Init()` - Initialize EtherNet/IP stack
- `EI_APP_Process()` - Process stack periodically
- `EI_APP_Shutdown()` - Clean shutdown
- `EI_APP_GetStatusString()` - Get human-readable status
- Device attribute access: `EI_APP_SetDeviceName()`, `EI_APP_GetSerialNumber()`

**Current Status:**
- ✅ Network initialization verified
- ✅ LED status indication working
- ⏳ Full EI_API integration pending
- ✅ Placeholder for stack processing ready

**To Integrate EI_API:**
1. Add `#include <EI_API.h>` 
2. Replace `EI_APP_Process()` body with actual stack processing
3. Add device profile callbacks for I/O handling

### ei_integration_example.h
**Purpose:** Documentation and code examples

**Contains:**
- Modified `main()` function example showing how to use abstraction layer
- Makefile additions
- CCS IDE project setup instructions
- Debugging troubleshooting guide
- Next steps for full integration

**Reference This File:** When modifying enet_lwip.c for integration

## Integration Steps

### Step 1: Include Headers in enet_lwip.c
```c
#include "ethernetip/ei_tm4c_config.h"
#include "ethernetip/ei_tm4c_netif.h"
#include "ethernetip/ei_tm4c_board.h"
#include "ethernetip/ei_app_minimal.h"
```

### Step 2: Initialize After lwIPInit()
In `main()` after `lwIPInit()` completes:
```c
if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
    UARTprintf("Failed to initialize EtherNet/IP\n");
    while(1);
}
```

### Step 3: Process in Main Loop
In the main loop, add after LED blink code:
```c
EI_APP_Process();
```

### Step 4: Update Makefile
Add include path and source files:
```makefile
IFLAGS += -I$(ROOT)/ethernetip
SRCS += $(ROOT)/ethernetip/ei_tm4c_netif.c
SRCS += $(ROOT)/ethernetip/ei_tm4c_board.c
SRCS += $(ROOT)/ethernetip/ei_app_minimal.c
```

Or for CCS, add files to project and update include paths.

## Testing

### Basic Functionality Test
```c
// In enet_lwip.c main()
if (EI_APP_Init(NULL) == EI_APP_STATUS_OK) {
    UARTprintf(EI_APP_GetStatusString());
    UARTprintf("\n");
}
```

Expected output:
```
[EI] TM4C1294 EtherNet/IP - Net: DOWN - SN: 0x00000001
```

### Network Status Test
After IP is assigned:
```
[EI] TM4C1294 EtherNet/IP - Net: READY - SN: 0x00000001
```

### LED Status Test
LED should be blinking while application runs.

## Debug Output

Debug output is controlled by `EI_CONFIG_DEBUG_OUTPUT` in `ei_tm4c_config.h`:

```c
#define EI_CONFIG_DEBUG_OUTPUT 1  // Enable debug output
```

Available macros:
```c
EI_LOG("Message: %d", value);           // Info level
EI_LOG_WARN("Warning: %s", name);       // Warning level
EI_LOG_ERROR("Error: %d", code);        // Error level
EI_ASSERT(condition);                   // Assertion
```

## Memory Footprint

Estimated resource usage:

| Component | Flash | RAM | Notes |
|-----------|-------|-----|-------|
| ei_tm4c_netif.c | ~1 KB | <1 KB | Network interface |
| ei_tm4c_board.c | ~2 KB | ~0.5 KB | Board abstractions |
| ei_app_minimal.c | ~3 KB | ~0.5 KB | App wrapper |
| **Total** | **~6 KB** | **~2 KB** | Minimal overhead |

Note: Full EI_API library will add 200-500 KB flash and 30-50 KB RAM

## Limitations and Future Work

### Current Limitations
1. ⏳ No actual EtherNet/IP stack processing yet (placeholder only)
2. ⏳ No device profile implementation
3. ⏳ No CIP attribute/service handling
4. ⏳ No I/O scheduling

### Future Enhancements
1. ✅ Integrate EI_API library
2. ✅ Add device profile support
3. ✅ Implement I/O callbacks
4. ✅ Add performance profiling
5. ✅ Optimize memory usage
6. ✅ Add real-time scheduling

## Troubleshooting

### Issue: "undefined reference to 'netif_list'"
**Cause:** lwIP headers not included or lwIP not linked
**Solution:** Verify lwIP is properly configured in Makefile/project

### Issue: Compilation fails with missing declarations
**Cause:** Include paths not set correctly
**Solution:** Add `-I$(ROOT)/ethernetip` to IFLAGS in Makefile

### Issue: EI_APP_Init() fails with ERR_NO_NETIF
**Cause:** lwIPInit() not called or network interface not created
**Solution:** Ensure lwIPInit() completes BEFORE calling EI_APP_Init()

### Issue: EI_APP_Init() fails with ERR_NETIF_FAILED
**Cause:** Network interface not up yet (no link)
**Solution:** This is normal - wait for Ethernet cable connection

## References

- **Original enet_lwip.c**: `enet_lwip.c` in this directory
- **EtherNet/IP Standard**: www.odva.org
- **EI_API Library**: [To be integrated]
- **lwIP Documentation**: https://savannah.nongnu.org/projects/lwip/
- **AM64x EtherNet/IP Example**: `c:\ti\ind_comms_sdk_am64x_2025_00_00_08\examples\industrial_comms\ethernetip_adapter_demo`

## Support and Questions

For issues or questions about this abstraction layer:

1. Check `ei_integration_example.h` for common integration patterns
2. Enable debug output in `ei_tm4c_config.h` for detailed logging
3. Verify network connectivity with `ping` before EtherNet/IP initialization
4. Check linker script (`enet_lwip.ld`) for memory conflicts

---

**Created:** December 2025  
**TM4C Board:** EK-TM4C1294XL  
**lwIP Version:** From TivaWare 2.2.0.295  
**EtherNet/IP API:** To be integrated
