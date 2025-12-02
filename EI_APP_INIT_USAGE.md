# EI_APP_Init() Call Location & Usage

**Status:** NOT YET CALLED in actual code  
**Date:** December 2, 2025  
**Project:** TM4C EtherNet/IP Integration

---

## 🔍 Current Call Status

### Function Definition
| Item | Details |
|------|---------|
| **Function** | `EI_AppStatus_t EI_APP_Init(const EI_AppDeviceConfig_t *pDevConfig)` |
| **Location** | `ethernetip/ei_app_minimal.c` (line 95) |
| **Header** | `ethernetip/ei_app_minimal.h` (line 79) |
| **Status** | ✅ Defined and ready to use |
| **Called in actual enet_lwip.c** | ❌ **NOT YET** - This is what you need to add! |

---

## 📍 WHERE IT SHOULD BE CALLED

### File: `enet_lwip.c` - Main Function

**Current Code (NOT calling EI_APP_Init):**
```c
int main(void)
{
    // ... system initialization ...
    
    // Network stack initialization
    lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);
    
    // Device locator
    LocatorInit();
    LocatorMACAddrSet(pui8MACArray);
    
    // Web server
    httpd_init();
    
    // ❌ MISSING: EI_APP_Init() should be called HERE!
    
    // ... interrupt setup ...
    
    // Main loop
    while(1) { ... }
}
```

### ✅ CORRECT CODE (After adding EI_APP_Init)

You need to add these lines **AFTER** `lwIPInit()` and **BEFORE** the main loop:

```c
int main(void)
{
    // ... system initialization ...
    
    // Network stack initialization
    lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);
    
    // Device locator
    LocatorInit();
    LocatorMACAddrSet(pui8MACArray);
    
    // Web server
    httpd_init();
    
    // 🆕 ADD THESE LINES:
    #include "ethernetip/ei_app_minimal.h"
    
    if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
        UARTprintf("EtherNet/IP initialization failed!\n");
        while(1);  // Halt on error
    }
    UARTprintf("EtherNet/IP initialized successfully\n");
    
    // ... interrupt setup ...
    
    // Main loop
    while(1) { ... }
}
```

---

## 📋 IMPLEMENTATION CHECKLIST

### Step 1: Add Include Header
**File:** `enet_lwip.c`  
**Location:** Near top with other includes

```c
#include "ethernetip/ei_app_minimal.h"
```

### Step 2: Call EI_APP_Init()
**File:** `enet_lwip.c`  
**Location:** In main() function, after `lwIPInit()` completes

```c
// Call with NULL to use default device configuration
if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
    UARTprintf("ERROR: EtherNet/IP initialization failed\n");
    while(1);  // Stop here
}
```

### Step 3: Add EI_APP_Process() to Main Loop
**File:** `enet_lwip.c`  
**Location:** In main loop, inside `while(g_bLED == false)` block

```c
while(1)
{
    while(g_bLED == false)
    {
        // 🆕 ADD THIS:
        EI_APP_Process();
    }
    
    g_bLED = false;
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, ...);
}
```

---

## 🎯 WHAT EI_APP_Init() DOES

### Initialization Sequence:

```
EI_APP_Init(NULL)
    │
    ├─ [1] Check if already initialized
    │       └─ Returns ERR_ALREADY_INIT if called twice
    │
    ├─ [2] Use provided config or defaults
    │       └─ If pDevConfig = NULL, uses defaults from ei_tm4c_config.h
    │
    ├─ [3] Initialize Board Layer
    │       └─ EI_TM4C_BoardInit()
    │           ├─ Configures GPIO for LED
    │           ├─ Sets LED to OFF
    │           └─ Ready for LED control
    │
    ├─ [4] Initialize Network Interface Layer
    │       └─ EI_TM4C_NetifInit()
    │           ├─ Gets lwIP's network interface
    │           ├─ Reads MAC address from lwIP
    │           ├─ Reads IP address (may be 0 if DHCP not yet assigned)
    │           └─ Status: DOWN initially
    │
    ├─ [5] Log Device Information
    │       ├─ Device name: "TM4C EtherNet/IP Device"
    │       ├─ Vendor ID: 0x0186 (Texas Instruments)
    │       ├─ Device Type: 0x002B (Generic device)
    │       ├─ Product Code: 0x1294 (TM4C1294)
    │       ├─ Serial Number: 0x12345678
    │       ├─ HW Revision: 1.0
    │       └─ FW Revision: 1.0
    │
    ├─ [6] Set LED Status
    │       └─ Blink LED to indicate initialization
    │
    ├─ [7] Mark as Initialized
    │       └─ Sets g_bAppInitialized = true
    │
    └─ Returns: EI_APP_STATUS_OK (success)
```

