# EI_APP_Init() - Quick Call Location Reference

## ❌ WHERE IT'S NOT CALLED

The function `EI_APP_Init()` is currently:
- ✅ **Defined** in `ethernetip/ei_app_minimal.c` (line 95)
- ✅ **Declared** in `ethernetip/ei_app_minimal.h` (line 79)
- ❌ **NOT CALLED** anywhere in `enet_lwip.c` (main application)

---

## 🎯 WHERE IT SHOULD BE CALLED

### In File: `enet_lwip.c` → Function: `main()`

**Add it RIGHT AFTER this line:**
```c
lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);
```

**Add these lines:**
```c
#include "ethernetip/ei_app_minimal.h"

if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
    UARTprintf("EtherNet/IP initialization failed\n");
    while(1);
}
```

---

## 📍 EXACT LOCATION IN CODE FLOW

```
main()
  │
  ├─ System initialization (clocks, GPIO)
  ├─ SysTickPeriodSet() & SysTickEnable()
  ├─ lwIPInit()                        ← HAPPENS FIRST
  │
  ├─ LocatorInit()
  ├─ httpd_init()
  │
  ├─► 🆕 EI_APP_Init(NULL)              ← ADD HERE (after lwIPInit)
  │
  ├─ IntPrioritySet() setup
  │
  └─ while(1) main loop                ← Also call EI_APP_Process() inside
      while(g_bLED == false)
        EI_APP_Process()                ← AND ADD HERE (in idle loop)
```

---

## 🔍 VERIFICATION

**To confirm it's NOT being called:**
- Search in `enet_lwip.c` for "EI_APP_Init" → **No results** ❌
- It's only referenced in documentation and examples
- The function is ready but NOT integrated yet

**To confirm it exists:**
- View `ei_app_minimal.c` line 95 → Function definition ✅
- View `ei_app_minimal.h` line 79 → Function declaration ✅

---

## 📋 CALL DOCUMENTATION

**Function:** `EI_AppStatus_t EI_APP_Init(const EI_AppDeviceConfig_t *pDevConfig)`

| Aspect | Details |
|--------|---------|
| **Defined in** | `ethernetip/ei_app_minimal.c` |
| **Declared in** | `ethernetip/ei_app_minimal.h` |
| **Called from** | ❌ Nowhere (needs to be added to `enet_lwip.c`) |
| **Parameter** | `NULL` for defaults, or `&customConfig` for custom |
| **Returns** | `EI_APP_STATUS_OK` on success |
| **Called once or multiple times?** | Once at startup |
| **Thread-safe?** | Yes, from main thread only |
| **Prerequisite** | Must call `lwIPInit()` first |

---

## 🚀 ACTION REQUIRED

You need to:
1. Edit `enet_lwip.c`
2. Add the include and function call (3 lines of code)
3. Recompile
4. Test

**Files involved:**
- `enet_lwip.c` ← Modify (add call)
- `ei_app_minimal.c` ← Already has function
- `ei_app_minimal.h` ← Already has declaration

---

## 📞 CALL USAGE EXAMPLE

### How it's currently DOCUMENTED to be called:

From `COMPLETE_EXECUTION_FLOW.md` (example):
```c
lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);
LocatorInit();
httpd_init();

#include "ethernetip/ei_app_minimal.h"

if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
    UARTprintf("Failed to initialize EtherNet/IP\n");
    while(1);
}
```

### How it SHOULD be called in actual code:

Same as above - but in `enet_lwip.c` main function!

---

## 🎓 SUMMARY

| Question | Answer |
|----------|--------|
| Is `EI_APP_Init()` defined? | ✅ Yes, in `ei_app_minimal.c` line 95 |
| Is it declared? | ✅ Yes, in `ei_app_minimal.h` line 79 |
| Is it being called? | ❌ No, NOT called anywhere |
| Where should it be called? | In `enet_lwip.c` main(), after `lwIPInit()` |
| How many times? | Once at startup |
| With what parameter? | `NULL` for default config |
| What does it do? | Initializes EtherNet/IP abstraction layer |
| What's the return value? | `EI_APP_STATUS_OK` = success, error code = failure |

---

**Status:** Ready to be integrated into `enet_lwip.c`
