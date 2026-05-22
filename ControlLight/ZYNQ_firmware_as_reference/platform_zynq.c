/*
 * Copyright (C) 2010 - 2019 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */
/*
* platform_zynq.c
*
* Zynq platform specific functions.
*
* 02/29/2012: UART initialization is removed. Timer initializations are
* removed. All unnecessary include files and hash defines are removed.
* 03/01/2013: Timer initialization is added back. Support for SI #692601 is
* added in the timer callback. The SI #692601 refers to the following issue.
*
* The EmacPs has a HW bug on the Rx path for heavy Rx traffic.
* Under heavy Rx traffic because of the HW bug there are times when the Rx path
* becomes unresponsive. The workaround for it is to check for the Rx path for
* traffic (by reading the stats registers regularly). If the stats register
* does not increment for sometime (proving no Rx traffic), the function resets
* the Rx data path.
*
* </pre>
 */

#ifdef __arm__

#include "xparameters.h"
#include "xparameters_ps.h"	/* defines XPAR values */
#include "xil_cache.h"
#include "xscugic.h"
#include "lwip/tcp.h"
#include "xil_printf.h"
#include "platform.h"
#include "platform_config.h"
#include "netif/xadapter.h"
#ifdef PLATFORM_ZYNQ
#include "xscutimer.h"

#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_DEVICE_ID		XPAR_SCUTIMER_DEVICE_ID
#define INTC_BASE_ADDR		XPAR_SCUGIC_0_CPU_BASEADDR
#define INTC_DIST_BASE_ADDR	XPAR_SCUGIC_0_DIST_BASEADDR
#define TIMER_IRPT_INTR		XPAR_SCUTIMER_INTR

#define RESET_RX_CNTR_LIMIT	400

void tcp_fasttmr(void);
void tcp_slowtmr(void);

static XScuTimer TimerInstance;

#ifndef USE_SOFTETH_ON_ZYNQ
static int ResetRxCntr = 0;
#endif

extern struct netif *echo_netif;

volatile int TcpFastTmrFlag = 0;
volatile int TcpSlowTmrFlag = 0;

#if LWIP_DHCP==1
volatile int dhcp_timoutcntr = 24;
void dhcp_fine_tmr();
void dhcp_coarse_tmr();
#endif

void
timer_callback(XScuTimer * TimerInstance)
{
	static int DetectEthLinkStatus = 0;
	/* we need to call tcp_fasttmr & tcp_slowtmr at intervals specified
	 * by lwIP. It is not important that the timing is absoluetly accurate.
	 */
	static int odd = 1;
#if LWIP_DHCP==1
    static int dhcp_timer = 0;
#endif
	DetectEthLinkStatus++;
	 TcpFastTmrFlag = 1;

	odd = !odd;
#ifndef USE_SOFTETH_ON_ZYNQ
	ResetRxCntr++;
#endif
	if (odd) {
#if LWIP_DHCP==1
		dhcp_timer++;
		dhcp_timoutcntr--;
#endif
		TcpSlowTmrFlag = 1;
#if LWIP_DHCP==1
		dhcp_fine_tmr();
		if (dhcp_timer >= 120) {
			dhcp_coarse_tmr();
			dhcp_timer = 0;
		}
#endif
	}

	/* For providing an SW alternative for the SI #692601. Under heavy
	 * Rx traffic if at some point the Rx path becomes unresponsive, the
	 * following API call will ensures a SW reset of the Rx path. The
	 * API xemacpsif_resetrx_on_no_rxdata is called every 100 milliseconds.
	 * This ensures that if the above HW bug is hit, in the worst case,
	 * the Rx path cannot become unresponsive for more than 100
	 * milliseconds.
	 */
#ifndef USE_SOFTETH_ON_ZYNQ
	if (ResetRxCntr >= RESET_RX_CNTR_LIMIT) {
		xemacpsif_resetrx_on_no_rxdata(echo_netif);
		ResetRxCntr = 0;
	}
#endif
	/* For detecting Ethernet phy link status periodically */
	if (DetectEthLinkStatus == ETH_LINK_DETECT_INTERVAL) {
		eth_link_detect(echo_netif);
		DetectEthLinkStatus = 0;
	}

	XScuTimer_ClearInterruptStatus(TimerInstance);
}