### Return Values:

| Return Code | Meaning | Action |
|------------|---------|--------|
| `EI_APP_STATUS_OK` | ✅ Success | Continue to main loop |
| `EI_APP_STATUS_ERR_ALREADY_INIT` | ⚠️ Called twice | Check code flow |
| `EI_APP_STATUS_ERR_INVALID_PARAM` | ❌ Bad parameter | Check pDevConfig |
| `EI_APP_STATUS_ERR_NETIF_FAILED` | ❌ lwIP not ready | Call lwIPInit() first |

---

## 💡 DIFFERENT CALL SCENARIOS

### Scenario 1: Call with NULL (Default Config)
```c
// Use defaults from ei_tm4c_config.h
if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
    UARTprintf("Init failed\n");
}
```

### Scenario 2: Call with Custom Config
```c
// Create custom device configuration
EI_AppDeviceConfig_t customConfig = {
    .ui16VendorID = 0x0186,
    .ui16DeviceType = 0x002B,
    .ui16ProductCode = 0x1294,
    .ui32SerialNumber = 0xDEADBEEF,
    .ui8RevisionMajor = 2,
    .ui8RevisionMinor = 0,
    .ui8FWRevisionMajor = 1,
    .ui8FWRevisionMinor = 5,
    .pcDeviceName = "My Custom Device",
};

// Call with custom config
if (EI_APP_Init(&customConfig) != EI_APP_STATUS_OK) {
    UARTprintf("Init failed\n");
}
```

### Scenario 3: Conditional Initialization
```c
// Only initialize if network is ready
if (EI_TM4C_NetifIsUp(NULL)) {
    if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
        UARTprintf("EtherNet/IP failed to initialize\n");
    }
}
```

---

## 🚀 COMPLETE INTEGRATION EXAMPLE

### Modified enet_lwip.c (Relevant Sections)

```c
// At the top with other includes
#include "ethernetip/ei_app_minimal.h"

int main(void)
{
    uint32_t ui32User0, ui32User1;
    uint8_t pui8MACArray[8];
    
    // ========================================
    // SYSTEM INITIALIZATION
    // ========================================
    SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);
    g_ui32SysClock = SysCtlClockFreqSet(..., 120000000);
    
    PinoutSet(true, false);
    UARTStdioConfig(0, 115200, g_ui32SysClock);
    
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
    
    // ========================================
    // TIMER SETUP
    // ========================================
    SysTickPeriodSet(g_ui32SysClock / 100);
    SysTickEnable();
    SysTickIntEnable();
    
    // ========================================
    // LWIP INITIALIZATION (Required!)
    // ========================================
    FlashUserGet(&ui32User0, &ui32User1);
    
    pui8MACArray[0] = ((ui32User0 >>  0) & 0xff);
    pui8MACArray[1] = ((ui32User0 >>  8) & 0xff);
    pui8MACArray[2] = ((ui32User0 >> 16) & 0xff);
    pui8MACArray[3] = ((ui32User1 >>  0) & 0xff);
    pui8MACArray[4] = ((ui32User1 >>  8) & 0xff);
    pui8MACArray[5] = ((ui32User1 >> 16) & 0xff);
    
    lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);
    
    LocatorInit();
    LocatorMACAddrSet(pui8MACArray);
    
    httpd_init();
    
    // ========================================
    // 🆕 ETHERNETIP INITIALIZATION
    // ========================================
    if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
        UARTprintf("Failed to initialize EtherNet/IP\n");
        while(1);  // Error - halt here
    }
    UARTprintf("EtherNet/IP initialized\n");
    
    // ========================================
    // INTERRUPT PRIORITIES
    // ========================================
    MAP_IntPrioritySet(INT_EMAC0, 0xC0);
    MAP_IntPrioritySet(FAULT_SYSTICK, 0x80);
    
    // ========================================
    // MAIN LOOP
    // ========================================
    while(1)
    {
        // Wait for SysTick (10ms interval)
        while(g_bLED == false)
        {
            // 🆕 PROCESS ETHERNETIP
            EI_APP_Process();
            
            // (Ethernet interrupt will fire here automatically
            //  and process incoming packets via lwIP)
        }
        
        g_bLED = false;
        
        // Toggle LED
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,
                     (GPIOPinRead(...) ^ GPIO_PIN_1));
    }
}
```

