# Complete Ethernet Data Flow & EtherNet/IP Integration Path

**Date:** December 2, 2025  
**Project:** TM4C EtherNet/IP Integration  
**File:** Detailed Execution & Data Flow Documentation

---

## 🔄 COMPLETE EXECUTION PATH: From main() to EtherNet/IP Stack

### STARTUP SEQUENCE (One-time initialization)

```
┌─────────────────────────────────────────────────────────────────┐
│ MAIN FUNCTION EXECUTION                                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│ 1. main() STARTS                                               │
│    ├─ SysCtlMOSCConfigSet()        [System Clock Setup]        │
│    ├─ SysCtlClockFreqSet(120MHz)   [Set CPU to 120 MHz]        │
│    ├─ PinoutSet(true, false)       [Configure GPIO pins]       │
│    └─ UARTStdioConfig()            [Setup serial debug]        │
│                                                                 │
│ 2. HARDWARE INITIALIZATION                                     │
│    ├─ SysTickPeriodSet()           [100 Hz timer]              │
│    ├─ SysTickEnable()              [Enable timer interrupts]   │
│    ├─ FlashUserGet()               [Read MAC from Flash]       │
│    └─ GPIOPinTypeGPIOOutput()      [Configure LED]             │
│                                                                 │
│ 3. NETWORK STACK INITIALIZATION                                │
│    │                                                            │
│    ├──> lwIPInit(clock, MAC, ...)                              │
│    │    └─ Initializes lwIP stack                              │
│    │       ├─ Ethernet MAC driver                              │
│    │       ├─ UDP/TCP protocol                                 │
│    │       ├─ DHCP client (gets IP)                            │
│    │       └─ Creates netif structure                          │
│    │                                                            │
│    ├──> LocatorInit()               [Device discovery]         │
│    ├──> httpd_init()                [Web server]               │
│    │                                                            │
│    └──> 🆕 EI_APP_Init(NULL)         [ETHERNETIP INIT]         │
│         └─ Initializes EtherNet/IP abstraction layer           │
│            ├─ EI_TM4C_BoardInit()   [LED, GPIO setup]          │
│            ├─ EI_TM4C_NetifInit()   [Get lwIP netif]           │
│            └─ Sets device config (VendorID, etc.)              │
│                                                                 │
│ 4. INTERRUPT SETUP                                             │
│    ├─ IntPrioritySet(INT_EMAC0)     [Ethernet interrupt]       │
│    └─ IntPrioritySet(FAULT_SYSTICK) [Timer interrupt]          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## ⚡ RUNTIME EXECUTION: Main Loop & Interrupts

### MAIN LOOP (Continuous Operation)

```
┌───────────────────────────────────────────────────────────────────┐
│ MAIN LOOP: while(1)                                              │
├───────────────────────────────────────────────────────────────────┤
│                                                                   │
│ BLOCKING WAIT:                                                   │
│ ├─> while(g_bLED == false) { }        [Wait for SysTick timer]   │
│ │                                                                │
│ │   (During this wait, interrupts are processed - see below)   │
│ │                                                                │
│ └─> 🆕 EI_APP_Process()                 [ETHERNETIP PROCESSING] │
│     └─ Called periodically in idle loop                          │
│        ├─ EI_TM4C_BoardProcess()      [Update LED blink]        │
│        └─ (Placeholder for EI_API stack processing)             │
│                                                                  │
│ LED TOGGLE:                                                      │
│ └─> GPIOPinWrite()                    [Toggle LED output]       │
│                                                                  │
│ (Loop repeats ~100 times per second = 10ms per cycle)           │
│                                                                  │
└───────────────────────────────────────────────────────────────────┘
```

---

## 🔌 INTERRUPT-DRIVEN ETHERNET DATA FLOW

### SysTick Interrupt (Every 10ms @ 100Hz)

```
┌──────────────────────────────────────────────────────────────────┐
│ SysTickIntHandler() - Called every 10ms                          │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│ ISR 1: PROCESS LWIP TIMERS                                      │
│ └─> lwIPTimer(10)                                                │
│     ├─ Updates lwIP internal timers                              │
│     ├─ Processes lwIP timeouts                                   │
│     │  ├─ ARP timeouts                                           │
│     │  ├─ TCP retransmissions                                    │
│     │  └─ UDP timeouts                                           │
│     └─ May trigger packet processing                             │
│                                                                  │
│ ISR 2: SET LED BLINK FLAG                                       │
│ └─> g_bLED = true                                                │
│     └─ Signals main loop to toggle LED                           │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### Ethernet Interrupt (When packet arrives)

