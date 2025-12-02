# Quick Reference Guide - EtherNet/IP Abstraction Layer

## Files Created

```
ethernetip/
├── ei_tm4c_config.h              Configuration and macros
├── ei_tm4c_netif.h/c             Network interface (lwIP adapter)
├── ei_tm4c_board.h/c             Board hardware abstractions
├── ei_app_minimal.h/c            Application wrapper
├── ei_integration_example.h      Integration examples
└── README.md                     Full documentation

+ IMPLEMENTATION_SUMMARY.md       This summary
+ ETHERNETIP_INTEGRATION_PLAN.md  Architecture document
```

## Quick Integration Steps

### 1. Add to Makefile (or CCS project)

```makefile
# Add include path
IFLAGS += -I./ethernetip

# Add source files
SRCS += ethernetip/ei_tm4c_netif.c
SRCS += ethernetip/ei_tm4c_board.c
SRCS += ethernetip/ei_app_minimal.c
```

### 2. Modify enet_lwip.c

**Add at top after existing includes:**
```c
#include "ethernetip/ei_tm4c_config.h"
#include "ethernetip/ei_app_minimal.h"
```

**In main(), after lwIPInit():**
```c
// Initialize EtherNet/IP stack
if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
    UARTprintf("EtherNet/IP init failed!\n");
    while(1);
}
```

**In main loop after LED blink:**
```c
// Process EtherNet/IP
EI_APP_Process();
```

### 3. Compile and Test

```bash
# GCC
make VERBOSE=1

# CCS
Project → Build → Build Project

# Other tools
Add files to project and compile
```

## API Quick Reference

### Network Interface (ei_tm4c_netif.h)
```c
EI_TM4C_NetifInit()          // Initialize interface
EI_TM4C_NetifGetMAC()        // Get MAC address
EI_TM4C_NetifGetIP()         // Get IP address
EI_TM4C_NetifIsUp()          // Check if up
EI_TM4C_NetifHasIP()         // Check if IP assigned
```

### Board Abstraction (ei_tm4c_board.h)
```c
EI_TM4C_BoardInit()          // Initialize board
EI_TM4C_LEDSetStatus()       // Set LED (ON/OFF/BLINK)
EI_TM4C_LEDToggle()          // Toggle LED
EI_TM4C_LEDSetBlinkRate()    // Set blink rate (1-10 Hz)
```

### Application (ei_app_minimal.h)
```c
EI_APP_Init(NULL)            // Initialize app
EI_APP_Process()             // Process in loop
EI_APP_GetStatusString()     // Get status
EI_APP_IsInitialized()       // Check if initialized
```

### Debug Output (ei_tm4c_config.h)
```c
EI_LOG("Message")            // Info
EI_LOG_WARN("Warning")       // Warning
EI_LOG_ERROR("Error")        // Error
EI_ASSERT(condition)         // Assertion
```

## Configuration (ei_tm4c_config.h)

```c
// Device identity
#define EI_DEVICE_VENDOR_ID          0x0186
#define EI_DEVICE_PRODUCT_CODE       0x1294
#define EI_DEVICE_SERIAL_NUMBER      0x00000001

// Enable debug output
#define EI_CONFIG_DEBUG_OUTPUT       1

// Memory settings
#define EI_CONFIG_PACKET_BUFFER_SIZE (1024 * 2)
#define EI_CONFIG_NUM_PACKET_BUFFERS 4
```

## Expected Output

On UART0 (115200 baud):

```
[EI] Board abstraction layer initialized
[EI] Network interface initialized
[EI] MAC: 88:0c:e0:5c:46:e9
[EI_NETIF] IP: 192.168.1.100
[EI] Device: TM4C1294 EtherNet/IP
[EI] Vendor ID: 0x0186
[EI] Serial Number: 0x00000001
[EI] HW Rev: 1.0
[EI] FW Rev: 1.0
[EI] EtherNet/IP application initialized successfully
```

LED should be **blinking** to indicate device is running.

## Troubleshooting

| Issue | Fix |
|-------|-----|
| Compilation fails | Check `-I./ethernetip` in IFLAGS |
| `undefined reference` errors | Make sure .c files are in SRCS |
| `netif_list` not found | Verify lwIP is properly linked |
| No debug output | Set `EI_CONFIG_DEBUG_OUTPUT 1` |
| LED not blinking | Check GPIO initialization in enet_lwip.c |
| IP not assigned | Verify Ethernet cable connected |

## File Purposes (One-Line Summary)

| File | Purpose |
|------|---------|
| `ei_tm4c_config.h` | Configuration, device IDs, debug macros |
| `ei_tm4c_netif.h/c` | Query network interface (lwIP adapter) |
| `ei_tm4c_board.h/c` | GPIO, LED control, hardware abstractions |
| `ei_app_minimal.h/c` | High-level EtherNet/IP app initialization |
| `ei_integration_example.h` | Code examples and integration patterns |
| `README.md` | Complete documentation and reference |

## Next Steps After Compilation

1. ✅ Get EI_API library from TI (libei_api.a or source)
2. ✅ Link against EI_API in Makefile
3. ✅ Add actual stack processing in `EI_APP_Process()`
4. ✅ Copy device_profiles/ from AM64x project
5. ✅ Implement I/O callbacks
6. ✅ Test with EtherNet/IP scanner

## Memory Usage (Abstraction Layer Only)

- **Flash:** ~6 KB
- **RAM:** ~2 KB

## Hardware Requirements

- **Board:** EK-TM4C1294XL
- **Clock:** 120 MHz (configured by enet_lwip.c)
- **Ethernet:** Connected to network with DHCP or AutoIP
- **UART0:** For debug output (115200 baud)

## Support Files

- `ETHERNETIP_INTEGRATION_PLAN.md` - Overall architecture
- `IMPLEMENTATION_SUMMARY.md` - Detailed summary
- `ethernetip/README.md` - Complete reference manual
- `ethernetip/ei_integration_example.h` - Code examples

## Important Notes

⚠️ **This is only the abstraction layer!**
- Does NOT include actual EtherNet/IP stack
- Does NOT include device profiles
- Does NOT include I/O handling
- **Next step:** Integrate EI_API library

✅ **What this layer provides:**
- Clean API for network access
- Hardware abstraction
- Debug output
- Device initialization framework
- Ready for EI_API integration

---

**Created:** December 2, 2025  
**Version:** 1.0 - Abstraction Layer Implementation
