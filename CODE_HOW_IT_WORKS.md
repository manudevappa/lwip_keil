# HOW THE CODE WORKS - Complete Technical Deep Dive

**Date:** December 2, 2025  
**Project:** TM4C EtherNet/IP Integration  
**Focus:** Code execution flow, parsing functions, interrupt handling

---

## 🎯 OVERVIEW

```
┌─────────────────────────────────────────────────────────────────┐
│                    COMPLETE CODE FLOW                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│ 1. INITIALIZATION                                               │
│    ├─ main() starts                                             │
│    ├─ System setup (clocks, GPIO, UART)                         │
│    ├─ lwIPInit() - Initialize network stack                     │
│    └─ EI_APP_Init() - Initialize EtherNet/IP layer              │
│                                                                 │
│ 2. MAIN LOOP (Continuous)                                       │
│    ├─ Wait for g_bLED flag                                      │
│    ├─ EI_APP_Process() - Process work                           │
│    └─ Toggle LED                                                │
│                                                                 │
│ 3. INTERRUPTS (Asynchronous)                                    │
│    ├─ SysTick (100 Hz)                                          │
│    │   └─ Calls lwIPTimer()                                     │
│    │   └─ Sets g_bLED flag                                      │
│    │                                                             │
│    └─ Ethernet (On packet)                                      │
│        └─ MAC driver processes frame                            │
│        └─ lwIP stack parses packet                              │
│        └─ Routes to UDP:2222 if EtherNet/IP                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🏗️ PART 1: INITIALIZATION SEQUENCE

### Step 1: main() Starts

**File:** `enet_lwip.c`, lines 230-260

```c
int main(void)
{
    uint32_t ui32User0, ui32User1;
    uint8_t pui8MACArray[8];
    
    // [1] Configure oscillator
    SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);
    
    // [2] Set system clock to 120 MHz
    g_ui32SysClock = MAP_SysCtlClockFreqSet(..., 120000000);
    
    // [3] Configure device pins (GPIO)
    PinoutSet(true, false);
    
    // [4] Configure UART for debug (115200)
    UARTStdioConfig(0, 115200, g_ui32SysClock);
    
    // [5] Configure SysTick timer (100 Hz)
    MAP_SysTickPeriodSet(g_ui32SysClock / SYSTICKHZ);  // = 120MHz / 100
    MAP_SysTickEnable();
    MAP_SysTickIntEnable();
```

**What Happens:**
- System clock set to 120 MHz
- GPIO pins initialized
- UART ready for debug output
- SysTick timer configured to fire every 10ms (100 Hz)

---

### Step 2: lwIPInit() - Network Stack

**File:** `enet_lwip.c`, lines 296-297

```c
// Get MAC address from Flash
MAP_FlashUserGet(&ui32User0, &ui32User1);

// Convert to byte array
pui8MACArray[0] = ((ui32User0 >>  0) & 0xff);
pui8MACArray[1] = ((ui32User0 >>  8) & 0xff);
pui8MACArray[2] = ((ui32User0 >> 16) & 0xff);
pui8MACArray[3] = ((ui32User1 >>  0) & 0xff);
pui8MACArray[4] = ((ui32User1 >>  8) & 0xff);
pui8MACArray[5] = ((ui32User1 >> 16) & 0xff);

// Initialize lwIP with DHCP
lwIPInit(g_ui32SysClock,      // System clock = 120MHz
         pui8MACArray,         // MAC address from Flash
         0, 0, 0,              // IP config (0 = use DHCP)
         IPADDR_USE_DHCP);     // Enable DHCP