```
┌────────────────────────────────────────────────────────────────────┐
│ ETHERNET_INT_HANDLER (Priority: 0xC0)                             │
│ (Triggered by MAC when frame received)                            │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│ 1. MAC DRIVER RECEIVES FRAME                                      │
│    └─> TivaWare Ethernet MAC driver                               │
│        ├─ DMA transfers frame to memory buffer                    │
│        └─ Invokes lwIP handler                                    │
│                                                                    │
│ 2. LWIP PROCESSES PACKET                                          │
│    └─> lwIP MAC input function (in lwiplib)                       │
│        ├─ Reads frame from buffer                                 │
│        ├─ Checks destination MAC                                  │
│        ├─ Routes to appropriate handler:                          │
│        │  ├─ Ethernet frame → IP layer                            │
│        │  └─ IP packet → UDP/TCP/ARP/etc.                        │
│        │                                                          │
│        └─ PACKET ROUTING:                                         │
│           ├─ ARP frame    → ARP handler                           │
│           ├─ IP UDP port 2222  → 🆕 EtherNet/IP (ENCAP)          │
│           ├─ IP TCP port 44818 → 🆕 EtherNet/IP (EXPLICIT MSG)   │
│           ├─ IP UDP port 5353  → mDNS (if enabled)               │
│           ├─ IP TCP port 80    → HTTP (web server)               │
│           └─ IP TCP port 44818 → HTTP (locator)                  │
│                                                                    │
│ 3. 🆕 ETHERNETIP PACKET RECEIVED                                  │
│    └─ EI_API receives UDP/2222 or TCP/44818 packet                │
│       ├─ Parses EtherNet/IP encapsulation header                  │
│       ├─ Extracts CIP payload                                     │
│       ├─ Routes to appropriate CIP handler:                       │
│       │  ├─ CIP Class/Instance/Attribute                          │
│       │  ├─ Implicit messaging (scheduled I/O)                    │
│       │  ├─ Explicit messaging (command/response)                 │
│       │  └─ Connection management                                 │
│       │                                                            │
│       └─ Generates response packet                                │
│          └─ (Sent back via lwIP UDP/TCP)                          │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

---

## 📊 COMPLETE PACKET FLOW DIAGRAM

### Incoming EtherNet/IP Packet Journey

```
PHYSICAL MEDIUM (Ethernet Cable)
        ↓
        │
        ├─ Electrical signals (Manchester encoding)
        ├─ Frame arrives at PHY (88E1512)
        │
        ↓
MAC LAYER (TivaWare Ethernet Driver)
        │
        ├─ MAC recognizes destination (matches programmed MAC)
        ├─ DMA transfers frame to RAM buffer
        ├─ Triggers EMAC interrupt (INT_EMAC0)
        │
        ↓
LWIP NETWORK STACK (in Ethernet Interrupt Context)
        │
        ├─ lwipif_input() - MAC input handler
        ├─ Parses Ethernet frame:
        │  ├─ Destination MAC: matches (destination)
        │  ├─ Source MAC: note sender
        │  ├─ EtherType: IP (0x0800)
        │
        ├─ Calls ip_input() - IP layer processing
        ├─ Parses IP header:
        │  ├─ Destination IP: matches (DHCP assigned)
        │  ├─ Protocol: UDP (17)
        │
        ├─ Calls udp_input() - UDP layer
        ├─ Parses UDP header:
        │  ├─ Destination port: 2222 (EtherNet/IP)
        │  ├─ Finds registered UDP callback
        │
        ↓
🆕 ETHERNETIP LAYER (EI_API Library)
        │
        ├─ EI_API_RUN() processes packet
        ├─ Checks EtherNet/IP encapsulation header:
        │  ├─ Command ID
        │  ├─ Length
        │  ├─ Session handle
        │  ├─ Status
        │  └─ Options
        │
        ├─ Extracts CIP payload (encapsulated data)
        ├─ Routes by command type:
        │
        ├── REGISTER_SESSION
        │  └─ Allocates session, responds with session handle
        │
        ├── UNREGISTER_SESSION
        │  └─ Closes session
        │
        ├── SEND_RR_DATA (Explicit Messaging)
        │  └─ ┌─────────────────────────────────────┐
        │     │ CIP MESSAGE PROCESSING              │
        │     ├─ Class: 0x20 (Device Level)        │
        │     ├─ Instance: 0x24 (Identity Object) │
        │     ├─ Attribute: 1,2,3,... (VendorID, etc)
        │     │                                    │
        │     └─ Device Profile Handler            │
        │        └─ Your I/O callbacks here        │
        │
        ├── SEND_UNIT_DATA (Implicit Messaging)
        │  └─ Scheduled I/O data transfer
        │     ├─ Updates input assembly
        │     ├─ Calls I/O callback
        │     └─ Returns output assembly
        │
        └─ GENERATES RESPONSE
           └─ Encapsulates CIP response
              └─ Sends via UDP/IP/Ethernet back

