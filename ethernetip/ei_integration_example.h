//*****************************************************************************
//
// ei_integration_example.h - Example Integration Guide for EtherNet/IP on TM4C
//
// This file shows how to integrate the EtherNet/IP abstraction layer into
// the existing enet_lwip.c application. Use this as a reference when
// modifying enet_lwip.c or creating a new variant.
//
// INTEGRATION STEPS:
//
// 1. Add #include directives to enet_lwip.c:
//    
//    #include "ethernetip/ei_tm4c_config.h"
//    #include "ethernetip/ei_tm4c_netif.h"
//    #include "ethernetip/ei_tm4c_board.h"
//    #include "ethernetip/ei_app_minimal.h"
//
// 2. In main(), after lwIPInit() completes, add:
//    
//    // Initialize EtherNet/IP stack
//    if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
//        UARTprintf("Failed to initialize EtherNet/IP\n");
//        while(1);
//    }
//
// 3. In the main loop, after LED blinking code, add:
//    
//    // Process EtherNet/IP stack
//    EI_APP_Process();
//
// Copyright (c) 2025 Texas Instruments Incorporated.
//
//*****************************************************************************

#ifndef EI_INTEGRATION_EXAMPLE_H
#define EI_INTEGRATION_EXAMPLE_H

//*****************************************************************************
//
// EXAMPLE: Modified main() function with EtherNet/IP integration
//
// This shows how to modify enet_lwip.c to include EtherNet/IP stack
//
//*****************************************************************************

/*
int main(void)
{
    uint32_t ui32User0, ui32User1;
    uint8_t pui8MACArray[8];

    // System initialization (unchanged from original enet_lwip.c)
    SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);
    PinoutSet(true, false);
    UARTStdioConfig(0, 115200, g_ui32SysClock);
    UARTprintf("\033[2J\033[H");
    UARTprintf("Ethernet lwIP + EtherNet/IP example\n\n");

    // GPIO initialization (unchanged)
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
    MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, ~GPIO_PIN_1);

    // SysTick initialization (unchanged)
    MAP_SysTickPeriodSet(g_ui32SysClock / SYSTICKHZ);
    MAP_SysTickEnable();
    MAP_SysTickIntEnable();

    // MAC address retrieval (unchanged)
    MAP_FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        UARTprintf("No MAC programmed!\n");
        while(1) { }
    }
    UARTprintf("Waiting for IP.\n");

    pui8MACArray[0] = ((ui32User0 >>  0) & 0xff);
    pui8MACArray[1] = ((ui32User0 >>  8) & 0xff);
    pui8MACArray[2] = ((ui32User0 >> 16) & 0xff);
    pui8MACArray[3] = ((ui32User1 >>  0) & 0xff);
    pui8MACArray[4] = ((ui32User1 >>  8) & 0xff);
    pui8MACArray[5] = ((ui32User1 >> 16) & 0xff);

    // lwIP initialization (unchanged)
    lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);

    // Locator and HTTP server (unchanged)
    LocatorInit();
    LocatorMACAddrSet(pui8MACArray);
    LocatorAppTitleSet("EK-TM4C1294XL enet_eip");
    httpd_init();

    // Interrupt priorities (unchanged)
    MAP_IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
    MAP_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

    // ===== NEW: Initialize EtherNet/IP Stack =====
    UARTprintf("Initializing EtherNet/IP stack...\n");
    if (EI_APP_Init(NULL) != EI_APP_STATUS_OK) {
        UARTprintf("Failed to initialize EtherNet/IP\n");
        while(1);
    }
    UARTprintf("EtherNet/IP stack ready!\n");

    // Main loop with EtherNet/IP processing
    while(1)
    {
        // Wait for SysTick interrupt to set the LED blink flag
        while(g_bLED == false)
        {
            // ===== NEW: Process EtherNet/IP in idle loop =====
            EI_APP_Process();
        }

        g_bLED = false;

        // Toggle LED (original behavior)
        MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,
                         (MAP_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_1) ^
                          GPIO_PIN_1));
    }
}
*/