```

**What lwIPInit() Does:**
1. Creates network interface (`struct netif`)
2. Initializes Ethernet MAC hardware driver (TivaWare)
3. Sets MAC address in hardware
4. Enables DHCP client
5. Registers MAC interrupt handler (INT_EMAC0)
6. Enables Ethernet receiver

**Result:** Network stack ready, waiting for IP assignment

---

### Step 3: EI_APP_Init() - EtherNet/IP Layer

**File:** `enet_lwip.c`, lines 310-313

```c
if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
    UARTprintf("EtherNet/IP initialization failed\n");
    while(1);
}
```

**File:** `ei_app_minimal.c`, lines 95-185

```c
EI_AppStatus_t EI_APP_Init(const EI_AppDeviceConfig_t *pDevConfig)
{
    // [1] Check if already initialized
    if (g_bAppInitialized) {
        return EI_APP_STATUS_ERR_ALREADY_INIT;
    }
    
    // [2] Use provided config or defaults
    if (pDevConfig != NULL) {
        memcpy(&g_AppDeviceConfig, pDevConfig, sizeof(...));
    }
    
    // [3] Initialize board layer
    boardStatus = EI_TM4C_BoardInit();
    if (boardStatus != EI_BOARD_STATUS_OK) {
        return EI_APP_STATUS_ERR_INVALID_PARAM;
    }
    
    // [4] Initialize network interface layer
    netifStatus = EI_TM4C_NetifInit();
    if (netifStatus != EI_NETIF_STATUS_OK) {
        return EI_APP_STATUS_ERR_NETIF_FAILED;
    }
    
    // [5] Log device info
    EI_LOG("Device: %s", g_AppDeviceConfig.pcDeviceName);
    EI_LOG("Vendor ID: 0x%04X", g_AppDeviceConfig.ui16VendorID);
    EI_LOG("MAC: %02x:%02x:%02x:%02x:%02x:%02x", ...);
    EI_LOG("IP: %d.%d.%d.%d", ...);
    
    // [6] Set LED to BLINK mode
    EI_TM4C_LEDSetStatus(EI_LED_BLINK);
    
    // [7] Mark as initialized
    g_bAppInitialized = true;
    
    return EI_APP_STATUS_OK;
}
```

**Called Functions:**

#### A. EI_TM4C_BoardInit()

**File:** `ei_tm4c_board.c`, lines 107-138

```c
EI_BoardStatus_t EI_TM4C_BoardInit(void)
{
    if (g_bBoardInitialized) {
        return EI_BOARD_STATUS_OK;
    }
    
    // [1] Test GPIO is accessible (sanity check)
    volatile uint32_t ui32Test = MAP_GPIOPinRead(EI_CONFIG_LED_PORT, 
                                                  EI_CONFIG_LED_PIN);
    
    // [2] Initialize LED to OFF
    MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, 0);
    g_eLEDStatus = EI_LED_OFF;
    
    // [3] Calculate blink threshold
    // For 2 Hz blink with 100 Hz SysTick: (100 / 2) / 2 = 25
    if (g_ui32SysClock > 0) {
        g_ui32BlinkThreshold = (100 / g_ui32BlinkRateHz) / 2;
    }
    
    g_bBoardInitialized = true;
    
    return EI_BOARD_STATUS_OK;
}
```

**What it does:**
- Verifies LED GPIO is accessible
- Sets LED to OFF initially
- Calculates blink timing (2 Hz)

---

#### B. EI_TM4C_NetifInit()

**File:** `ei_tm4c_netif.c`, lines 124-160

```c
EI_NetifStatus_t EI_TM4C_NetifInit(void)
{
    // [1] Prevent duplicate init
    if (g_bInitialized) {
        return EI_NETIF_STATUS_OK;
    }
    
    // [2] Get lwIP network interface
    g_pNetif = NetifGetLwipInterface();
    //        ↓
    //        Calls: extern struct netif *netif_list;
    //        Returns: netif_list (created by lwIPInit)
    
    if (g_pNetif == NULL) {
        EI_LOG_ERROR("lwIP network interface not found");
        return EI_NETIF_STATUS_ERR_NO_NETIF;
    }
    
    // [3] Verify interface is up
    if (!netif_is_up(g_pNetif)) {
        EI_LOG_WARN("Network interface is not up yet");
    }
    
    // [4] Log MAC address
    NETIF_DEBUG_PRINT("MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                      g_pNetif->hwaddr[0], ...);
    
    // [5] Log IP information
    if (g_pNetif->ip_addr.addr != 0) {
        // IP assigned
    } else {
        // Waiting for DHCP
    }
    
    g_bInitialized = true;
    return EI_NETIF_STATUS_OK;
}
```

**Key Helper Function:**

```c
static struct netif* NetifGetLwipInterface(void)
{
    // Access lwIP's global network interface list
    extern struct netif *netif_list;  // ← Created by lwIPInit()
    
    if (netif_list != NULL) {
        return netif_list;  // Return the first (and usually only) netif
    }
    
    return NULL;
}
```

**What it does:**
- Gets network interface from lwIP (netif_list)
- Verifies interface is valid
- Reads and logs MAC address
- Checks IP address status

---

### Step 4: Interrupt Priority Setup

**File:** `enet_lwip.c`, lines 305-308

```c
// Set interrupt priorities
// SysTick at 0x80 (higher priority = processed first)
MAP_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

// Ethernet at 0xC0 (lower priority = can be preempted)
MAP_IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
```

**Priority Levels:**
```
Priority 0x80 (SysTick) ← Processed first (can preempt others)
Priority 0xC0 (Ethernet) ← Can be preempted by SysTick
```

---

## ⚡ PART 2: MAIN LOOP PROCESSING

### Main Loop Structure

**File:** `enet_lwip.c`, lines 318-338

```c
while(1)  // Continuous loop
{
    // ========================================
    // WAIT FOR SYSTICK FLAG
    // ========================================
    while(g_bLED == false)  // Sleep here until SysTick fires
    {
        // During this wait, interrupts can fire:
        // - SysTick: every 10ms (sets g_bLED = true)
        // - Ethernet: anytime a packet arrives
    }
    
    // ========================================
    // SYSTICK FIRED - PROCESS WORK
    // ========================================
    
    // Call EtherNet/IP processing
    EI_APP_Process();
    
    // ========================================
    // TOGGLE LED
    // ========================================
    g_bLED = false;  // Clear the flag
    
    MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,
                     (MAP_GPIOPinRead(...) ^ GPIO_PIN_1));  // Toggle
    
    // Loop back to wait
}
```

### EI_APP_Process() Function

**File:** `ei_app_minimal.c`, lines 187-224

```c
void EI_APP_Process(void)
{
    // [1] Check if initialized
    if (!g_bAppInitialized) {
        return;  // Not ready yet
    }
    
    // [2] Update board (LED blinking)
    EI_TM4C_BoardProcess();
    //  ↓
    //  Calls: BoardLEDUpdateBlink()
    
    // [3] Update status periodically
    static uint32_t ui32UpdateCounter = 0;
    ui32UpdateCounter++;
    
    if (ui32UpdateCounter >= 100) {  // Every ~1 second at 100 Hz
        ui32UpdateCounter = 0;
        AppUpdateStatusString();  // Build status string
        
        // Check network status
        bool bHasIP = false;
        if (EI_TM4C_NetifHasIP(&bHasIP) == EI_NETIF_STATUS_OK) {
            if (bHasIP) {
                // Network ready - could change LED to solid ON
            }
        }
    }
    
    // [4] TODO: EI_API processing when library integrated
}
```

**Helper Function: BoardLEDUpdateBlink()**

**File:** `ei_tm4c_board.c`, lines 65-88

```c
static void BoardLEDUpdateBlink(void)
{
    if (g_eLEDStatus != EI_LED_BLINK) {
        return;  // Not in blink mode
    }
    
    // Increment blink counter
    g_ui32BlinkCounter++;
    
    // When counter reaches threshold, toggle LED
    if (g_ui32BlinkCounter >= g_ui32BlinkThreshold) {
        g_ui32BlinkCounter = 0;  // Reset counter
        
        // Read current LED state
        uint32_t ui32CurrentState = MAP_GPIOPinRead(EI_CONFIG_LED_PORT, 
                                                     EI_CONFIG_LED_PIN);
        
        if (ui32CurrentState) {
            // LED is ON → turn it OFF
            MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, 0);
        } else {
            // LED is OFF → turn it ON
            MAP_GPIOPinWrite(EI_CONFIG_LED_PORT, EI_CONFIG_LED_PIN, 
                           EI_CONFIG_LED_PIN);
        }
    }
}
```

**Blink Calculation:**
```
SysTick frequency:     100 Hz (every 10ms)
Desired blink rate:    2 Hz (on/off each second)
Threshold calculation: (100 / 2) / 2 = 25 calls per toggle
Result:                Toggle LED every 25 calls = every 250ms = 2 Hz blink
```

---

## 🔌 PART 3: INTERRUPT HANDLERS

### Interrupt Handler 1: SysTick (Every 10ms)

**File:** `enet_lwip.c`, lines 207-221

```c
void SysTickIntHandler(void)
{
    // Priority: 0x80 (HIGH - can preempt Ethernet)
    // Called: Every 10ms (100 Hz)
    
    // ========================================
    // [1] PROCESS LWIP TIMERS
    // ========================================
    lwIPTimer(SYSTICKMS);  // = lwIPTimer(10)
    
    // What lwIPTimer() does:
    // ├─ Updates ARP timeouts
    // ├─ Updates TCP retransmission timers
    // ├─ Updates UDP timeouts
    // ├─ Updates DHCP state machine
    // └─ May trigger packet processing
    
    // ========================================
    // [2] SIGNAL MAIN LOOP
    // ========================================
    g_bLED = true;  // Tell main loop to process
    
    // Main loop is waiting in: while(g_bLED == false)
    // Now that g_bLED = true, it will wake up and process
}
```

**Function Chain:**
```
SysTickIntHandler()
    ↓