OUTGOING RESPONSE:
        ↓
lwIP SENDS RESPONSE
        │
        ├─ udp_send() queues response
        ├─ ip_output() adds IP header
        ├─ etharp_output() resolves MAC
        ├─ netif->linkoutput() adds Ethernet header
        │
        ↓
MAC DRIVER TRANSMITS
        │
        ├─ TivaWare places frame in TX buffer
        ├─ MAC controller transmits on wire
        ├─ Physical layer sends electrical signals
        │
        ↓
PHYSICAL MEDIUM (Ethernet Cable)
        └─ Response reaches scanner/controller
```

---

## 🔀 DETAILED CODE PATH: Initialization

### Step-by-Step from main()

```c
main()
  │
  ├─ [1] System Setup
  │   ├─ SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ)
  │   ├─ g_ui32SysClock = SysCtlClockFreqSet(..., 120MHz)
  │   │   └─ Sets global: g_ui32SysClock = 120000000
  │   ├─ PinoutSet(true, false)
  │   │   └─ Configures all GPIO pins (Ethernet, LED, UART)
  │   └─ UARTStdioConfig(0, 115200, g_ui32SysClock)
  │       └─ Debug UART ready
  │
  ├─ [2] Timer & LED Setup
  │   ├─ GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1)
  │   │   └─ Port N, Pin 1 = output
  │   ├─ SysTickPeriodSet(g_ui32SysClock / 100)
  │   │   └─ = 1200000 cycles = 10ms period
  │   ├─ SysTickEnable()
  │   └─ SysTickIntEnable()
  │       └─ Enables SysTickIntHandler() every 10ms
  │
  ├─ [3] MAC Address Retrieval
  │   ├─ FlashUserGet(&ui32User0, &ui32User1)
  │   │   └─ Reads MAC from Flash USER registers
  │   └─ Convert to pui8MACArray[6]
  │       └─ pui8MACArray[] = { 0x88, 0x0C, 0xE0, 0x5C, 0x46, 0xE9 }
  │
  ├─ [4] 🔑 LWIP INITIALIZATION
  │   └─ lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP)
  │       │
  │       ├─ Creates network interface (netif)
  │       ├─ Initializes Ethernet MAC driver
  │       ├─ Sets MAC address in hardware
  │       ├─ Starts DHCP client
  │       ├─ Registers MAC interrupt handler
  │       │   └─ Attached to INT_EMAC0
  │       │       └─ Calls lwipif_input() on frame received
  │       │
  │       ├─ Enables Ethernet MAC receiver
  │       └─ lwIP READY TO RECEIVE PACKETS
  │           └─ netif created: gNetif
  │               ├─ ip_addr = (waiting for DHCP)
  │               ├─ hwaddr = MAC from Flash
  │               ├─ state = running
  │               └─ flags = UP | RUNNING | DHCP
  │
  ├─ [5] Additional Services
  │   ├─ LocatorInit()
  │   │   └─ Device discovery service
  │   ├─ LocatorMACAddrSet(pui8MACArray)
  │   │   └─ Advertises MAC address
  │   └─ httpd_init()
  │       └─ Web server initialization (optional)
  │
  ├─ [6] 🆕 ETHERNETIP INITIALIZATION
  │   └─ EI_APP_Init(NULL)
  │       │
  │       ├─ EI_TM4C_BoardInit()
  │       │   ├─ GPIO for LED already configured
  │       │   └─ Sets LED to OFF
  │       │
  │       ├─ EI_TM4C_NetifInit()
  │       │   ├─ Gets netif from lwIP
  │       │   ├─ Reads MAC: pui8MAC = { 0x88, 0x0C, 0xE0, ... }
  │       │   ├─ Reads IP: pui32IP = 0 (waiting for DHCP)
  │       │   └─ Status: DOWN (no link yet)
  │       │
  │       └─ EI_APP_IsInitialized() = true
  │           └─ 🆕 ETHERNETIP READY TO PROCESS
  │
  ├─ [7] Interrupt Priority Setup
  │   ├─ IntPrioritySet(INT_EMAC0, 0xC0)
  │   │   └─ Ethernet: priority 3 (lower priority)
  │   └─ IntPrioritySet(FAULT_SYSTICK, 0x80)
  │       └─ SysTick: priority 2 (higher priority)
  │
  └─ [8] MAIN LOOP BEGINS
      └─ while(1) { ... }