//*****************************************************************************
//
// EXAMPLE: Makefile additions for EtherNet/IP integration
//
// Add these lines to the Makefile to include the new abstraction layer:
//
//*****************************************************************************

/*
# Add include paths for EtherNet/IP abstraction layer
IFLAGS += -I$(ROOT)/ethernetip

# Add source files for EtherNet/IP abstraction layer
OBJS += $(BUILD)/ethernetip/ei_tm4c_netif.o
OBJS += $(BUILD)/ethernetip/ei_tm4c_board.o
OBJS += $(BUILD)/ethernetip/ei_app_minimal.o

# Build rules for EtherNet/IP files
$(BUILD)/ethernetip/%.o: $(ROOT)/ethernetip/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(IFLAGS) $< -o $@
*/

//*****************************************************************************
//
// EXAMPLE: CCS Project additions
//
// For CCS IDE, add these files to your project:
//
//   - ethernetip/ei_tm4c_config.h
//   - ethernetip/ei_tm4c_netif.h
//   - ethernetip/ei_tm4c_netif.c
//   - ethernetip/ei_tm4c_board.h
//   - ethernetip/ei_tm4c_board.c
//   - ethernetip/ei_app_minimal.h
//   - ethernetip/ei_app_minimal.c
//
// And update the include paths:
//   Project -> Properties -> Build -> Compiler -> Include Options
//   Add: ${PROJECT_ROOT}/ethernetip
//
//*****************************************************************************

//*****************************************************************************
//
// DEBUGGING AND TROUBLESHOOTING
//
//*****************************************************************************

/*
If you encounter compilation errors:

1. "undefined reference to 'netif_list'"
   - This is expected if EI_API library is not yet integrated
   - The netif_list is part of lwIP core
   - Make sure lwIP is properly linked

2. "undefined reference to 'g_ui32SysClock'"
   - This variable is defined in enet_lwip.c
   - Make sure both files are compiled together and linked

3. Missing functions from EI_API
   - The ei_app_minimal.c is a PLACEHOLDER
   - When you integrate the full EI_API library, replace ei_app_minimal.c
   - with a full implementation that calls EI_API functions

4. Memory errors (stack overflow, heap exhaustion)
   - Check linker script (enet_lwip.ld)
   - May need to disable web server (httpd) to free RAM
   - Reduce packet buffer count in ei_tm4c_config.h

5. Network interface not found
   - Ensure lwIPInit() completes BEFORE EI_APP_Init()
   - Check that network interface is actually created by lwIP
   - Verify lwIP include paths are correct
*/

//*****************************************************************************
//
// NEXT STEPS AFTER ABSTRACTION LAYER WORKS
//
//*****************************************************************************

/*
Once the abstraction layer compiles and initializes:

1. Integrate EI_API Library:
   - Obtain EI_API library from TI or build from source
   - Link against libei_api.a (or equivalent)
   - Include EI_API headers in ei_app_minimal.c
   - Replace placeholder EI_APP_Process() with actual EI_API calls

2. Add Device Profiles:
   - Copy device_profiles/ from AM64x project
   - Update device_profiles/common/device_profile_cfg.c
   - Implement I/O read/write callbacks for your application

3. Configure Stack Parameters:
   - Edit config/cfg_stack.h for EtherNet/IP stack behavior
   - Adjust assembly sizes, class definitions, etc.
   - Enable/disable features as needed

4. Testing:
   - Use EtherNet/IP scanner (e.g., Linuxcnc, RSLogix)
   - Verify device discovery
   - Test CIP communications
   - Monitor performance

5. Performance Tuning:
   - Profile stack performance
   - Adjust packet buffer sizes
   - Optimize interrupt priorities
   - Consider enabling hardware timestamping if needed
*/

#endif  // EI_INTEGRATION_EXAMPLE_H