lwIPTimer(10)
    ├─ sys_now() - Get current time
    ├─ arp_tmr() - ARP timeout
    ├─ ip_reass_tmr() - IP reassembly
    ├─ dhcp_coarse_tmr() - DHCP coarse
    ├─ dhcp_fine_tmr() - DHCP fine
    ├─ tcp_tmr() - TCP timeouts
    ├─ tcp_slowtmr() - TCP slow
    └─ igmp_tmr() - IGMP timeout
    
    ↓
    May trigger:
    ├─ IP packet processing
    ├─ TCP packet processing
    └─ UDP packet processing
```

---

### Interrupt Handler 2: Ethernet MAC (INT_EMAC0)

**File:** TivaWare (ethernetif.c - in lwIP driver)

```c
// Note: This is NOT in enet_lwip.c - it's in TivaWare library
// Registered by: lwIPInit()

void EMAC0_IRQHandler(void)  // Or ethernetif_input()
{
    // Priority: 0xC0 (LOW - can be preempted by SysTick)
    // Called: Whenever Ethernet frame arrives on wire
    
    // ========================================
    // [1] MAC DRIVER RECEIVES FRAME
    // ========================================
    // TivaWare MAC driver code (in lwiplib):
    uint32_t ui32Status = MAP_EMACIntStatus(EMAC0_BASE, false);
    
    if (ui32Status & EMAC_INT_RECEIVE) {
        // Frame received - transfer via DMA to RAM buffer
        
        // ========================================
        // [2] CREATE PACKET BUFFER (pbuf)
        // ========================================
        // lwIP creates a packet buffer pointing to RAM frame data
        struct pbuf *p = low_level_input(netif);
        
        // ========================================
        // [3] CALL LWIP ETHERNET INPUT FUNCTION
        // ========================================
        if (p != NULL) {
            ethernet_input(p, netif);  // ← lwIP packet processing
            //          ↓
            //          Calls: netif->input(p, netif)
        }
    }
}
```

**lwIP Packet Processing Chain:**

```
ethernet_input(pbuf)  [Ethernet Layer]
    ├─ Parse Ethernet frame header
    ├─ Check destination MAC (matches ours?)
    ├─ Check EtherType
    │   ├─ 0x0800 = IPv4
    │   ├─ 0x0806 = ARP
    │   └─ etc.
    └─ Call appropriate handler
         ↓
         