```

---

## 📝 DETAILED CODE PATH: Main Loop & Runtime

### Continuous Operation

```c
while(1)                                           // Repeat forever
{
    // WAIT FOR SYSTICK INTERRUPT
    while(g_bLED == false)                         // Sleep until timer fires
    {
        // During this wait (every 10ms):
        // ─────────────────────────────────────────────────────────
        
        // 🆕 CALL ETHERNETIP PROCESSING
        EI_APP_Process()
        {
            EI_TM4C_BoardProcess()
            {
                BoardLEDUpdateBlink()               // Update LED state
            }
            
            // (Placeholder for actual EI_API processing when integrated)
            // EI_API_RUN(adapter, cipNode)        // Process stack
            //   └─ Handles queued packets
            //   └─ Updates I/O
            //   └─ Manages connections
        }
        
        // ETHERNET INTERRUPT CAN FIRE AT ANY TIME
        // ─────────────────────────────────────────────────────────
        
        // If packet arrives on wire:
        //   ├─ INT_EMAC0 interrupt fires (priority 0xC0)
        //   ├─ MAC DMA transfers frame to buffer
        //   ├─ lwipif_input() called
        //   ├─ Packet routed to UDP port 2222 (if EtherNet/IP)
        //   └─ EI_API processes and queues response
        //       └─ (EI_APP_Process() will send it next iteration)
        
        // SYSTICK INTERRUPT FIRES EVERY 10ms
        // ─────────────────────────────────────────────────────────
        // (Processor pauses whatever it's doing)
        // SysTickIntHandler()
        // {
        //   lwIPTimer(10)           // Update lwIP timers
        //   g_bLED = true           // Signal main loop
        // }
        // (Processor resumes from where it paused)
    }
    
    // SYSTICK FIRED - Exit wait loop
    g_bLED = false                                 // Clear flag
    
    // TOGGLE LED
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,    // Toggle output
                 (GPIOPinRead(...) ^ GPIO_PIN_1))
    
    // Loop back to wait
}
```

---

## 🎯 EXACT INTEGRATION POINTS FOR ETHERNETIP

### Point 1: Initialization (After lwIPInit)

**Location:** enet_lwip.c, main() function, after lwIPInit()

```c
// Current code (line ~350):
lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);

// 🆕 ADD THIS:
#include "ethernetip/ei_tm4c_config.h"
#include "ethernetip/ei_app_minimal.h"

if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
    UARTprintf("Failed to initialize EtherNet/IP\n");
    while(1);
}
```

**What happens:**
- EI_APP_Init() initializes board & network interface
- Reads MAC from lwIP
- Gets IP address (0 initially, updated when DHCP assigns)
- Sets up LED to indicate device status

---

### Point 2: Main Loop Processing (In idle wait)

**Location:** enet_lwip.c, main() function, in while(1) loop, inside while(g_bLED==false)

```c
// Current code (lines ~355-358):
while(1)
{
    while(g_bLED == false)
    {
        // 🆕 ADD THIS - Process EtherNet/IP stack
        EI_APP_Process();
    }
    
    g_bLED = false;
    
    // Toggle LED...
}
```

**What happens every iteration (10ms cycle):**
- EI_APP_Process() is called many times
- Checks network status
- Would process EI_API stack work
- Updates LED if needed

---

### Point 3: Packet Reception (Interrupt-driven, automatic)

**Location:** In Ethernet interrupt (automatic, no code changes needed)

**Flow:**
```
Frame arrives on wire
    ↓
INT_EMAC0 fires (Ethernet interrupt)
    ├─ MAC driver in TivaWare handles it
    ├─ lwipif_input() processes the frame
    ├─ IP layer checks destination
    ├─ UDP layer checks port 2222
    ├─ Finds EI_API registered handler
    └─ EI_API processes the packet
        ├─ Parses EtherNet/IP header
        ├─ Extracts CIP message
        ├─ Routes to appropriate handler
        └─ Queues response