---

## ⚠️ IMPORTANT NOTES

### Calling Order
```
1. System initialization (clocks, GPIO, UART)
2. lwIPInit()              ← MUST happen first
3. EI_APP_Init()           ← Call AFTER lwIPInit
4. Main loop               ← Call EI_APP_Process() here
```

### Why This Order?
- **lwIPInit() first**: Creates network interface, initializes MAC driver
- **EI_APP_Init() second**: Reads MAC/IP from lwIP (depends on it existing)
- **Main loop last**: Both systems are ready to process packets

### Error Handling
If `EI_APP_Init()` fails:
- Check that `lwIPInit()` completed successfully
- Verify Ethernet cable is connected (for link detection)
- Check UART debug output for detailed error messages

### Thread Safety
- ✅ Safe to call from main()
- ✅ Safe to call from initialization context
- ❌ NOT safe to call from interrupt context
- ❌ NOT safe to call from multiple threads

---

## 📞 FUNCTION CALL CHAIN

When you call `EI_APP_Init(NULL)`:

```
enet_lwip.c: main()
    ↓
EI_APP_Init(NULL)
    ├─ EI_TM4C_BoardInit()
    │   └─ GPIO configuration for LED
    │
    ├─ EI_TM4C_NetifInit()
    │   ├─ Queries lwIP for netif
    │   ├─ Gets MAC from lwIP
    │   ├─ Gets IP from lwIP
    │   └─ Validates link status
    │
    └─ Logs device information & status
```

Returns to `enet_lwip.c: main()` and continues...

---

## 🔄 WHAT HAPPENS AFTER Init

After `EI_APP_Init()` completes successfully:

### Device is Ready
- ✅ LED blinking (initialization indicator)
- ✅ Network interface online
- ✅ MAC address available
- ✅ IP address waiting (if DHCP enabled)

### During Main Loop
Each call to `EI_APP_Process()`:
- Updates LED blink pattern
- Monitors network status
- Will process EI_API packets when library is integrated

### When Ethernet Packet Arrives
Automatically (no code needed):
- INT_EMAC0 fires
- MAC driver → lwIP → UDP port 2222
- EI_API processes packet
- Response queued in transmit buffer

---

## 📝 SUMMARY

| Item | Status | Action |
|------|--------|--------|
| Function defined? | ✅ Yes | In `ei_app_minimal.c` |
| Function declared? | ✅ Yes | In `ei_app_minimal.h` |
| Called in enet_lwip.c? | ❌ **NO** | **You must add it** |
| Where to call? | - | After `lwIPInit()` in main() |
| How to call? | - | `EI_APP_Init(NULL)` |
| Check return value? | ✅ Yes | For error handling |
| When to call? | - | Once at startup |
| Call again? | ❌ No | Only once (will error) |
| Thread-safe? | ✅ Yes | From main only |

---

## 🎯 NEXT STEPS

**To actually integrate EI_APP_Init():**

1. ✅ Open `enet_lwip.c`
2. ✅ Add `#include "ethernetip/ei_app_minimal.h"` to headers
3. ✅ Add `EI_APP_Init(NULL)` call after `lwIPInit()`
4. ✅ Check return value and handle errors
5. ✅ Add `EI_APP_Process()` call in main loop
6. ✅ Recompile and test

**Result:** EtherNet/IP stack will be initialized and ready to receive packets!