If IPv4:
    ip_input(pbuf)  [IP Layer]
    ├─ Parse IP header
    ├─ Check destination IP (matches ours?)
    ├─ Check protocol field
    │   ├─ 6 = TCP
    │   ├─ 17 = UDP
    │   └─ etc.
    └─ Call appropriate handler
         ↓
         
If UDP:
    udp_input(pbuf)  [UDP Layer]
    ├─ Parse UDP header
    ├─ Check destination port
    │   ├─ 2222 = EtherNet/IP ← RELEVANT!
    │   ├─ 80 = HTTP
    │   ├─ 5353 = mDNS
    │   └─ etc.
    └─ Call registered callback for port 2222
         ↓
         
If Port 2222:
    EI_API_Callback(pbuf)  [EtherNet/IP Layer]
    ├─ Parse EtherNet/IP encapsulation header
    ├─ Check Command field
    │   ├─ 0x0065 = SEND_RR_DATA (Explicit)
    │   ├─ 0x0066 = SEND_UNIT_DATA (Implicit)
    │   └─ etc.
    └─ Process command
         ├─ Extract CIP message
         ├─ Route to appropriate handler
         ├─ Generate response
         └─ Queue response for transmission
```

---

## 🔍 PART 4: PACKET PARSING FUNCTIONS

### Parsing Hierarchy

```
┌─────────────────────────────────┐
│  ETHERNET FRAME (60-1514 bytes) │
├─────────────────────────────────┤
│ Dest MAC (6)  │ Src MAC (6)     │
│ EtherType (2) │ Data...         │
└─────────────────────────────────┘
         ↓ ethernet_input()