```

Response will be sent automatically when:
- Next EI_APP_Process() call
- OR on next packet that requires immediate response

---

## 🔍 DATA FLOW IN DETAIL

### Scenario: EtherNet/IP Scanner sends "GetStatus" request

```
┌─ SCANNER (Industrial Automation Device)
│  └─ Sends UDP packet to:
│     └─ Destination IP: 192.168.1.100 (DHCP assigned to TM4C)
│        Destination Port: 2222 (EtherNet/IP)
│        Payload: EtherNet/IP encapsulated CIP message
│

│ ┌─ TRANSMISSION (Wire → TM4C)
│ │
├─► FRAME ARRIVES ON ETHERNET CABLE
│   └─ Physical layer signals (Manchester encoded)
│
├─► MAC CONTROLLER RECEIVES (in TM4C)
│   ├─ PHY passes frame to MAC
│   ├─ MAC checks destination (matches our MAC)
│   ├─ DMA transfers 1514 bytes to RAM buffer
│   ├─ INT_EMAC0 interrupt fires
│   └─ Processor pauses current instruction
│
├─► INT_EMAC0 HANDLER (Priority: 0xC0, HIGH)
│   │ (This preempts main loop!)
│   │
│   ├─ MAC Driver (TivaWare)
│   │  └─ ethernetif_input() in lwip/contrib/port/tiva
│   │     ├─ Reads frame from DMA buffer
│   │     └─ Calls ethernet_input(pbuf)
│   │
│   ├─ LWIP ETHERNET LAYER
│   │  └─ ethernetif.c:ethernet_input()
│   │     ├─ Parses Ethernet header
│   │     │  ├─ Dest MAC: matches (ours)
│   │     │  ├─ Src MAC: remember (scanner)
│   │     │  └─ EtherType: 0x0800 (IP)
│   │     │
│   │     └─ Calls ip_input(pbuf)
│   │
│   ├─ LWIP IP LAYER
│   │  └─ ip.c:ip_input()
│   │     ├─ Parses IP header
│   │     │  ├─ Dest IP: 192.168.1.100 (matches ours)
│   │     │  ├─ Src IP: 192.168.1.50 (scanner)
│   │     │  └─ Protocol: 17 (UDP)
│   │     │
│   │     └─ Calls udp_input(pbuf)
│   │
│   ├─ LWIP UDP LAYER
│   │  └─ udp.c:udp_input()
│   │     ├─ Parses UDP header
│   │     │  ├─ Dest Port: 2222 (EtherNet/IP)
│   │     │  ├─ Src Port: 52000 (scanner)
│   │     │  ├─ Checksum: verify
│   │     │  └─ Length: ~200 bytes
│   │     │
│   │     ├─ Looks up PCB (Protocol Control Block)
│   │     │  └─ Finds UDP socket listening on 2222
│   │     │     └─ Registered by EI_API during init
│   │     │
│   │     └─ Calls registered callback (EI_API)
│   │
│   ├─► 🆕 ETHERNETIP LAYER
│   │   └─ EI_API_RUN() is called
│   │      (Or: EI_API UDP callback invoked)
│   │
│   │      ├─ PARSE ENCAPSULATION HEADER
│   │      │  └─ EtherNet/IP encapsulation.c
│   │      │     ├─ Command: SEND_RR_DATA (0x0065)
│   │      │     ├─ Command length: ~180
│   │      │     ├─ Session handle: (scanner's session)
│   │      │     ├─ Status: 0 (no error)
│   │      │     └─ Options: 0
│   │      │
│   │      ├─ EXTRACT CIP MESSAGE
│   │      │  └─ CIP.c
│   │      │     ├─ Service: Get_Attributes (0x0E)
│   │      │     ├─ Path: /20/24/01 (Device/Identity/Attribute 1)
│   │      │     └─ (Requesting Vendor ID)
│   │      │
│   │      ├─ ROUTE TO HANDLER
│   │      │  └─ device_profile_intf.c
│   │      │     ├─ Class 0x20 = Device
│   │      │     ├─ Instance 0x24 = Identity Object
│   │      │     └─ Calls your callback:
│   │      │        ├─ DEVICE_PROFILE_GetAttribute()
│   │      │        │
│   │      │        └─ ATTRIBUTE 1: Vendor ID
│   │      │           └─ Returns: 0x0186 (Texas Instruments)
│   │      │
│   │      ├─ BUILD RESPONSE
│   │      │  └─ Encapsulates CIP response
│   │      │     └─ Status: 0 (success)
│   │      │     └─ Data: { 0x86, 0x01 } (Vendor ID)
│   │      │
│   │      ├─ QUEUE RESPONSE PACKET
│   │      │  └─ Adds to transmit queue
│   │      │     ├─ EtherNet/IP encapsulation header
│   │      │     ├─ CIP response data
│   │      │     └─ UDP/IP/Ethernet headers
│   │      │
│   │      └─ RESUME FROM INTERRUPT
│   │
│   └─ (Processor resumes from where it paused)
│
├─► MAIN LOOP CONTINUES
│   └─ (In while(g_bLED==false) or processing LEDs)
│
├─► NEXT SYSTEM TICK (10ms later)
│   └─ SysTickIntHandler()
│      ├─ lwIPTimer(10)           // Process lwIP timers
│      └─ (May trigger response transmission)
│
├─► RESPONSE TRANSMISSION
│   │
│   ├─ lwIP sends queued packet
│   │  ├─ udp_send()
│   │  ├─ ip_output()
│   │  └─ etharp_output()
│   │
│   ├─ MAC Driver queues in TX buffer
│   │
│   └─ Ethernet frame transmitted on wire
│      ├─ Physical layer encodes
│      └─ Sent back to scanner
│
└─► SCANNER RECEIVES RESPONSE
    └─ Vendor ID = 0x0186 ✓
```

---

## 📊 TIMING DIAGRAM

```
Time    Main Loop        SysTick         Ethernet         Status
────────────────────────────────────────────────────────────────────
 0ms    while(g_bLED)                                    Waiting
        EI_APP_Process()

 1ms    EI_APP_Process()                                Waiting
        (busy loop)

 2ms    EI_APP_Process()                                Waiting

 5ms    EI_APP_Process()    [Interrupt!]                Waiting
                  ↓
                  lwIPTimer(10)
                  g_bLED = true
                  
 5ms+   (resume)                                         
        exit while loop

 6ms    GPIOPinWrite()      [Toggle LED]                 LED: ON
        while(g_bLED)

 7ms    EI_APP_Process()                                 LED: ON
        
 10ms   EI_APP_Process()    [Interrupt!]                 LED: ON
                  ↓
                  lwIPTimer(10)
                  g_bLED = true

10ms+   exit while loop
        GPIOPinWrite()      [Toggle LED]                 LED: OFF

...

FRAME
50ms    [Ethernet packet arrives during EI_APP_Process()]
        
        [INT_EMAC0 fires - PREEMPTS main loop]
        
        MAC → lwIP → UDP → EI_API → Response queued
        
        [Resume main loop from where it paused]
        
        (Response sent on next TX opportunity)
```

---

## 🎓 SUMMARY: Data Path from Ethernet to EtherNet/IP

### 5 Key Points:

1. **Initialization (main function)**
   - lwIPInit() sets up Ethernet MAC, UDP/IP
   - EI_APP_Init() sets up EtherNet/IP abstraction
   - Both run sequentially during startup

2. **Main Loop (continuous)**
   - EI_APP_Process() called periodically (~100 times/sec)
   - Processes any work queued by interrupts
   - Updates LED status

3. **Ethernet Reception (interrupt-driven)**
   - Frame arrives → INT_EMAC0 fires
   - MAC driver → lwIP → UDP → EI_API
   - Response queued in buffer

4. **Response Transmission (next opportunity)**
   - lwIP sends queued packet
   - MAC driver transmits on wire
   - Takes ~10-100 microseconds

5. **Timing**
   - SysTick interrupt: every 10ms (100 Hz)
   - Ethernet interrupt: whenever packet arrives
   - Main loop: blocked on g_bLED flag, woken every 10ms

---

**Key Files:**
- `enet_lwip.c` - Main application (existing TivaWare example)
- `ei_app_minimal.c` - EtherNet/IP wrapper (NEW)
- `ei_tm4c_netif.c` - Network interface adapter (NEW)
- `lwiplib.c` (TivaWare) - lwIP port

**Interface Points:**
- EI_APP_Init() - After lwIPInit()
- EI_APP_Process() - In main loop idle
- UDP port 2222 - Automatic routing by lwIP

**Data Owner:** Each layer owns its data
- TivaWare owns: MAC hardware state
- lwIP owns: Network buffers, TCP/UDP state
- EI_API owns: EtherNet/IP/CIP messages