void platform_setup_timer(void)
{
	int Status = XST_SUCCESS;
	XScuTimer_Config *ConfigPtr;
	int TimerLoadValue = 0;

	ConfigPtr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
	Status = XScuTimer_CfgInitialize(&TimerInstance, ConfigPtr,
			ConfigPtr->BaseAddr);
	if (Status != XST_SUCCESS) {

		xil_printf("In %s: Scutimer Cfg initialization failed...\r\n",
		__func__);
		return;
	}

	Status = XScuTimer_SelfTest(&TimerInstance);
	if (Status != XST_SUCCESS) {
		xil_printf("In %s: Scutimer Self test failed...\r\n",
		__func__);
		return;

	}

	XScuTimer_EnableAutoReload(&TimerInstance);
	/*
	 * Set for 250 milli seconds timeout.
	 */
	TimerLoadValue = XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ / (8*128);/// 8;  128 might be too much //ToDo: adjust to make as responsive as needed, but not too fast in order to not waste time on ethernet pulling checks

	XScuTimer_LoadTimer(&TimerInstance, TimerLoadValue);
	return;
}

XScuGic InterruptController;
//This should be defined in xparameters.h but somehow isn't
/* Definitions for Fabric interrupts connected to ps7_scugic_0 */
#define XPAR_FABRIC_AXI_GPIO_4_GPIO_IO_O_INTR 61U
#define INTC_DEVICE_INT_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
extern void ExtIrq_Handler(void *InstancePtr);
static XScuGic_Config *GicConfig;

extern int firefly_control_init(XScuGic *InterruptController);

int platform_setup_interrupts(void)
{
	//xil_printf("\r\n");

	//xil_printf("\r\nplatform_setup_interrupts\r\n");
	// Initialize GIC
	GicConfig = XScuGic_LookupConfig(INTC_DEVICE_INT_ID);
	if (NULL == GicConfig) {
		xil_printf("XScuGic_LookupConfig(%d) failed\r\n",
				INTC_DEVICE_INT_ID);
		return XST_FAILURE;
	}

	//xil_printf("platform_setup_interrupts: XScuGic_CfgInitialize\r\n");

	int Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,
					   GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("XScuGic_CfgInitialize failed\r\n");
		return XST_FAILURE;
	}

	//xil_printf("platform_setup_interrupts: Xil_ExceptionInit\r\n");
	Xil_ExceptionInit();
	//xil_printf("platform_setup_interrupts: XScuGic_DeviceInitialize\r\n");
	XScuGic_DeviceInitialize(INTC_DEVICE_ID);

	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	//xil_printf("platform_setup_interrupts: Xil_ExceptionRegisterHandler\r\n");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler)XScuGic_InterruptHandler,//(Xil_ExceptionHandler)XScuGic_DeviceInterruptHandler,
			&InterruptController);//(void *)INTC_DEVICE_ID);

	//xil_printf("firefly_control_interrupt_init: XScuGic_Connect 3a\r\n");

	  Status = XScuGic_Connect(&InterruptController,
			  XPAR_FABRIC_AXI_GPIO_4_GPIO_IO_O_INTR, (Xil_ExceptionHandler)ExtIrq_Handler, (void *)NULL);
	  if (Status != XST_SUCCESS) {
		  xil_printf("firefly_control_interrupt_init: XScuGic_Connect failed\r\n");
		  return XST_FAILURE;
	  }

	firefly_control_init(&InterruptController);


	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	//xil_printf("platform_setup_interrupts: XScuGic_RegisterHandler\r\n");
	XScuGic_RegisterHandler(INTC_BASE_ADDR, TIMER_IRPT_INTR,
					(Xil_ExceptionHandler)timer_callback,
					(void *)&TimerInstance);

	/*
	 * Enable the interrupt for scu timer.
	 */
	//xil_printf("platform_setup_interrupts: XScuGic_EnableIntr\r\n");
	XScuGic_EnableIntr(INTC_DIST_BASE_ADDR, TIMER_IRPT_INTR);

	return XST_SUCCESS;
}

void platform_enable_interrupts()
{
	/*
	 * Enable non-critical exceptions.
	 */

	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	XScuTimer_EnableInterrupt(&TimerInstance);
	XScuTimer_Start(&TimerInstance);
	return;
}

void init_platform()
{
	platform_setup_timer();
	platform_setup_interrupts();

	return;
}

void cleanup_platform()
{
	Xil_ICacheDisable();
	Xil_DCacheDisable();
	return;
}
#endif
#endif