┌─────────────────────────────────┐
│  IP PACKET                      │
├─────────────────────────────────┤
│ Version/IHL (1)                 │
│ Type of Service (1)             │
│ Total Length (2)                │
│ Identification (2)              │
│ Flags/Fragment Offset (2)       │
│ TTL (1) │ Protocol (1)         │
│ Checksum (2)                    │
│ Source IP (4) │ Dest IP (4)    │
│ Options (0-40)                  │
│ Data...                         │
└─────────────────────────────────┘
         ↓ ip_input()
┌─────────────────────────────────┐
│  UDP PACKET                     │
├─────────────────────────────────┤
│ Source Port (2)                 │
│ Dest Port (2)  ← Check = 2222?  │
│ Length (2)                      │
│ Checksum (2)                    │
│ Data...                         │
└─────────────────────────────────┘
         ↓ udp_input() / Port 2222 callback
┌─────────────────────────────────┐
│  ETHERNETIP ENCAPSULATION       │
├─────────────────────────────────┤
│ Command (2)   ← 0x0065 or 0x0066│
│ Length (2)                      │
│ Session Handle (4)              │
│ Status (4)                      │
│ Options (4)                     │
│ Data (CIP Message)              │
└─────────────────────────────────┘
         ↓ EI_API Callback
┌─────────────────────────────────┐
│  CIP MESSAGE                    │
├─────────────────────────────────┤
│ Service (1)                     │
│ Path (variable)                 │
│ │  ├─ Class Code                 │
│ │  ├─ Instance Number            │
│ │  └─ Attribute Number           │
│ Request/Response Data           │
└─────────────────────────────────┘
```

---

### Parsing Functions Used

#### 1. ethernet_input() [lwIP]

**What it does:**
- Reads Ethernet frame header
- Extracts source/destination MAC
- Reads EtherType field
- Routes to IP handler if IPv4

**Code Location:** lwIP library (lwiplib.c)

#### 2. ip_input() [lwIP]

**What it does:**
- Reads IP header
- Checks destination IP
- Extracts protocol field (6=TCP, 17=UDP)
- Routes to appropriate layer handler

**Code Location:** lwIP library (lwiplib.c)

#### 3. udp_input() [lwIP]

**What it does:**
- Reads UDP header
- Extracts destination port
- Looks up registered callback for that port
- Calls callback function

**Code Location:** lwIP library (lwiplib.c)

#### 4. EI_API_Callback() [Your Code - Placeholder]

**Currently:** Would be in EI_API library (when integrated)

**What it should do:**
```c
void EI_API_Callback(struct pbuf *p, struct udp_pcb *pcb, const ip_addr_t *addr)
{
    // Parse EtherNet/IP header from pbuf
    uint8_t *pData = p->payload;
    uint16_t ui16Command = (pData[0] | (pData[1] << 8));  // Little-endian
    
    if (ui16Command == 0x0065) {  // SEND_RR_DATA (Explicit)
        // Parse EtherNet/IP explicit message
        // Extract CIP message
        // Route to handler
        // Send response
    }
    else if (ui16Command == 0x0066) {  // SEND_UNIT_DATA (Implicit)
        // Parse EtherNet/IP implicit message
        // Extract assembly data
        // Call user callback
        // Send response
    }
    
    pbuf_free(p);  // Free the buffer
}
```

---

## 📊 COMPLETE FUNCTION CALL TREE

```
main()
├─ SysCtlMOSCConfigSet()
├─ SysCtlClockFreqSet()
├─ PinoutSet()
├─ UARTStdioConfig()
├─ SysTickPeriodSet()
├─ SysTickEnable()
├─ SysTickIntEnable()
├─ FlashUserGet()
├─ lwIPInit()
│   ├─ [TivaWare/lwIP]
│   ├─ Creates struct netif
│   ├─ Initializes MAC driver
│   ├─ Registers INT_EMAC0 handler
│   └─ Starts DHCP
│
├─ LocatorInit()
├─ LocatorMACAddrSet()
├─ httpd_init()
│
├─ IntPrioritySet(INT_EMAC0)
├─ IntPrioritySet(FAULT_SYSTICK)
│
├─ EI_APP_Init(NULL)
│   ├─ EI_TM4C_BoardInit()
│   │   └─ BoardLEDUpdateBlink() calculation
│   ├─ EI_TM4C_NetifInit()
│   │   └─ NetifGetLwipInterface()
│   │       └─ extern struct netif *netif_list
│   ├─ EI_TM4C_NetifGetInfo()
│   ├─ EI_TM4C_LEDSetStatus()
│   └─ Log device info
│
└─ while(1)
    ├─ while(g_bLED == false)  // Wait for SysTick
    │
    ├─ EI_APP_Process()
    │   ├─ EI_TM4C_BoardProcess()
    │   │   └─ BoardLEDUpdateBlink()
    │   │       ├─ Increment g_ui32BlinkCounter
    │   │       ├─ Check if >= g_ui32BlinkThreshold
    │   │       └─ Toggle LED via GPIOPinWrite()
    │   │
    │   ├─ AppUpdateStatusString()
    │   │   ├─ EI_TM4C_NetifIsUp()
    │   │   ├─ EI_TM4C_NetifHasIP()
    │   │   └─ usprintf() - Build status
    │   │
    │   └─ EI_TM4C_NetifHasIP()
    │
    ├─ g_bLED = false
    └─ GPIOPinWrite() - Toggle LED


INTERRUPTS (Asynchronous):

SysTickIntHandler()  [Every 10ms]
├─ lwIPTimer(10)
│   ├─ sys_now()
│   ├─ arp_tmr()
│   ├─ dhcp_coarse_tmr()
│   ├─ dhcp_fine_tmr()
│   ├─ tcp_tmr()
│   └─ [May process packets]
└─ g_bLED = true

INT_EMAC0_Handler()  [On packet]
├─ MAC driver reads frame
├─ ethernet_input(pbuf)
│   ├─ Parse Ethernet header
│   └─ ip_input(pbuf)
│       ├─ Parse IP header
│       └─ udp_input(pbuf)
│           ├─ Parse UDP header
│           └─ if (port == 2222)
│               └─ EI_API_Callback(pbuf)
│                   ├─ Parse EtherNet/IP header
│                   ├─ Check command (0x0065 or 0x0066)
│                   ├─ Process message
│                   └─ Queue response (udp_send)
│
└─ Resume main loop
```

---

## 📝 PARSING BREAKDOWN

### What Gets Parsed Where?

| Layer | Function | What's Parsed | Location |
|-------|----------|---------------|----------|
| **Physical** | MAC driver | Frame bits | TivaWare (hardware) |
| **Ethernet** | ethernet_input() | MAC, EtherType | lwIP library |
| **IP** | ip_input() | IP header, protocol | lwIP library |
| **Transport** | udp_input() | UDP header, port | lwIP library |
| **EtherNet/IP** | EI_API_Callback() | Encapsulation header | (EI_API library) |
| **CIP** | CIP handler | Service, path, data | (EI_API library) |

---

## 🎯 INTERRUPT FLOW SUMMARY

```
PACKET ARRIVES ON WIRE
    ↓
MAC PHY detects frame
    ↓
MAC controller DMA transfers to RAM
    ↓
INT_EMAC0 fires (Priority 0xC0)
    ↓
CHECK: Can SysTick interrupt this?
    YES if SysTick fires (Priority 0x80 > 0xC0)
    NO otherwise
    ↓
TivaWare MAC driver
├─ Reads frame from buffer
└─ Calls lwIP ethernet_input()
    ↓
lwIP Ethernet Layer
├─ Parses Ethernet header
├─ Checks destination MAC
└─ Routes to IP layer
    ↓
lwIP IP Layer
├─ Parses IP header
├─ Checks destination IP
└─ Routes to UDP layer
    ↓
lwIP UDP Layer
├─ Parses UDP header
├─ Checks destination port
├─ Port 2222? YES
└─ Calls EI_API_Callback()
    ↓
EtherNet/IP Layer (when EI_API integrated)
├─ Parses EtherNet/IP encapsulation
├─ Checks command (0x0065 or 0x0066)
├─ Processes message
├─ Builds response
└─ Queues response (udp_send)
    ↓
lwIP processes response when appropriate
├─ Adds UDP header
├─ Adds IP header
├─ Adds Ethernet header
└─ Transfers to MAC TX buffer
    ↓
MAC controller transmits frame on wire
    ↓
Scanner/Controller receives response
```

---

## ✅ SUMMARY

### Key Points:

1. **lwIP does most parsing:**
   - ethernet_input() → Ethernet layer
   - ip_input() → IP layer
   - udp_input() → UDP layer + port routing

2. **All automatic:**
   - lwIP handles packet parsing
   - Automatic routing to port 2222 when available
   - Response queuing automatic

3. **Interrupt-driven:**
   - SysTick every 10ms (lwIP timers)
   - Ethernet on packet (packet processing)
   - Proper priority (SysTick > Ethernet)

4. **Main loop is simple:**
   - Wait for SysTick flag
   - Process EI_APP_Process()
   - Toggle LED
   - Repeat

5. **Ready for EI_API integration:**
   - All low-level code in place
   - Just add EI_API_Callback() when library available
   - Will handle CIP parsing and device callbacks

