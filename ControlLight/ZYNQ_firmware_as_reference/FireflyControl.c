
#include "xparameters.h"
#include "stdio.h"
#include "xil_testmem.h"
#include "xil_exception.h"
#include "xil_types.h"
#include "xil_cache.h"
#include "xgpio.h"

#include "xuartps.h"	// if PS uart is used
#include "xscutimer.h"  // if PS Timer is used
#include "xdmaps.h"		// if PS DMA is used
#include "xscugic.h" 	// if PS GIC is used
#include "xil_printf.h"
#include "stdlib.h"
#include "platform.h"

#include "xtime_l.h"

#include "xgpiops.h"

#include "netif/xadapter.h"

#include "echo.h"
#include "main.h"
#include <math.h>

#include "OpticsFoundryCPUCommandSequencer/CPUCommandSequencer.h"

extern void RunEventLoop(); //defined in main.c

#include "FireflyControl.h"

//I2C port
#include "xiicps.h"
#include "xil_printf.h"

//Mirco/PicoZed 7020: 1GByte of DDR  XPAR_PS7_DDR_0_S_AXI_HIGHADDR = 0x3FFFFFFF
//PYNQ Z2: 512MByte of DDR           XPAR_PS7_DDR_0_S_AXI_HIGHADDR = 0x1FFFFFFF

#define AVAILABLE_MEMORY (XPAR_PS7_DDR_0_S_AXI_HIGHADDR - DDR_MEMORY + 1)
#define DDR_INPUT_BUFFER_LENGTH_IN_BYTES 0x01000000
#define DDR_MODIFY_BUFFER_LENGTH_IN_BYTES 0x01000000
#define DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES (AVAILABLE_MEMORY - DDR_INPUT_BUFFER_LENGTH_IN_BYTES - DDR_MODIFY_BUFFER_LENGTH_IN_BYTES)
#define DDR_INPUT_BUFFER_START (XPAR_PS7_DDR_0_S_AXI_HIGHADDR - DDR_INPUT_BUFFER_LENGTH_IN_BYTES - DDR_MODIFY_BUFFER_LENGTH_IN_BYTES + 1)
#define DDR_MODIFY_BUFFER_START (XPAR_PS7_DDR_0_S_AXI_HIGHADDR - DDR_MODIFY_BUFFER_LENGTH_IN_BYTES + 1)
#define TIMER_DEVICE_ID	XPAR_SCUTIMER_DEVICE_ID
#define TIMER_LOAD_VALUE 0xFFFFFFFF
#define DMA0_ID XPAR_XDMAPS_1_DEVICE_ID
#define INTC_DEVICE_INT_ID XPAR_SCUGIC_SINGLE_DEVICE_ID


u32 BRAM_OUTPUT_BUFFER_LENGTH = (XPAR_AXI_BRAM_CTRL_0_S_AXI_HIGHADDR - XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR)/4 + 1; //131072

u32* source = (u32 *)(DDR_MEMORY);
u32* destination = (u32 *)BRAM_MEMORY;
u32* BRAM_input_buffer = (u32 *)XPAR_AXI_BRAM_CTRL_INPUT_DATA_S_AXI_BASEADDR;
u32* DDR_input_buffer = (u32 *) DDR_INPUT_BUFFER_START;
#define BRAM_INPUT_BUFFER_SIZE_IN_BYTES  32768  //2024 08 13 compensating for non-32-bit addressing error, change in Vivado //XPAR_AXI_BRAM_CTRL_INPUT_DATA_S_AXI_HIGHADDR - XPAR_AXI_BRAM_CTRL_INPUT_DATA_S_AXI_BASEADDR + 1

volatile static int DMA0Done = 1;	/* Dma transfer is done */
volatile static int DMA0Error = 0;	/* Dma Bus Error occurs */
volatile static int DMA1Done = 1;	/* Dma transfer is done */
volatile static int DMA1Error = 0;	/* Dma Bus Error occurs */
bool Sequence_running = FALSE;
bool Sequence_ended_in_error = FALSE;
u32 Sequence_length_in_64bit_commands = 0;
u32 Sequence_start_pos_in_64bit_commands = 0;
u32 Sequence_length_in_64bit_commands_transferred_to_BRAM = 0;
u32 SequenceNumber = 0;
bool DebugModeOn = FALSE;
bool IgnoreTCPIP = FALSE;

u32 InputBufferPosition = 0;

XUartPs Uart_PS;		/* Instance of the UART Device */
XScuTimer Timer;		/* Cortex A9 SCU Private Timer Instance */
XDmaPs Dma;				/* PS DMA */
//XScuGic Gic;			/* PS GIC */
//XGpio axi_gpio_0_device, axi_gpio_2_device, axi_gpio_3_device, axi_gpio_5_device;
XGpio axi_gpio_config_status, axi_gpio_core_status_0, axi_gpio_core_status_1, axi_gpio_core_status_2,  axi_gpio_core_status_3,  axi_gpio_core_status_4,  axi_gpio_core_status_5,  axi_gpio_core_status_6,  axi_gpio_stop_watch;



/*****************************************************************************/
/**
* The purpose of this function is to illustrate how to use the XGpioPs component.
* It initializes the PS GPIO and sets the direction of the Output Pin.
*
* @param	PsGpioInstancePtr is a pointer to the XGpioPs driver Instance
* @param	DeviceId is the XPAR_<GPIOPS_instance>_DEVICE_ID value from	xparameters.h
* @return	XST_SUCCESS if the Test is successful, otherwise XST_FAILURE
*
*****************************************************************************/

static XGpioPs PsGpio;
extern XGpioPs_Config XGpioPs_ConfigTable[XPAR_XGPIOPS_NUM_INSTANCES];
#define GPIO_DEVICE_ID  	XPAR_XGPIOPS_0_DEVICE_ID


#define RESET_PIN		0
#define OUTPUT_PIN		3
#define INPUT_PIN		9


int PsGpioSetup(XGpioPs* PsGpioInstancePtr, u16 DeviceId)
{
	int Status;
	XGpioPs_Config*GpioConfigPtr;
	GpioConfigPtr = XGpioPs_LookupConfig(DeviceId);
	if(GpioConfigPtr == NULL){
		if (DebugModeOn) xil_printf(" PS GPIO INIT FAILED 1 \n\r");
		return XST_FAILURE;
		}
	if (DebugModeOn) xil_printf("initialize ps gpio 0\r\n");
	Status = XGpioPs_CfgInitialize(PsGpioInstancePtr,
				GpioConfigPtr,
				GpioConfigPtr->BaseAddr);
	if(XST_SUCCESS != Status){
		if (DebugModeOn) xil_printf(" PS GPIO INIT FAILED 2 \n\r");
		return XST_FAILURE;
		}

	if (DebugModeOn) xil_printf("set direction output pin\r\n");
	for (int i= 0;i<8;i++) {
		XGpioPs_SetDirectionPin(PsGpioInstancePtr, i,/*0: in 1:out*/ 1);
		XGpioPs_SetOutputEnablePin(PsGpioInstancePtr, i,/* 1 for output enable, 0: disable */ 1);
	}
	if (DebugModeOn) xil_printf("set direction input pin \r\n");

	for (int i= 8;i<64;i++) {
		XGpioPs_SetDirectionPin(PsGpioInstancePtr, i, 0);
		XGpioPs_SetOutputEnablePin(PsGpioInstancePtr, i, 0);
	}
	return XST_SUCCESS;
}


int test_ps_gpio(void) {
	int Status;

	if (DebugModeOn) xil_printf("test ps gpio 0\r\n");

	Status = PsGpioSetup(&PsGpio, GPIO_DEVICE_ID);
	if(XST_SUCCESS != Status){
		if (DebugModeOn) xil_printf(" PS GPIO INIT FAILED \n\r");
		return XST_FAILURE;
	}

	if (DebugModeOn) xil_printf("PS out = 0 \r\n");
	XGpioPs_WritePin(&PsGpio,OUTPUT_PIN,0);

	for (int i = 0;i<64;i++) {
		XGpioPs_WritePin(&PsGpio,i,0);
		}

//	u32 data;
//	data = XGpio_DiscreteRead(&axi_gpio_5_device, 1);
//	if (DebugModeOn) xil_printf("GPIO_5 port 1: %x\r\n", data);
//	data = XGpio_DiscreteRead(&axi_gpio_5_device, 2);
//	if (DebugModeOn) xil_printf("GPIO_5 port 2: %x\r\n", data);



	for (int i = 0;i<64;i++) {
		if (1== XGpioPs_ReadPin(&PsGpio,i)) {if (DebugModeOn) xil_printf("1");} else {if (DebugModeOn) xil_printf("0");}
	}
	if (DebugModeOn) xil_printf("\r\n");

	if (DebugModeOn) xil_printf("PS out = 1 \r\n");

	XGpioPs_WritePin(&PsGpio,OUTPUT_PIN,1);

	for (int i = 0;i<64;i++) {
		XGpioPs_WritePin(&PsGpio,i,1);
		}

	//data = XGpio_DiscreteRead(&axi_gpio_5_device, 1);
	//if (DebugModeOn) xil_printf("GPIO_5 port 1: %x\r\n", data);
	//data = XGpio_DiscreteRead(&axi_gpio_5_device, 2);
	//if (DebugModeOn) xil_printf("GPIO_5 port 2: %x\r\n", data);

	for (int i = 0;i<64;i++) {
		if (1== XGpioPs_ReadPin(&PsGpio,i)) {if (DebugModeOn) xil_printf("1");} else {if (DebugModeOn) xil_printf("0");}
	}
	if (DebugModeOn) xil_printf("\r\n");


	if (1== XGpioPs_ReadPin(&PsGpio,INPUT_PIN))
		{if (DebugModeOn) xil_printf("PS in Turned 1 \r\n");}
	else {if (DebugModeOn) xil_printf("PS in Turned 0 \r\n");}

	for (int i = 0;i<64;i++) {
			XGpioPs_WritePin(&PsGpio,i,0);
			}
	return XST_SUCCESS;
}

u8 address_shift = 16;

int configure_axi_gpio(void) {
	XGpio_Config *cfg_ptr;
	//u32 data;
	int Status;
	//if (DebugModeOn) xil_printf("Configure axi gpio\r\n");

	// Initialize axi_gpio_0_device
/*	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_0_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_0_device, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" AXI GPIO 0 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi gpio 2\r\n");
	// Initialize axi_gpio_2_device
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_2_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_2_device, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" AXI GPIO 2 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi gpio 3\r\n");
	// Initialize axi_gpio_3_device
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_3_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_3_device, cfg_ptr, cfg_ptr->BaseAddress);
	//if (DebugModeOn) xil_printf("GPIO_3 base address: %x\r\n", cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" AXI GPIO 3 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi gpio 5\r\n");
	// Initialize axi_gpio_5_device
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_5_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_5_device, cfg_ptr, cfg_ptr->BaseAddress);
	//if (DebugModeOn) xil_printf("GPIO_5 base address: %x\r\n", cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" AXI GPIO 5 INIT FAILED \n\r");
		return XST_FAILURE;
	}
	*/
	//if (DebugModeOn) xil_printf("test axi_gpio_config_status\r\n");
	// Initialize axi_gpio_config_status
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_CONFIG_STATUS_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_config_status, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_config_status INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi_gpio_core_status_0\r\n");
	// Initialize axi_gpio_core_status_0
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_CORE_STATUS_0_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_core_status_0, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_core_status_0 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi_gpio_core_status_1\r\n");
	// Initialize axi_gpio_core_status_1
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_CORE_STATUS_1_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_core_status_1, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_core_status_1 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi_gpio_core_status_2\r\n");
	// Initialize axi_gpio_core_status_2
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_CORE_STATUS_2_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_core_status_2, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_core_status_2 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi_gpio_core_status_3\r\n");
	// Initialize axi_gpio_core_status_3
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_CORE_STATUS_3_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_core_status_3, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_core_status_3 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi_gpio_core_status_4\r\n");
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_CORE_STATUS_4_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_core_status_4, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_core_status_4 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi_gpio_core_status_5\r\n");
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_CORE_STATUS_5_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_core_status_5, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_core_status_5 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi_gpio_core_status_6\r\n");
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_CORE_STATUS_6_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_core_status_6, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_core_status_6 INIT FAILED \n\r");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("test axi_gpio_stop_watch\r\n");
	cfg_ptr = XGpio_LookupConfig(XPAR_AXI_GPIO_STOP_WATCH_DEVICE_ID);
	Status = XGpio_CfgInitialize(&axi_gpio_stop_watch, cfg_ptr, cfg_ptr->BaseAddress);
	if (XST_SUCCESS != Status){
		print(" axi_gpio_stop_watch INIT FAILED \n\r");
		return XST_FAILURE;
	}

//axi_gpio_core_status_1, axi_gpio_core_status_2,  axi_gpio_core_status_3;

	//XGpio_SetDataDirection(&axi_gpio_0_device, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0);
	//XGpio_SetDataDirection(&axi_gpio_0_device, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0);
	//XGpio_SetDataDirection(&axi_gpio_2_device, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	//XGpio_SetDataDirection(&axi_gpio_2_device, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	//XGpio_SetDataDirection(&axi_gpio_3_device, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0);
	//XGpio_SetDataDirection(&axi_gpio_3_device, /*channel 1 or 2*/2, /*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	//XGpio_SetDataDirection(&axi_gpio_5_device, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	//XGpio_SetDataDirection(&axi_gpio_5_device, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);

	XGpio_SetDataDirection(&axi_gpio_config_status, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0);
	XGpio_SetDataDirection(&axi_gpio_config_status, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_0, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_0, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_1, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_1, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_2, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_2, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_3, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_3, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_4, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_4, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_5, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_5, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_6, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_core_status_6, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_stop_watch, /*channel 1 or 2*/ 1,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);
	XGpio_SetDataDirection(&axi_gpio_stop_watch, /*channel 1 or 2*/ 2,/*direction mask 0:out 1: in*/ 0xFFFFFFFF);

	//XGpio_DiscreteWrite(&axi_gpio_3_device, 1, 0xDEADBEEF);
	//data = 0;
	//data = XGpio_DiscreteRead(&axi_gpio_3_device, 2);
	//if (DebugModeOn) xil_printf("AXI GPIO test data transfer result: %x (should be BEEF_bitstream_version)\r\n", data);

	u32 target = (BRAM_OUTPUT_BUFFER_LENGTH+1)>>1;
	u32 start = 1;
	address_shift = 0;
	while ((start != target) && (address_shift<32)) {
		start = start << 1;
		address_shift++;
	}
	if (address_shift>32) {
		if (DebugModeOn) xil_printf("\r\n error shift = %d",address_shift);
		address_shift = 16;
	}
	//if (DebugModeOn) xil_printf("address shift = %d\r\n",address_shift);
	return XST_SUCCESS;
}

/*u32 Read_BRAM_port_B_high(u32 address) {
	XGpio_DiscreteWrite(&axi_gpio_1_device, 1, address);
	return XGpio_DiscreteRead(&axi_gpio_2_device, 2);
}

u32 Read_BRAM_port_B_low(u32 address) {
	XGpio_DiscreteWrite(&axi_gpio_1_device, 1, address);
	return XGpio_DiscreteRead(&axi_gpio_2_device, 1);
}*/


// PS DMA related definitions
XDmaPs_Config *DmaCfg;
XDmaPs_Cmd Dma0Cmd = {
	.ChanCtrl = {
		.SrcBurstSize = 4,
		.SrcBurstLen = 4,
		.SrcInc = 1,		// increment source address
		.DstBurstSize = 4,
		.DstBurstLen = 4,
		.DstInc = 1,		// increment destination address
	},
};

XDmaPs_Cmd Dma1Cmd = {
	.ChanCtrl = {
		.SrcBurstSize = 4,
		.SrcBurstLen = 4,
		.SrcInc = 1,		// increment source address
		.DstBurstSize = 4,
		.DstBurstLen = 4,
		.DstInc = 1,		// increment destination address
	},
};


//Interrupt demo
//http://www.globaltek.kr/zynq-interrupt-example-tutorial/?ckattempt=1

//XScuGic InterruptController;
//static XScuGic_Config *GicConfig;

u32 fabric_config = 0;
void WriteConfigBit(unsigned short BitNr, unsigned short value) {
	u32 mask = 1 << BitNr;
	u32 newbit = (value>0) ? mask : 0;
	fabric_config = (fabric_config & ~mask) | newbit;
	XGpio_DiscreteWrite(&axi_gpio_config_status, 1, fabric_config);
}

void WriteConfigBits(unsigned short StartBitNr,
                     unsigned short NrOfBits,
                     u32 value)
{
    if (NrOfBits == 0 || NrOfBits > 32)
        return;
    u32 mask;
    if (NrOfBits == 32)
        mask = 0xFFFFFFFFu;
    else
        mask = ((1u << NrOfBits) - 1u) << StartBitNr;
    u32 newbits = (value << StartBitNr) & mask;
    fabric_config = (fabric_config & ~mask) | newbits;
    XGpio_DiscreteWrite(&axi_gpio_config_status, 1, fabric_config);
}

u32 last_status = 0;
bool ReadStatusBit(unsigned short BitNr, bool read_from_last_status) {
	//if (DebugModeOn) xil_printf(" ReadStatusBit(%d,%d)\n\r", BitNr, read_from_last_status);
	if (!read_from_last_status) last_status = XGpio_DiscreteRead(&axi_gpio_config_status, 2);
	return ( (last_status & (1 << BitNr)) > 0 ) ? TRUE : FALSE;
}

u32 ReadStatusBits(unsigned short StartBitNr,
                   unsigned short NrOfBits,
                   bool read_from_last_status)
{
    //if (DebugModeOn)
    //    xil_printf(" ReadStatusBits(%d,%d,%d)\n\r",
    //               StartBitNr, NrOfBits, read_from_last_status);
    if (NrOfBits == 0 || NrOfBits > 32)
        return 0;
    if (!read_from_last_status)
        last_status = XGpio_DiscreteRead(&axi_gpio_config_status, 2);
    u32 mask;
    if (NrOfBits == 32)
        mask = 0xFFFFFFFFu;
    else
        mask = (1u << NrOfBits) - 1u;
    return (last_status >> StartBitNr) & mask;
}

bool GetExternalClockLocked(){
	return ReadStatusBit(0, FALSE);
}

bool GetInternalClockLocked(){
	return ReadStatusBit(1, FALSE);
}

bool GetRunningFromCore(bool read_from_last_status) {
	return ReadStatusBit(2, read_from_last_status);
}

bool GetRunning(bool read_from_last_status) {
	return GetRunningFromCore(read_from_last_status) && Sequence_running;
}

bool GetErrorDetected(bool read_from_last_status) {
	return ReadStatusBit(3, read_from_last_status);
}

bool GetWaitingForPeriodicTrigger() {
	return ReadStatusBit(4, FALSE);
}

bool GetWarningMissedPeriodicTrigger() {
	return ReadStatusBit(5, FALSE);
}

bool GetExternalClockLockWatcher(){
	return ReadStatusBit(6, FALSE);
}

bool GetExternalClockSelectLockWatcher(){
	return ReadStatusBit(7, FALSE);
}

bool GetInternalClockLockWatcher(){
	return ReadStatusBit(8, FALSE);
}

bool GetInteruptRequested() {
	return ReadStatusBit(9, FALSE);
}

bool GetExternalClockSelectLock() {
	return ReadStatusBit(10, FALSE);
}

bool GetSecondaryInteruptRequested(bool read_from_last_status) {
	return ReadStatusBit(11, read_from_last_status);
}

bool GetExtCondition0() {
	return ReadStatusBit(12, FALSE);
}

bool GetExtCondition1() {
	return ReadStatusBit(13, FALSE);
}

bool GetInterlock(bool read_from_last_status) {
	return ReadStatusBit(14, read_from_last_status);
}

bool GetInputMemInterlockRequested(bool read_from_last_status) {
	return ReadStatusBit(15, read_from_last_status);
}

u32 GetPLtoPSCommand(bool read_from_last_status) {
	if (!read_from_last_status) last_status = XGpio_DiscreteRead(&axi_gpio_config_status, 2);
	return ( (last_status >> 16) & 0xFFFF );
}

u32 ReadLatchedSequenceAddress() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_0, 1) && 0xFFFF;
}

u32 ReadLatchedExtendedAddress() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_0, 1);
}

u32 ReadInputBufferAddress() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_0, 2);
}

u64 ReadLatchedWaitCycles() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_1, 1) | ((u64)(XGpio_DiscreteRead(&axi_gpio_core_status_1, 2)) << 32);
}

u32 ReadPeriodicTriggerCountLow() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_2, 1);
}

u32 ReadPeriodicTriggerCountHigh() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_2, 2);
}
u64 ReadPeriodicTriggerCount() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_2, 1) | ((u64)(XGpio_DiscreteRead(&axi_gpio_core_status_2, 2)) << 32);
}

u32 ReadCurrentSequenceAddress() {
	//2026 03 18 disabled in block design, as there is timing closure error on address register.
	//proper way to solve this is to do less with address register, potentially by pipelining some of its function.
	return XGpio_DiscreteRead(&axi_gpio_core_status_3, 2);
}

u32 ReadCurrentSequenceCommand_low() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_4, 1);
}

u32 ReadCurrentSequenceCommand_high() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_4, 2);
}

u32 ReadBus() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_5, 1);
}

u32 ReadCoreOptions() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_5, 2);
}

u64 ReadClockCyclesThisRun() {
	return XGpio_DiscreteRead(&axi_gpio_core_status_6, 1) | ((u64)(XGpio_DiscreteRead(&axi_gpio_core_status_6, 2)) << 32);
}

u64 ReadStopWatch() {
	return XGpio_DiscreteRead(&axi_gpio_stop_watch, 1) | ((u64)(XGpio_DiscreteRead(&axi_gpio_stop_watch, 2)) << 32);
}


void ToggleConfigBit(unsigned short BitNr) {
	WriteConfigBit(BitNr,1);
	WriteConfigBit(BitNr,0);
}

void ResetExternalClockLock() {
	ToggleConfigBit(0);
}

void ResetInternalClockLock() {
	ToggleConfigBit(1);
}

void ResetClockWatcher() {
	ToggleConfigBit(2);
}

void ResetCore() {
	ToggleConfigBit(3);
}

void StartSequenceSignalToCore() {
	ToggleConfigBit(4);
}

void PauseSequence() {
	WriteConfigBit(5,1);
}

void ContinueSequence() {
	WriteConfigBit(5,0);
}

void ClearCoreInterrupt() {
	ToggleConfigBit(6);
}

void DisableAndClearCoreInterrupt() {
	WriteConfigBit(6,1);
}

void EnableCoreInterrupt() {
	WriteConfigBit(6,0);
}

void LatchCoreStatus() {
	ToggleConfigBit(7);
}

void SelectExternalClock() {
	WriteConfigBit(8,1);
}

void SelectInternalClock() {
	WriteConfigBit(8,0);
}

void TriggerCore() {
	ToggleConfigBit(9);
}

void SetCoreCondition(unsigned char condition) {
	WriteConfigBit(10,condition > 0);
}

void ResetStopWatch() {
	ToggleConfigBit(11);
}

void SetLED0Red(unsigned char OnOff) {
	WriteConfigBit(12,OnOff > 0);
}

void SetLED0Green(unsigned char OnOff) {
	WriteConfigBit(13,OnOff > 0);
}

void SetLED0Blue(unsigned char OnOff) {
	WriteConfigBit(14,OnOff > 0);
}

void SetLED0Color(unsigned char red, unsigned char green, unsigned char blue) {
	u32 mask = (1+2+4) << 12;
	//if (DebugModeOn) xil_printf(" mask = 0x%x", mask);
	u32 newbits = (((red>0) ? (1<< 12) : 0) | ((green>0) ? (2<< 12) : 0) | ((blue>0) ? (4<< 12) : 0));
	//if (DebugModeOn) xil_printf(" newbits = 0x%x", newbits);
	fabric_config = (fabric_config & ~mask) | newbits;
	//if (DebugModeOn) xil_printf(" config = 0x%x", fabric_config);
	XGpio_DiscreteWrite(&axi_gpio_config_status, 1, fabric_config);
	//if (DebugModeOn) xil_printf("\r\n");
}

void ResetI2C() {
	ToggleConfigBit(15);
}

void SelectExternalClockInput(unsigned char input) {
	WriteConfigBit(16,input > 0);
}

void ResetSelectExternalClock() {
	ToggleConfigBit(17);
}

void ResetSecondaryCoreInterrupt() {
	ToggleConfigBit(18);
}

void ResetInputMemCoreInterrupt() {
	ToggleConfigBit(19);
}

void SetInputMemCoreInterrupt(unsigned char OnOff) {
	WriteConfigBit(19,OnOff > 0);
}

void SetHeartbeat(unsigned char OnOff) {
	WriteConfigBit(20,OnOff > 0); //was 22
}

void SetCPUI2C0Destination(u8 Destination) {
	WriteConfigBit(21,Destination > 0); //was 20, for now just two destinations
}

void SwitchBuzzer(unsigned char OnOff) {
	WriteConfigBit(24,OnOff > 0);  //was 21, 2025 05 16: in the future should be 24
}

void SetPSOptions(u8 value) {
	WriteConfigBits(24,8, value);  //was 21, 2025 05 16: in the future should be 24
}

u32 DMA0_transfer_max_start_space = 0; //in code lines, not clock cycles
u32 DMA0_transfer_max_start_space_worst = 0xffffffff;
int32_t DMA0_transfer_min_stop_safety_space = 1000000; //in code lines, not clock cycles
u32 DMA0_transfer_min_stop_safety_space_worst = 0xffffffff;

u32 DMA1_transfer_max_start_space = 0; //in data entries, not clock cycles
u32 DMA1_transfer_min_stop_safety_space = 10000; //in data entries, not clock cycles
int32_t DMA1_transfer_max_start_space_worst = 0xffff;
u32 DMA1_transfer_min_stop_safety_space_worst = 0xffff;

void FireflyControl_print_statistics(void) {
	if (DebugModeOn) xil_printf("\r\n");
	if (DebugModeOn) xil_printf("Max DMA0 start delay (DMA0 Nr %d) = %d\r\n",DMA0_transfer_max_start_space_worst,DMA0_transfer_max_start_space);
	if (DebugModeOn) xil_printf("Min DMA0 stop space (DMA0 Nr %d) = %d\r\n",DMA0_transfer_min_stop_safety_space_worst, DMA0_transfer_min_stop_safety_space);
	if (DebugModeOn) xil_printf("\r\n");
}


u32 NextDMA0Transfer = 0;
#define DMA0TransferSize 262144
bool LastDMA0Transfer = 1;

u32 NextDMA1Transfer = 0;
u32 InputBufferContentsLength = 0;
#define DMA1TransferSize_in_bytes (BRAM_INPUT_BUFFER_SIZE_IN_BYTES/2)
bool LastDMA1Transfer = 1;

#define UseDMA 0
#define UseDMAForInputBuffer 0

void StopSequenceOnError(u8 error_number) {
	PauseSequence();
	Sequence_ended_in_error = TRUE;
	Sequence_running = FALSE;
	IgnoreTCPIP = FALSE;
	if (DebugModeOn) xil_printf("\n\rStopSequenceOnError(%d)\r\n",error_number);
}
//interrupt init from DMA transfer demo

void DmaDoneHandlerCore(unsigned int Channel) {
	if (Channel ==0) {
		if (NextDMA0Transfer>1) {
			LatchCoreStatus();
			u32 final_extended_addr = ReadLatchedExtendedAddress();
			u32 next_start_address = NextDMA0Transfer * (DMA0TransferSize/8); //in 64-bit words, i.e. program lines
			int32_t Current_DMA0_transfer_min_stop_safety_space = next_start_address - final_extended_addr;
			if (Current_DMA0_transfer_min_stop_safety_space < DMA0_transfer_min_stop_safety_space) {
				DMA0_transfer_min_stop_safety_space = Current_DMA0_transfer_min_stop_safety_space;
				DMA0_transfer_min_stop_safety_space_worst = (NextDMA0Transfer-1);
			}
			if (DMA0_transfer_min_stop_safety_space<1) {
				StopSequenceOnError(0);
				if (DebugModeOn) xil_printf("\r\nDmaDoneHandler DMA0 Nr %d Min stop space = %d\r\n",(-1), Current_DMA0_transfer_min_stop_safety_space);
			}
		}
		DMA0Error = 0;
		DMA0Done = 1;
	} if (Channel == 1) {
		//	unlike the output buffer, the input buffer currently doesn't have an extended address. This means we can't easily check for overflow errors
		/*if (NextDMA1Transfer>1) {
			u32 final_extended_addr = ReadLatchedExtendedAddress();
			u32 next_start_address = NextDMA1Transfer * (DMA1TransferSize/4); //in 64-bit words, i.e. program lines
			int32_t Current_DMA1_transfer_min_stop_safety_space = next_start_address - final_extended_addr;
			if (Current_DMA1_transfer_min_stop_safety_space < DMA1_transfer_min_stop_safety_space) {
				DMA1_transfer_min_stop_safety_space = Current_DMA1_transfer_min_stop_safety_space;
				DMA1_transfer_min_stop_safety_space_worst = (NextDMA1Transfer-1);
			}
			if (DMA1_transfer_min_stop_safety_space<1) {
				//StopSequenceOnError(0);
				if (DebugModeOn) xil_printf("\r\nDmaDoneHandler DMA1 Nr %d Min stop space = %d\r\n",(-1), Current_DMA1_transfer_min_stop_safety_space);
			}
		}*/
		DMA1Error = 0;
		DMA1Done = 1;
	}
}

void DmaDoneHandler(unsigned int Channel,
		    XDmaPs_Cmd *DmaCmd,
		    void *CallbackRef) {
	DmaDoneHandlerCore(Channel);
}

void DmaFaultHandler(unsigned int Channel,
		     XDmaPs_Cmd *DmaCmd,
		     void *CallbackRef) {
	if (Channel == 0) {
		DMA0Error = 1;
		DMA0Done = 1;
		StopSequenceOnError(1);
	} else if (Channel == 1){
		DMA1Error = 1;
		DMA1Done = 1;
	}
}

bool FirstDMA1TransferRequest = TRUE;
bool FinalDMA1TransferRequestHappened = FALSE;
u32 InputBufferContentOriginSequence = 0;
bool LastInputMemHalfBufferReadWasHigh = TRUE;

void ResetInputMemBuffer() {
	NextDMA1Transfer = 0;
	InputBufferContentsLength = 0;
	FirstDMA1TransferRequest = TRUE;
	FinalDMA1TransferRequestHappened = FALSE;
	InputBufferContentOriginSequence = SequenceNumber;
	LastInputMemHalfBufferReadWasHigh = TRUE;
	SetInputMemCoreInterrupt(0);
}

extern void WaitForDMAEndOfTransfer(u32 channel, u32 timeout);
extern void DisableSequenceTransferRequestInterrupt();
extern void EnableSequenceTransferRequestInterrupt();

bool CheckInputMemBufferReadout(bool FinalTransfer) {
	//reminders:
	//	u32* BRAM_input_buffer
	//	u32* DDR_input_buffer
	//	DDR_INPUT_BUFFER_LENGTH_IN_BYTES
	//	BRAM_INPUT_BUFFER_SIZE_IN_BYTES

	//two ways to trigger: over interrupt or over comparison of input_mem_addr with last readout address
	//code for interrupt (somehow not reliable -> comment out)
	/*
	bool InputMemInterruptRequested = GetInputMemInterlockRequested(FALSE);
	if (!((InputMemInterruptRequested) || (FinalTransfer))) return TRUE;
	if (InputMemInterruptRequested) {
		//SetInputMemCoreInterrupt(1);
		ResetInputMemCoreInterrupt();
	}
	*/

	u32 Input_Mem_Address_in_32_bit_words = ReadInputBufferAddress();
	bool next_read_high_buffer = (Input_Mem_Address_in_32_bit_words < (DMA1TransferSize_in_bytes/4));
	bool InputMemInterruptRequested = LastInputMemHalfBufferReadWasHigh != next_read_high_buffer;
	if (!((InputMemInterruptRequested) || (FinalTransfer))) return TRUE;
	LastInputMemHalfBufferReadWasHigh = next_read_high_buffer;
	if (DebugModeOn) {
		xil_printf("Input Mem Buffer Readout requested (DMA1 Nr %d) %s\r\n", NextDMA1Transfer, (FinalTransfer) ? "; final transfer" : "");
		xil_printf("Input_Mem_Address_in_32_bit_words = %d\r\n", Input_Mem_Address_in_32_bit_words);
	}
	/*
	//only if interrupt is used to trigger:
	if (FirstDMA1TransferRequest && (!FinalTransfer) && (Input_Mem_Address_in_32_bit_words < 100)) {
		//The first request is triggered when input buf entry is = address 1,
		//i.e. after just 1 valid entry,
		//ignore that request
		FirstDMA1TransferRequest = FALSE;
		DMA1Error = 0;
		DMA1Done = 1;
		//SetInputMemCoreInterrupt(0);
		if (DebugModeOn) xil_printf("First InputMemInterruptRequest -> ignoring it\n\r");
		return TRUE;
	}*/
	FirstDMA1TransferRequest = FALSE;

	if (FinalTransfer) {
		if (FinalDMA1TransferRequestHappened) {
			//SetInputMemCoreInterrupt(0);
			return TRUE;
		}
		FinalDMA1TransferRequestHappened = TRUE;
		//transfer the buffer half that does contain Input_Mem_Address
		bool read_high_buffer = (Input_Mem_Address_in_32_bit_words >= (DMA1TransferSize_in_bytes/4));
		if (DebugModeOn) {
			xil_printf("FinalTransfer\r\n");
			xil_printf("read_high_buffer = %s\r\n", (read_high_buffer) ? "TRUE" : "FALSE");
			xil_printf("DMA1TransferSize_in_32bit_words = %x\r\n", DMA1TransferSize_in_bytes/4);
			xil_printf("BRAM_input_buffer = %x\r\n", BRAM_input_buffer);
		}
		//32-bit addresses, i.e. adding 1 adds 4 bytes
		Dma1Cmd.BD.SrcAddr = (u32)(BRAM_input_buffer + ((read_high_buffer) ? (DMA1TransferSize_in_bytes/4) : 0));
		Dma1Cmd.BD.DstAddr = (u32)(DDR_input_buffer + NextDMA1Transfer * (DMA1TransferSize_in_bytes/4));  // in 32-bit addresses
		Dma1Cmd.BD.Length = Input_Mem_Address_in_32_bit_words*4 - ((read_high_buffer) ? (DMA1TransferSize_in_bytes) : 0); // + 4; //FS 2024 08 04 Why was here this +4? It seems wrong.
	} else {
		//transfer the buffer half that does not contain Input_Mem_Address
		bool read_high_buffer = (Input_Mem_Address_in_32_bit_words < (DMA1TransferSize_in_bytes/4));

		if (DebugModeOn) {
			xil_printf("not FinalTransfer\r\n");
			xil_printf("read_high_buffer = %s\r\n", (read_high_buffer) ? "TRUE" : "FALSE");
			xil_printf("DMA1TransferSize_in_32bit_words = %x\r\n", DMA1TransferSize_in_bytes/4);
			xil_printf("BRAM_input_buffer = %x\r\n", BRAM_input_buffer);
		}
		//32-bit addresses, i.e. adding 1 adds 4 bytes
		Dma1Cmd.BD.SrcAddr = (u32)(BRAM_input_buffer + ((read_high_buffer) ? (DMA1TransferSize_in_bytes/4) : 0));
		Dma1Cmd.BD.DstAddr = (u32)(DDR_input_buffer + NextDMA1Transfer * (DMA1TransferSize_in_bytes/4));  // in 32-bit addresses
		Dma1Cmd.BD.Length = DMA1TransferSize_in_bytes;
	}

	InputBufferContentsLength += Dma1Cmd.BD.Length;

	if (DebugModeOn) xil_printf("ReadLatchedExtendedAddress\r\n");
	//u32 extended_addr = ReadLatchedExtendedAddress();//is this command needed?
//	unlike the output buffer, the input buffer currently doesn't have an extended address. This means we can't easily check for overflow errors
//	u32 Max_to_transfer_in_bytes=4*Max_DMA1_to_transfer_in_32bit_commands; //conversion from number of 32bit commands to bytes
//	u32 to_tranfer_in_bytes = (Max_to_transfer_in_bytes > DMA1TransferSize_in_bytes) ? DMA1TransferSize_in_bytes : Max_DMA1_to_transfer_in_bytes;
//	Sequence_length_in_32bit_commands_transferred_to_BRAM += to_tranfer_in_bytes/4;
//	u32 next_start_address = (NextDMA1Transfer-1) * (DMA1TransferSize/4); //in 32-bit words, i.e. data entries
//	u32 Current_DMA1_transfer_max_start_space = extended_addr - next_start_address;
//	if (Current_DMA1_transfer_max_start_space > DMA1_transfer_max_start_space) {
//		DMA1_transfer_max_start_space = Current_DMA1_transfer_max_start_space;
//		DMA1_transfer_max_start_space_worst = NextDMA0Transfer;
//	}

	if (InputBufferPosition+(Dma1Cmd.BD.Length/4) > (DDR_INPUT_BUFFER_LENGTH_IN_BYTES/4)) {
		if (DebugModeOn) xil_printf("\nCheckInputMemBufferReadout: input buffer full");
		//SetInputMemCoreInterrupt(0);
		return FALSE;
	}
	if (DebugModeOn) xil_printf("WaitForDMAEndOfTransfer 1\r\n");
	if (UseDMAForInputBuffer) WaitForDMAEndOfTransfer(1,10); //time in ms
	if (DebugModeOn) xil_printf("DisableSequenceTransferRequestInterrupt\r\n");
	DisableSequenceTransferRequestInterrupt(); //ToDo: find correct code for this command
	if (DebugModeOn) xil_printf("WaitForDMAEndOfTransfer 0\r\n");
	WaitForDMAEndOfTransfer(0,10); //time in ms



	//only for debug
	//BRAM_input_buffer[0] = 0xBEEFDEAD;

	//If the BRAM seems not to store data from FPGA core, check if Vivado changed the Block Memory Controler enable pin. It should be always enabled.

	DMA1Done = 0;
	DMA1Error = 0;

	NextDMA1Transfer += 1;
	if (LastDMA1Transfer == 1) LastDMA1Transfer = 0; else LastDMA1Transfer = 1;
	if (UseDMAForInputBuffer) {
		if (DebugModeOn) xil_printf("\r\nDMA Transferring 0x%x = %d bytes from %x to %x\r\n",Dma1Cmd.BD.Length, Dma1Cmd.BD.Length, Dma1Cmd.BD.SrcAddr, Dma1Cmd.BD.DstAddr);
		u32 Status = XDmaPs_Start(&Dma, /*Channel*/ 1, &Dma1Cmd, 0);	// release DMA buffer as we are done
		//if (DebugModeOn) xil_printf("DoSequenceTransfer command sent\r\n");
		if (Status != XST_SUCCESS) {
			if (DebugModeOn) xil_printf("\nCheckInputMemBufferReadout error: problem setting up DMA\r\n");
			//StopSequenceOnError(4);
			EnableSequenceTransferRequestInterrupt();
			//SetInputMemCoreInterrupt(0);
			return FALSE;
		}
	} else {

		u32* source = (u32*)Dma1Cmd.BD.SrcAddr;//(source + NextDMA0Transfer * (DMA0TransferSize/4));
		u32* destination = (u32*)Dma1Cmd.BD.DstAddr;//(destination + ((write_high_buffer) ? (DMA0TransferSize/4) : 0));
		if (DebugModeOn) xil_printf("\n\rCPU transferring 0x%x 32-bit words from %x to %x\r\n",Dma1Cmd.BD.Length/4, (u32)source, (u32)destination);
		//memcpy(destination,source+2,Dma1Cmd.BD.Length-8);
		//destination[0] = source[0];
		//destination[1] = source[1];
		for (u32 i=0; i< Dma1Cmd.BD.Length/4; i++){
			destination[i] = source[i];
		}

		//u32 last = (DmaCmd.BD.Length/4) - 1;
		//destination[last] = source[last];
		DmaDoneHandlerCore(/*channel*/1);
	}
	EnableSequenceTransferRequestInterrupt();


	/*
	//for debug only: display first five entries of the input buffer
	if (DebugModeOn) xil_printf("First 5 entries of the input buffer:\r\n");
	for (u32 i=0; i<5; i++) {
		if (DebugModeOn) xil_printf("Entry %d: DDR %x BRAM %x\r\n",i,DDR_input_buffer[i], BRAM_input_buffer[i]);
	}
	*/

	//if (DebugModeOn) xil_printf("BRAM content other than 0 of 0xFFFFFFFF:\r\n");
	//for (u32 j=0; j<BRAM_INPUT_BUFFER_SIZE_IN_BYTES/4; j++) {
	//	u32 BRAMContent = BRAM_input_buffer[j];
	//	if ((BRAMContent != 0) && (BRAMContent != 0xFFFFFFFF))
	//		if (DebugModeOn) xil_printf("Entry %x: DDR %x BRAM %x\r\n",j,DDR_input_buffer[j], BRAMContent);
	//}

	//SetInputMemCoreInterrupt(0);
	return TRUE;
}


void SendInputBuffer() {
	//unsigned
	char buf[128];
	sprintf(buf,"%lu\n",InputBufferContentOriginSequence);
	if (DebugModeOn) xil_printf(buf);
	server_send_ascii(buf);
	sprintf(buf,"%lu\n",InputBufferContentsLength);
	server_send_ascii(buf);
	if (DebugModeOn) xil_printf(buf);
	if (InputBufferContentsLength>0) {
		server_send_binary(( char *)(DDR_input_buffer),InputBufferContentsLength, DebugModeOn); //length in bytes
	}
}

void CS_send_error_messages() {
	int Nr = CS_GetNrErrorMessages();
	if (Nr == 0) {
		if (DebugModeOn) xil_printf("No error messages\r\n");
		server_send_ascii("0\n");
	} else {
		char buf[128];
		sprintf(buf,"%d\n",Nr);
		server_send_ascii(buf);
		for (int i=0; i<Nr; i++) {
			const char * msg = CS_GetErrorMessage(i);
			if (DebugModeOn) xil_printf("%s\r\n", msg);
			server_send_ascii((char *)msg);
		}
	}
}


bool Extra_DMA0_transfer_request_happened = FALSE;

bool DoSequenceTransfer(bool write_high_buffer){
	if (!DMA0Done) {
		if (DebugModeOn) xil_printf("\n\rDoSequenceTransfer error: previous DMA0 transfer ongoing (DMA0 Nr %d)\r\n",NextDMA0Transfer);
		StopSequenceOnError(2);
		FireflyControl_print_statistics();
		return FALSE;
	}
	if (DMA0Error) {
		if (DebugModeOn) xil_printf("\n\rDoSequenceTransfer error: previous DMA0 transfer went wrong (DMA0 Nr %d)\r\n",NextDMA0Transfer);
		StopSequenceOnError(3);
		FireflyControl_print_statistics();
		return FALSE;
	}
	u32 Max_to_transfer_in_64bit_commands = Sequence_length_in_64bit_commands - Sequence_length_in_64bit_commands_transferred_to_BRAM - Sequence_start_pos_in_64bit_commands;
	if (Max_to_transfer_in_64bit_commands == 0) {
		if (Extra_DMA0_transfer_request_happened) {
			//it's normal that the execution of the last snippet of code
			//triggers an extra interrupt asking for data transfer.
			//Ignore that request.
			//However if a second such request comes,
			//the sequence didn't correctly end in a stop command
			//and we forcefully stop it and display an error.
			if (DebugModeOn) xil_printf("\n\rDoSequenceTransfer error: all data already transferred. DMA0 transfer request too much  (DMA0 Nr %d)\r\n Sequence didn't contain stop command? (cyc = %d)\n\r",NextDMA0Transfer, ReadLatchedWaitCycles());
			StopSequenceOnError(10);
			FireflyControl_print_statistics();
			return FALSE;
		} else {
			if (DebugModeOn) xil_printf("\n\rDoSequenceTransfer: normal extra call (cyc = %d)\n\r",NextDMA0Transfer, ReadLatchedWaitCycles());
			Extra_DMA0_transfer_request_happened = TRUE;
			DisableAndClearCoreInterrupt();
			DMA0Error = 0;
			DMA0Done = 1;
			return TRUE;
		}
	}
	u32 Max_to_transfer_in_bytes=8*Max_to_transfer_in_64bit_commands; //conversion from number of 64bit commands to bytes
	u32 to_tranfer_in_bytes = (Max_to_transfer_in_bytes > DMA0TransferSize) ? DMA0TransferSize : Max_to_transfer_in_bytes;
	Sequence_length_in_64bit_commands_transferred_to_BRAM += to_tranfer_in_bytes/8;
	//XDmaPs_SetDoneHandler(&Dma,0,DmaDoneHandler,0);
	DMA0Done = 0;
	DMA0Error = 0;

	/*XDmaPs_Cmd DmaCmd = {
		.ChanCtrl = {
			.SrcBurstSize = 4,
			.SrcBurstLen = 4,
			.SrcInc = 1,		// increment source address
			.DstBurstSize = 4,
			.DstBurstLen = 4,
			.DstInc = 1,		// increment destination address
		},
	};*/

	//if (DebugModeOn) xil_printf("Transferring block %d to buffer %d\r\n",NextDMA0Transfer, write_high_buffer);
	Dma0Cmd.BD.SrcAddr = (u32)(source + NextDMA0Transfer * (DMA0TransferSize/4) + Sequence_start_pos_in_64bit_commands*2);  // in 32-bit addresses
	Dma0Cmd.BD.DstAddr = (u32)(destination + ((write_high_buffer) ? (DMA0TransferSize/4) : 0));
	Dma0Cmd.BD.Length = to_tranfer_in_bytes;  // usually DMA0TransferSize except for the last transfer or short sequences

	if (((NextDMA0Transfer+1) * DMA0TransferSize) <= (DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES)) {
		NextDMA0Transfer += 1;
		if (LastDMA0Transfer == 1) LastDMA0Transfer = 0; else LastDMA0Transfer = 1;
		//if (DebugModeOn) xil_printf("DoSequenceTransfer ending ok\r\n");
		if (UseDMA) {
			//if (DebugModeOn) xil_printf("\r\nDMA Transferring 0x%x = %d bytes from %x to %x\r\n",DmaCmd.BD.Length, DmaCmd.BD.Length, DmaCmd.BD.SrcAddr, DmaCmd.BD.DstAddr);
			Xil_DCacheFlushRange((UINTPTR)Dma0Cmd.BD.SrcAddr, Dma0Cmd.BD.Length);
			__asm__("dsb"); //insert instruction barrier to be sure cache flush is executed before DMA transfer starts
			u32 Status = XDmaPs_Start(&Dma, /*Channel*/0, &Dma0Cmd, 0);	// release DMA buffer as we are done
			//if (DebugModeOn) xil_printf("DoSequenceTransfer command sent\r\n");
			if (Status != XST_SUCCESS) {
				if (DebugModeOn) xil_printf("\n\rDoSequenceTransfer error: problem setting up DMA\r\n");
				StopSequenceOnError(4);
				return FALSE;
			}
		} else {
			u32* source = (u32*)Dma0Cmd.BD.SrcAddr;//(source + NextDMA0Transfer * (DMA0TransferSize/4));
			u32* destination = (u32*)Dma0Cmd.BD.DstAddr;//(destination + ((write_high_buffer) ? (DMA0TransferSize/4) : 0));
			//if (DebugModeOn) xil_printf("\n\rCPU transferring 0x%x 32-bit words from %x to %x\r\n",Dma0Cmd.BD.Length/4, (u32)source, (u32)destination);
			//memcpy(destination,source+2,Dma0Cmd.BD.Length-8);
			//destination[0] = source[0];
			//destination[1] = source[1];
			for (u32 i=0; i< Dma0Cmd.BD.Length/4; i++){
				destination[i] = source[i];
			}

			//u32 last = (DmaCmd.BD.Length/4) - 1;
			//destination[last] = source[last];
			DmaDoneHandlerCore(/*channel*/ 0);
		}
		return TRUE;
	} else return FALSE;
}

#define MaxInterruptErrorMessageNr 10
#define MaxInterruptErrorMessageLength 100
u8 NextInterruptErrorMessageNr = 0;
u8 NextInterruptErrorMessageNrToDisplay = 1;
u8 NumberOfInterruptErrorMessagesToDisplay = 0;
unsigned char InterruptErrorMessages[MaxInterruptErrorMessageNr * MaxInterruptErrorMessageLength];

void AddInterruptErrorMessage(unsigned char* message) {
	if (NumberOfInterruptErrorMessagesToDisplay == MaxInterruptErrorMessageNr) {
		InterruptErrorMessages[NextInterruptErrorMessageNr * MaxInterruptErrorMessageLength] = 'X';
		InterruptErrorMessages[NextInterruptErrorMessageNr * MaxInterruptErrorMessageLength + 1] = 0;
		return;
	}
	NumberOfInterruptErrorMessagesToDisplay+=1;
	NextInterruptErrorMessageNr+=1;
	if (NextInterruptErrorMessageNr >= MaxInterruptErrorMessageNr) NextInterruptErrorMessageNr = 0;
	//ToDo: finish and use
}


void ExtIrq_Handler(void *InstancePtr)
{
  //if (DebugModeOn) xil_printf("ExtIrq_Handler\r\n");
	DisableAndClearCoreInterrupt();

  if (!DMA0Done) {
	  u32 Current_DMA0_transfer_max_start_space = 0;
	  u32 next_start_address = (NextDMA0Transfer-1) * (DMA0TransferSize/8); //in 64-bit words, i.e. program line
	  while ((!DMA0Done) && (Current_DMA0_transfer_max_start_space<1000)) {
		  LatchCoreStatus();
		  u32 extended_addr = ReadLatchedExtendedAddress();
		  Current_DMA0_transfer_max_start_space = extended_addr - next_start_address;
	  }
  }
//  if (!DMA0Done) {
//	  if (DebugModeOn) xil_printf("\r\nExtIrq_Handler error: last DMA transfer still ongoing (DMA Nr %d)\r\n",NextDMA0Transfer);
//  }

  //u16 addr = ReadCurrentSequenceAddress();
  //bool write_high_buffer = (addr & (1 << 9)) == 0;
  //if (write_high_buffer == LastDMA0Transfer) {
	  LatchCoreStatus();
	  u32 extended_addr = ReadLatchedExtendedAddress();

	  u32 next_start_address = (NextDMA0Transfer-1) * (DMA0TransferSize/8); //in 64-bit words, i.e. program lines
	  u32 Current_DMA0_transfer_max_start_space = extended_addr - next_start_address;
	  if (Current_DMA0_transfer_max_start_space > DMA0_transfer_max_start_space) {
		  DMA0_transfer_max_start_space = Current_DMA0_transfer_max_start_space;
		  DMA0_transfer_max_start_space_worst = NextDMA0Transfer;
	  }

	  //if (DebugModeOn) xil_printf("\r\nExtIrq_Handler DMA Nr %d Max start space = %d\r\n",NextDMA0Transfer,Current_DMA_transfer_max_start_space);

	  u32 TransferToDo = (extended_addr / (DMA0TransferSize/4)) + 1;
	  if (TransferToDo != NextDMA0Transfer) {
		  if (TransferToDo > NextDMA0Transfer) {
			  if (DebugModeOn) xil_printf("\r\nExtIrq_Handler error: too late (DMA0 Nr %d)\r\n",NextDMA0Transfer);
			  StopSequenceOnError(5);
			  //BlockCoreInterrupt();
			  if (DebugModeOn) xil_printf("IRQ: Address %x=%d ToDo %d expected %d cycles %x run %x\r\n", extended_addr, extended_addr, TransferToDo, NextDMA0Transfer, ReadLatchedWaitCycles(), GetRunning(/*from_last_status_read*/ FALSE));
			  return;
		  } //else if (DebugModeOn) xil_printf("\r\nExtIrq_Handler warning: IRQ not needed\r\n");
//		  if (DebugModeOn) xil_printf("Address %x=%d ToDo %d expected %d cycles %x run %x\r\n", extended_addr, extended_addr, TransferToDo, NextDMA0Transfer, ReadLatchedWaitCycles(), GetRunning());
	  }
	  //if (DebugModeOn) xil_printf("\n\rIRQ: Address %x=%d ToDo %d expected %d cycles %x run %x\r\n", extended_addr, extended_addr, TransferToDo, NextDMA0Transfer, ReadLatchedWaitCycles(), GetRunning());
//  }
//  bool write_high_buffer = (extended_addr & (1 << 15)) == 0;
  bool write_high_buffer = (extended_addr & ((BRAM_OUTPUT_BUFFER_LENGTH+1)>>2)) == 0;

  if (DoSequenceTransfer(write_high_buffer) == FALSE) {
	  if (DebugModeOn) xil_printf("\n\rExtIrq_Handler error\r\n");
	  StopSequenceOnError(9);
	  return;
  }
  EnableCoreInterrupt();
}

bool CheckTransferSuccessful(u32 TransferNumber, u32 check_length_in_bytes) {
	//if (DebugModeOn) xil_printf("Checking transfer\r\n");
	//bool use_high_buffer = TransferNumber % 2;
	//u32* source_check = (source + TransferNumber * (DMA0TransferSize/4));
	//u32* destination_check = (destination + ((use_high_buffer) ? (DMA0TransferSize/4) : 0));
	u32* source_check = (u32*)Dma0Cmd.BD.SrcAddr;//(u32)(source + ThisDMATransfer * DMA0TransferSize);
	u32* destination_check = (u32*)Dma0Cmd.BD.DstAddr;// (u32)(destination + ((write_high_buffer) ? DMA0TransferSize : 0));
	//if (DebugModeOn) xil_printf("Checking transfer 0x%x 32-bit words from %x to %x\r\n",Dma0Cmd.BD.Length/4, (u32)source_check, (u32)destination_check);
	bool success = TRUE;
	if (check_length_in_bytes>Dma0Cmd.BD.Length) check_length_in_bytes = Dma0Cmd.BD.Length;
	for (u32 i=0; i< check_length_in_bytes/4; i++){
		if (!(source_check[i] == destination_check[i])) {
			success = FALSE;
			if (DebugModeOn) xil_printf("Transfer bad at %x: s=%x != d=%x\r\n",i,source_check[i], destination_check[i]);
		}
	}
	//if (DebugModeOn) xil_printf("Checking transfer ended\n\r");
	return success;
}

void WaitForDMAEndOfTransfer(u32 channel, u32 timeout) {//ToDo program timeout
	if (channel == 0) {
		while (!DMA0Done);
	}
	if (channel == 1) {
		while (!DMA1Done);
	}
}


//interrupt init from interrupt demo
int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{
 // Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XScuGic_InterruptHandler, XScuGicInstancePtr);
 // Xil_ExceptionEnable();
  return XST_SUCCESS;
}


//This should be defined in xparameters.h but somehow isn't
/* Definitions for Fabric interrupts connected to ps7_scugic_0 */
#define XPAR_FABRIC_AXI_GPIO_4_GPIO_IO_O_INTR 61U
/* fast inmterrupt
 * Core0_nFIQ 28
 */

XScuGic *MyInterruptController = NULL;

void EnableSequenceTransferRequestInterrupt() {
    XScuGic_Enable(MyInterruptController,
  		  XPAR_FABRIC_AXI_GPIO_4_GPIO_IO_O_INTR);
}

void DisableSequenceTransferRequestInterrupt() {
    XScuGic_Disable(MyInterruptController,
  		  XPAR_FABRIC_AXI_GPIO_4_GPIO_IO_O_INTR);
}

int firefly_control_init(XScuGic *InterruptController) {
	MyInterruptController = InterruptController;
	//if (DebugModeOn) xil_printf("firefly_control_init\r\n");
	int Status;

	// Setup DMA Controller
	DmaCfg = XDmaPs_LookupConfig(DMA0_ID);
	if (!DmaCfg) {
		if (DebugModeOn) xil_printf("Lookup DMAC %d failed\r\n", DMA0_ID);
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("firefly_control_init DMA cfg\r\n");

	Status = XDmaPs_CfgInitialize(&Dma,DmaCfg,DmaCfg->BaseAddress);

	if (Status) {
		if (DebugModeOn) xil_printf("XDmaPs_CfgInitialize failed\r\n");
		return XST_FAILURE;
	}


	//if (DebugModeOn) xil_printf("SetupIntrSystem: XScuGic_Connect 1\r\n");


	// Connect the Fault ISR
	Status = XScuGic_Connect(InterruptController,
				 XPAR_XDMAPS_0_FAULT_INTR,
				 (Xil_InterruptHandler)XDmaPs_FaultISR,
				 (void *)(&Dma));
	if (Status != XST_SUCCESS) {
		if (DebugModeOn) xil_printf("SetupIntrSystem: XScuGic_Connect failed\r\n");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("SetupIntrSystem: XScuGic_Connect 2\r\n");

	// Connect the Done ISR for channel 0 of DMA 0
	Status = XScuGic_Connect(InterruptController,
				 XPAR_XDMAPS_0_DONE_INTR_0,
				 (Xil_InterruptHandler)XDmaPs_DoneISR_0,
				 (void *)(&Dma));

	if (Status != XST_SUCCESS) {
		if (DebugModeOn) xil_printf("SetupIntrSystem: XScuGic_Connect failed\r\n");
		return XST_FAILURE;
	}

	//if (DebugModeOn) xil_printf("SetupIntrSystem: XScuGic_Enable 1\r\n");
	// Enable the interrupt for the device
	XScuGic_Enable(InterruptController,
			XPAR_XDMAPS_0_DONE_INTR_0);

	//if (DebugModeOn) xil_printf("SetupIntrSystem: XScuGic_Enable 2\r\n");
	// Enable the interrupt for the device
	///strangely never done in demo
	//XScuGic_Enable(InterruptController,XPAR_XDMAPS_0_FAULT_INTR);

  	XDmaPs_SetDoneHandler(&Dma,0,DmaDoneHandler,0);
  	//if (DebugModeOn) xil_printf("firefly_control_interrupt_init 1\r\n");
  	XDmaPs_SetFaultHandler(&Dma,DmaFaultHandler,0);

    //if (DebugModeOn) xil_printf("firefly_control_interrupt_init: XScuGic_Connect 3\r\n");

    //fast interrupt help
    //https://support.xilinx.com/s/question/0D52E00006iHpK6SAK/zynq-7000-fiq-not-handled-fiqen-bit?language=en_US
    //https://support.xilinx.com/s/question/0D52E00006iHriRSAS/how-to-set-up-the-fiq?language=en_US
    //You need to set clear interrupt register. Please that particular register in user guide on interrupt controller.
//    Status = XScuGic_Connect(&InterruptController, XPS_FIQ_INT_ID,
  //              (Xil_ExceptionHandler)ExtIrq_Handler,
    //            (void *)&InterruptController);


    Status = XScuGic_Connect(InterruptController,
  		  XPAR_FABRIC_AXI_GPIO_4_GPIO_IO_O_INTR, (Xil_ExceptionHandler)ExtIrq_Handler, (void *)NULL);
    if (Status != XST_SUCCESS) {
  	  if (DebugModeOn) xil_printf("firefly_control_interrupt_init: XScuGic_Connect failed\r\n");
  	  return XST_FAILURE;
    }

    //if (DebugModeOn) xil_printf("firefly_control_interrupt_init: XScuGic_Enable 3\r\n");
    EnableSequenceTransferRequestInterrupt();
    //if (DebugModeOn) xil_printf("firefly_control_interrupt_init 0\r\n");

	if (Status != XST_SUCCESS) {
		if (DebugModeOn) xil_printf("SetupIntrSystem failed\r\n");
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}


u64 StartWaitCycles = 0;
u32 LastPLtoPSCommand = 0;
XTime tSequenceStart;
long double last_sequence_print_status_time_in_seconds = 0;
bool FireflyControl_start(unsigned long long start_pos, bool reset_input_buffer) {
	Sequence_start_pos_in_64bit_commands = start_pos;
	if (Sequence_length_in_64bit_commands <= Sequence_start_pos_in_64bit_commands) {
		if (DebugModeOn) xil_printf("FireflyControl_start no sequence to execute\r\n");
		StopSequenceOnError(6);
		return FALSE;
	}
	if (Sequence_start_pos_in_64bit_commands>0) {
		if ((Sequence_length_in_64bit_commands*8) > (DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES - DMA0TransferSize)) {
			if (DebugModeOn) xil_printf("FireflyControl_start sequence too long to not start at buffer position zero\r\n");
			StopSequenceOnError(6);
			return false;
		}
		if ((Sequence_start_pos_in_64bit_commands*8) > (DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES - DMA0TransferSize)) {
			if (DebugModeOn) xil_printf("FireflyControl_start start_pos too high\r\n");
			StopSequenceOnError(6);
			return false;
		}
	}
	SequenceNumber += 1;
	Sequence_length_in_64bit_commands_transferred_to_BRAM = 0;

	Extra_DMA0_transfer_request_happened = FALSE;
	Sequence_ended_in_error = FALSE;
	Sequence_running = FALSE;
	IgnoreTCPIP = FALSE;
	LastPLtoPSCommand = 0;
	InputBufferPosition = 0;
	DMA0Done = 1;
	DMA0Error = 0;
	DMA0_transfer_max_start_space = 0; //in code lines, not clock cycles
	DMA0_transfer_min_stop_safety_space = 10000; //in code lines, not clock cycles
	DMA0_transfer_max_start_space_worst = 0xffff;
	DMA0_transfer_min_stop_safety_space_worst = 0xffff;
	NextDMA0Transfer = 0;

	DMA1Done = 1;
	DMA1Error = 0;
	DMA1_transfer_max_start_space = 0; //in code lines, not clock cycles
	DMA1_transfer_min_stop_safety_space = 10000; //in code lines, not clock cycles
	DMA1_transfer_max_start_space_worst = 0xffff;
	DMA1_transfer_min_stop_safety_space_worst = 0xffff;
	if (reset_input_buffer) ResetInputMemBuffer();


	if (DebugModeOn) xil_printf("FireflyControl_start 0\r\n");

	//XDmaPs_SetDoneHandler(&Dma,0,DmaDoneHandler,0);
	//if (DebugModeOn) xil_printf("StartSequence 1\r\n");
	//XDmaPs_SetFaultHandler(&Dma,DmaFaultHandler,0);
	LastDMA0Transfer = 1;
	LastDMA1Transfer = 1;
	if (DebugModeOn) xil_printf("StartSequence 2\r\n");
	ClearCoreInterrupt();
	//if (DebugModeOn) xil_printf("StartSequence 3\r\n");
	ResetCore();
	//if (DebugModeOn) xil_printf("Transfer block 0\r\n");
	if (!DoSequenceTransfer(0)) {
		if (DebugModeOn) xil_printf("Transfer DMA block 0 not successful at all\r\n");
		StopSequenceOnError(7);
		return FALSE;
	}
	if (DebugModeOn) xil_printf("Waiting for DMA to finish\r\n");
	WaitForDMAEndOfTransfer(0,10); //ToDo: add timeout
	if (DebugModeOn) {
		xil_printf("Checking transfer\r\n");
		if (!CheckTransferSuccessful(0,1000)) {
			xil_printf("Transfer DMA block 0 not successful\r\n");
			StopSequenceOnError(8);
			return FALSE;
		}
		xil_printf("Transfer DMA block 0 successful\r\n");
	}
	LatchCoreStatus();
	//if (DebugModeOn) xil_printf("Send start signal\r\n");
	StartSequenceSignalToCore();
	LatchCoreStatus();
	StartWaitCycles = ReadLatchedWaitCycles();
	XTime_GetTime(&tSequenceStart);
	last_sequence_print_status_time_in_seconds = 0;
	Sequence_running = TRUE;
	return TRUE;
}

void FireflyControl_reset() {
	if (DebugModeOn) xil_printf("FireflyControl_reset\r\n");
	ResetCore();
	ClearCoreInterrupt();
	Sequence_ended_in_error = FALSE;
	Sequence_running = FALSE;
	//Sequence_length_in_64bit_commands = 0;
	Sequence_length_in_64bit_commands_transferred_to_BRAM = 0;
	Sequence_start_pos_in_64bit_commands = 0;
	LastPLtoPSCommand = 0;
}

void FireflyControl_send_sequence() {
	if (DebugModeOn) xil_printf("FireflyControl_send_sequence\r\n");
	unsigned char * number_bytes_as_string = server_get_command(/*timeout*/10.0);
	if (number_bytes_as_string == NULL) {
		if (DebugModeOn) xil_printf("FireflyControl_send_sequence : no data size received\r\n");
		return;
	}
	u32 number_bytes = atoi((char*)number_bytes_as_string);
	if (number_bytes > DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES) {
		if (DebugModeOn) xil_printf("FireflyControl_send_sequence : data exceeds ring buffer size. Use streaming over ethernet to refill data (ToDo: extend Vitis code to allow that)\r\n");
		return; //ToDo: correct error handling
	}
	Sequence_length_in_64bit_commands = number_bytes / 8;
	server_set_binary_mode((u8*)(DDR_MEMORY), DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES, number_bytes);
}

void FireflyControl_modify_sequence() {
	if (DebugModeOn) xil_printf("FireflyControl_modify_data\r\n");
	unsigned char * number_bytes_as_string = server_get_command(/*timeout*/10.0);
	if (number_bytes_as_string == NULL) {
		if (DebugModeOn) xil_printf("FireflyControl_modify_data : no data size received\r\n");
		return;
	}
	u32 number_bytes = atoi((char*)number_bytes_as_string);
	if (number_bytes > DDR_MODIFY_BUFFER_LENGTH_IN_BYTES) {
		if (DebugModeOn) xil_printf("FireflyControl_modify_data : data exceeds modify data buffer size. Use streaming over ethernet to refill data (ToDo: extend Vitis code to allow that)\r\n");
		return; //ToDo: correct error handling
	}
	u32 modify_data_length_in_64bit_commands = number_bytes / 12;
	server_set_binary_mode((u8*)(DDR_MODIFY_BUFFER_START), DDR_MODIFY_BUFFER_LENGTH_IN_BYTES, number_bytes);

	if (!server_wait_till_digital_tranfer_done()) return;
	//modification data contains two tables, one after the other.
	//the first table is a list of 32-bit values, containing the list index of the 64 bit command that needs to be modified
	//the second table contains a list of the new 64-bit commands.
	u32* modify_data_pointers = (u32*)(DDR_MODIFY_BUFFER_START);
	u32* modify_data = (u32*)(DDR_MODIFY_BUFFER_START + modify_data_length_in_64bit_commands*4);
	u32* data =  (u32*)(DDR_MEMORY);
	for (u32 n = 0; n<modify_data_length_in_64bit_commands; n++) {
		u32 index = modify_data_pointers[n];
		if (index < Sequence_length_in_64bit_commands) {
			data[2*index] = modify_data[2*n];
			data[2*index + 1] = modify_data[2*n + 1];
		} else {
			if (DebugModeOn) xil_printf("FireflyControl_modify_data : index higher than sequence long\r\n");
		}
	}
}


void FireflyControl_print_sequence(u32 print_length) {
	if (DebugModeOn) xil_printf("Sequence length = %u\n\r", Sequence_length_in_64bit_commands);
	if (print_length > Sequence_length_in_64bit_commands) print_length = Sequence_length_in_64bit_commands;
	for (u32 n=0;n<print_length;n++) {
		if (DebugModeOn) xil_printf("%02u : %08x %08x\r\n",n,source[n*2+1],source[n*2]);
	}
}

void FireflyControl_RunFastVCOLoop(unsigned long start_buffer_pos, unsigned long AD9854_frequency_buffer_pos) {
	if (DebugModeOn) xil_printf("FireflyControl_RunFastVCOLoop start_buffer_pos = %lu, AD9854_frequency_buffer_pos = %lu\r\n", start_buffer_pos, AD9854_frequency_buffer_pos);
	//For the moment this is just a speed test. How fast could we make the DDS VCO loop?
	
	u32 SrcAddrOutBuf = (u32)(source + start_buffer_pos*2);  //start_buffer_pos is in 64 bit commands, SrcAddrOutBuf in 32-bit addresses
	const u32 DstAddrOutBuf = (u32)(destination);
	const u32 LengthOutBuf = 6*8; //AD9854 frequency tuning word = 48bits; /8 = 6x 64-bit commands, but only high buffer needs to be rewritten, the low buffer stays the same.
	u32* sourceOutBuf = (u32*)SrcAddrOutBuf;//(source + NextDMA0Transfer * (DMA0TransferSize/4));
	u32* destinationOutBuf = (u32*)DstAddrOutBuf;//(destination + ((write_high_buffer) ? (DMA0TransferSize/4) : 0));
		
	const u32 SrcAddr = (u32)(BRAM_input_buffer);
	const u32 DstAddr = (u32)(DDR_input_buffer);  // in 32-bit addresses
	const u32 Length = 4;
	const u32* source = (u32*)SrcAddr;//(source + NextDMA0Transfer * (DMA0TransferSize/4));
	u32* destination = (u32*)DstAddr;//(destination + ((write_high_buffer) ? (DMA0TransferSize/4) : 0));
	if (DebugModeOn) xil_printf("\n\rCPU transferring 0x%x 32-bit words from %x to %x\r\n",Length/4, (u32)source, (u32)destination);
	
	for (unsigned long j=0; j<100000; j++) {
		//quickly transfer the first 4 32-bit words from BRAM to DDR. This word should contain analog input data of last run
		//memcpy(destination,source+2,Dma1Cmd.BD.Length-8);
		//destination[0] = source[0];
		//destination[1] = source[1];
		//for (u32 i=0; i< Length/4; i++){
		//	destination[i] = source[i];
		//}
		destination[0] = source[0];
		//start running next sequence 
		ResetCore();
		StartSequenceSignalToCore();
		Sequence_running = true;
		//while sequence is running, determine the next frequency tuning word
		long long value = destination[0]*10 + 10; //ToDo: correct equation to calculate the frequency tuning word 
		OpticsFoundryFPGA_SetAD9858FrequencyAtBufferPosition(AD9854_frequency_buffer_pos, value);
		//wait till sequence is finished
		while (GetRunning(/*ReadFromLast*/ FALSE)) {}
		Sequence_running = false;
		//Copy new frequency tuning word from DRAM to BRAM
		//only high buffer needs to be rewritten, the low buffer stays the same.
		for (u32 i=1; i< (LengthOutBuf/4)+1; i=i+2) {
			destinationOutBuf[i] = sourceOutBuf[i]; 
		}		
	}
	if (DebugModeOn) xil_printf("FireflyControl_RunFastVCOLoop test ended.");
}

void FireflyControl_stop() {
	FireflyControl_reset();
}

void FireflyControl_set_frequency() {
	unsigned char * frequency = server_get_command(/*timeout*/10.0);
	frequency = frequency;// do something with variable to make C warning disappear
	//ToDo do something?
}

void FireflyControl_get_frequency() {
	server_send_ascii("100\n"); //ToDo: send measured frequency in MHz
}

void FireflyControl_set_trigger() {
	unsigned char * trigger = server_get_command(/*timeout*/10.0);
	trigger = trigger;// do something with variable to make C warning disappear
	//ToDo do something?
}

void FireflyControl_get_current_waveform_point() {
	LatchCoreStatus();
	u64 ElapsedClockCycles = ReadClockCyclesThisRun();//ReadLatchedWaitCycles() - StartWaitCycles;
//	u32 address = ReadLatchedExtendedAddress();
	bool running = GetRunning(/*ReadFromLast*/ FALSE);
	char buf[20];
	sprintf(buf, "%llu\n",ElapsedClockCycles);
	server_send_ascii(buf); //ToDo: send current sequence time, not point
	server_send_ascii((running) ? "1\n" : "0\n");
//	if (DebugModeOn) xil_printf("Elapsed cycles %d\r\n", ElapsedClockCycles);
}

void FireflyControl_get_sequence_number() {
	char buf[20];
	sprintf(buf, "%lu\n",SequenceNumber);
	server_send_ascii(buf); //ToDo: send current sequence time, not point
	if (DebugModeOn) xil_printf("Sequence number = %s", buf);
}

u32 BootWatchdog = 0;

void FireflyControl_get_boot_watchdog() {
	char buf[20];
	sprintf(buf, "%lu\n",BootWatchdog);
	server_send_ascii(buf);
	if (DebugModeOn) xil_printf("FireflyControl_get_boot_watchdog: Boot watchdog = %s", buf);
}

void FireflyControl_set_boot_watchdog() {
	unsigned char * number_as_string = server_get_command(/*timeout*/10.0);
	if (number_as_string == NULL) {
		if (DebugModeOn) xil_printf("FireflyControl_set_boot_watchdog : no data size received\r\n");
		return;
	}
	BootWatchdog = atoi((char*)number_as_string);
	if (DebugModeOn) xil_printf("FireflyControl_set_boot_watchdog : Boot watchdog = %lu\n", BootWatchdog);
}

void print_sequence_status() {
	LatchCoreStatus();
	u32 address = ReadLatchedExtendedAddress();
	if (DebugModeOn) xil_printf("\rDMA0.%d Add %x cmd %x%x bus %x cyc %u ",  NextDMA0Transfer, address, ReadCurrentSequenceCommand_high(), ReadCurrentSequenceCommand_low(), ReadBus(), ReadClockCyclesThisRun()); //ReadLatchedWaitCycles());
	u32 PeriodicTriggerCountLow = ReadPeriodicTriggerCountLow();
	u32 PeriodicTriggerCountHigh = ReadPeriodicTriggerCountHigh();
	if (DebugModeOn) xil_printf("ptc = %x%x ", PeriodicTriggerCountHigh,PeriodicTriggerCountLow);
	bool WaitingForPeriodicTrigger = GetWaitingForPeriodicTrigger();
	if (DebugModeOn) xil_printf("wpt = %d ", (WaitingForPeriodicTrigger) ? 1 : 0);
	u32 Input_Mem_Address_in_32_bit_words = ReadInputBufferAddress();
	if (DebugModeOn) xil_printf("in_mem = %d ", Input_Mem_Address_in_32_bit_words);
}

void PrintSequenceStatusRegularly() {
	XTime tNow;
	XTime_GetTime(&tNow);
	long double sequence_run_time_in_seconds = (long double)((tNow - tSequenceStart) *2)/(long double)COUNTS_PER_SECOND;//XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ;
	//float c = COUNTS_PER_SECOND;
	//wait_time_in_seconds = 1.0 * (tEnd - tStart);
	//wait_time_in_seconds /= c;
	if ((sequence_run_time_in_seconds - last_sequence_print_status_time_in_seconds) > 0.1) {
		last_sequence_print_status_time_in_seconds = sequence_run_time_in_seconds;
		print_sequence_status();
	}
}



extern void sleep(double);

void firefly_control_self_test(void) {
//	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	if (DebugModeOn) xil_printf("firefly_control_init test gpio\r\n");
	configure_axi_gpio();
	//SetLED0Red(1);
	test_ps_gpio();

	if (GetExternalClockLocked()) {
		if (DebugModeOn) xil_printf("external clock locked\r\n");
		SelectExternalClock();
		sleep(0.01);
	} else if (DebugModeOn) xil_printf("external clock not locked\r\n");

   if (GetInternalClockLocked()) {if (DebugModeOn) xil_printf("internal clock locked\r\n");}
   else if (DebugModeOn) xil_printf("internal clock not locked\r\n");
   LatchCoreStatus();
   if (GetInteruptRequested()) {if (DebugModeOn) xil_printf("interrupt requested\r\n");}
   else if (DebugModeOn) xil_printf("interrupt not requested\r\n");

   if (DebugModeOn) xil_printf("Address %x lcyc %x cycles %x\r\n", ReadCurrentSequenceAddress(), ReadLatchedSequenceAddress(), ReadLatchedWaitCycles());

   LatchCoreStatus();

   if (GetInteruptRequested()) {if (DebugModeOn) xil_printf("interrupt requested\r\n");}
   else if (DebugModeOn) xil_printf("interrupt not requested\r\n");

   if (DebugModeOn) xil_printf("Address %x lcyc %x cycles %x\r\n", ReadCurrentSequenceAddress(), ReadLatchedSequenceAddress(), ReadLatchedWaitCycles());



   if (DebugModeOn) xil_printf("Output buffer size = %d 64-bit commands\n\r",DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES/8);
   if (DebugModeOn) xil_printf("Input buffer size = %d 32-bit values\n\r",DDR_INPUT_BUFFER_LENGTH_IN_BYTES/4);
   if (DebugModeOn) xil_printf("firefly_control_self_test end\r\n");
}

u32 next_command = 0;

void StartNewProgram() {
	next_command = 0;
}

void AddCommandBusOut(u32 data, u32 delay ) {
	const u32 delay_mask_low = 0x7FFFFFF; //27 bit
	const u32 delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
	const u32 bus_data_mask = 0x0FFFFFFF;
	const u8 command_mask = 0x1F;  //5 bit
	unsigned char command = 1; //CMD_STEP
	source[next_command * 2 + 0] = ((delay & delay_mask_low) << 5) + (command_mask & command);
	source[next_command * 2 + 1] = ((bus_data_mask & data) << 4) | ((delay >> 27) & delay_mask_high);
	next_command++;
}

void AddCommandSetStrobeOptions(u8 strobe_choice, u8 strobe_low_length, u8 strobe_high_length) {
	const u8 command_mask = 0x1F;  //5 bit
	u8 command = 9; //CMD_SET_STROBE_OPTIONS
	source[next_command * 2 + 0] = ((strobe_high_length & 0xFF) << 24) | ((strobe_low_length & 0xFF) << 16) | ((strobe_choice & 0x07) << 8) | (command_mask & command);
	source[next_command * 2 + 1] = 0;
	next_command++;
}

/*
 CMD_ANALOG_IN_OUT: begin
                            adc_register_address <= command[14:8];  //to read standard analog in, this should be 3, see Xilinx user guide UG480
                            adc_write_enable <= command[15:15];
                            adc_programming_out <= command[31:16];
                            wait_time[29:0] <= command[63:34];
                            wait_time[47:30] <= 0;
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command[32:32] == 0) begin  //if command[32:32] is high, the actual reading will be started trhough CMD_REPEAT
                                if (command[33:33] == 0) ANA_IN_state <= ANA_IN_START; //conversion and register read
                                else ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE; //only register read or write
                            end
                        end
 */
void AddCommandAnalogInOut(u8 adc_register_address, u8 adc_write_enable, u16 adc_programming_out, u8 dont_execute_now, u8 only_read_write, u32 wait_time) {
	const u8 command_mask = 0x1F;  //5 bit
	u8 command = 24; //CMD_ANALOG_IN_OUT
	source[next_command * 2 + 0] = ((adc_programming_out & 0xFFFF) << 16) | ((adc_write_enable & 0x01) << 15) | ((adc_register_address & 0x7F) << 8) | (command_mask & command);
	source[next_command * 2 + 1] = ((wait_time & 0x3FFF) << (34-32)) | ((dont_execute_now & 0x01) << (32-32)) | ((only_read_write & 0x01) << (33-32));
	next_command++;
}

void AddCommandStop() {
	source[next_command * 2 + 0] = 0;
	source[next_command * 2 + 1] = 0;
	Sequence_length_in_64bit_commands = next_command + 1;
}

void AddCommandWriteInputBuffer(u8 write_address, u32 address, u32 data, u32 wait) {
	const u8 command_mask = 0x1F;  //5 bit
	u8 command = 10; //CMD_SET_INPUT_BUF_MEM
	source[next_command * 2 + 0] = ((wait & 0x1FFF) << 19) | ((address & 0x7FF) << 8) | ((write_address & 0x1) << 7) | (command_mask & command);
	source[next_command * 2 + 1] = data;
	next_command++;
}

u32 ReadInputBuffer(u32 n) {
	if (n>(BRAM_INPUT_BUFFER_SIZE_IN_BYTES/4)) {
		if (DebugModeOn) xil_printf("ReadInputBuffer requested data beyond BRAM buffer size. ToDo: Program interrupt driven DMA transfer from BRAM to DDR.\r\n");
		return 0;
	}
	return BRAM_input_buffer[n];
}

void firefly_control_test_adc() {
	//u32 delay = 50;

	DMA0Error = 0;
	DMA0Done = 0;

	// Following code is required only if Cache is enabled
	Xil_DCacheFlushRange((u32)&source, DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES);
	Xil_DCacheFlushRange((u32)&destination, BRAM_OUTPUT_BUFFER_LENGTH);
	Xil_DCacheFlushRange((u32)&BRAM_input_buffer, BRAM_INPUT_BUFFER_SIZE_IN_BYTES);
	Xil_DCacheFlushRange((u32)&DDR_input_buffer, DDR_INPUT_BUFFER_LENGTH_IN_BYTES);

	//create sequence
	u8 max_register = 0x3F;
	StartNewProgram();
	u8 display_registers = 1;
	if (display_registers) {
		for (u8 i=0; i<=max_register; i++) {
			AddCommandAnalogInOut( /*adc_register_address*/ i, /*adc_write_enable*/ 0, /*adc_programming_out*/ 0, /*dont_execute_now*/ 0, /*only_read_write*/1, /*wait_time*/ 50);
			//AddCommandWriteInputBuffer( /*write_address*/ 0, /*address*/i, /*data*/i, /* wait */ 4);
		}
	}
	AddCommandWriteInputBuffer( /*write_address*/ 0, /*address*/0, /*data*/0xDEADBEEF, /* wait */ 4);
	AddCommandWriteInputBuffer( /*write_address*/ 0, /*address*/0, /*data*/0x01234567, /* wait */ 4);
	AddCommandWriteInputBuffer( /*write_address*/ 0, /*address*/0, /*data*/0xABCDEF01, /* wait */ 4);

	u32 NrAnaIn = 2560;
	u32 AnaInDelay = 150;
	for (u32 i=0; i<NrAnaIn; i++) {
		AddCommandAnalogInOut(/* adc_register_address */ 3, /* adc_write_enable */ 0, /* adc_programming_out */ 0, /* dont_execute_now*/ 0, /* only_read_write*/ 0,/*wait_time*/ AnaInDelay);
		//AddCommandAnalogInOut( /*adc_register_address*/ 3, /*adc_write_enable*/ 0, /*adc_programming_out*/ 0, /*dont_execute_now*/ 0, /*only_read_write*/1, /*wait_time*/ delay);
	}
	//AddCommandAnalogInOut( /*adc_register_address*/ 3, /*adc_write_enable*/ 0, /*adc_programming_out*/ 0, /*dont_execute_now*/ 0, /*only_read_write*/1, /*wait_time*/ delay);
	//AddCommandAnalogInOut( /*adc_register_address*/ 3, /*adc_write_enable*/ 0, /*adc_programming_out*/ 0, /*dont_execute_now*/ 0, /*only_read_write*/1, /*wait_time*/ delay);
	AddCommandWriteInputBuffer( /*write_address*/ 0, /*address*/0, /*data*/0xFEEB, /* wait */ 4);
	AddCommandStop();

	if (DebugModeOn) xil_printf("length %x last command %x %x\r\n",Sequence_length_in_64bit_commands,source[(Sequence_length_in_64bit_commands-1)*2+1],source[(Sequence_length_in_64bit_commands-1)*2]);
	// reset timer
	//XScuTimer_RestartTimer(TimerInstancePtr);
	FireflyControl_start(/*start_pos*/ 0, /*reset_input_buffer*/ TRUE);
	FireflyControl_wait_till_finished(10);
	//CntValue1 = XScuTimer_GetCounterValue(TimerInstancePtr);
	//if (DebugModeOn) xil_printf("Sequence took %d clock cycles\r\n", TIMER_LOAD_VALUE-CntValue1);

	//display input buffer
	if (DebugModeOn) xil_printf("\n\r");
	u32 offset = 0;
	if (display_registers) {
		if (DebugModeOn) xil_printf("XADC register content\n\r");
		for (u8 i=0;i<=max_register+1; i++) {
			if (DebugModeOn) xil_printf("[%x]=%x ",i,ReadInputBuffer(i));
		}
		if (DebugModeOn) xil_printf("\n\r");
		offset = max_register+2;
	}
	if (DebugModeOn) xil_printf("data\n\r");
	for (u8 i=0;i<30; i++) {
		if (DebugModeOn) xil_printf("[%x]=%x ",i,ReadInputBuffer(i+offset));
	}
	if (DebugModeOn) xil_printf("\n\r");
	if (DebugModeOn) xil_printf("\n\r");
	offset+=3;
	for (u32 i=0;i<NrAnaIn/10; i++) {
		u32 average = 0;
		for (u8 k=0;k<10; k++) {
			average += ReadInputBuffer(i+offset+k) & 0xFFFF;
		}
		if (DebugModeOn) xil_printf("%d\n\r",average/10);
	}
	if (DebugModeOn) xil_printf("\n\r");

	CheckInputMemBufferReadout(/* LastTransfer */ TRUE);
}


void firefly_control_execute_test_sequence () {
	server_send_ascii("Type in delay of each command ");
	char * delay_string = (char*)server_get_command(/*timeout*/20.0);
	if (delay_string == NULL) {
		if (DebugModeOn) xil_printf("firefly_control_execute_test_sequence : no delay received\r\n");
		return;
	}
	u32 delay = atoi(delay_string);
	if (DebugModeOn) xil_printf("using delay 0x%x = %d\r\n", delay, delay);

	DMA0Error = 0;
	DMA0Done = 0;

	// Following code is required only if Cache is enabled
	Xil_DCacheFlushRange((u32)&source, DDR_OUTPUT_BUFFER_LENGTH_IN_BYTES);
	Xil_DCacheFlushRange((u32)&destination, BRAM_OUTPUT_BUFFER_LENGTH);
	Xil_DCacheFlushRange((u32)&BRAM_input_buffer, BRAM_INPUT_BUFFER_SIZE_IN_BYTES);
	Xil_DCacheFlushRange((u32)&DDR_input_buffer, DDR_INPUT_BUFFER_LENGTH_IN_BYTES);


	//StartNewProgram();
	u32 desired_sequence_length = 100000;  //in 64-bit words

	const u32 delay_mask_low = 0x7FFFFFF; //27 bit
	const u32 delay_mask_high = 0x0F; // 4 bit
	const u32 bus_data_mask = 0x0FFFFFFF;
	const u8 command_mask = 0x1F;  //5 bit

	//u32 delay = 0x7FFFFFFF; //21 seconds (31 bit)
	//u32 delay = 0x07FFFFFF; //1 seconds (31 bit)
	//u32 delay = 0x0003FFFF; //0.001 seconds (31 bit)
	u8 command = 1; //CMD_STEP
	u32 bus_data = 0x0F0F0F0; //28 bit

	for (u32 i=0; i<desired_sequence_length; i++) {
		bus_data = i;
		source[i*2] = ((delay & delay_mask_low) << 5) + (command_mask & command);
		source[i*2+1] = ((bus_data_mask & bus_data) << 4) | ((delay >> 27) & delay_mask_high);
		//AddCommandBusOut(/*data*/ i,  delay );
	}
	next_command = desired_sequence_length;
	AddCommandStop();
	if (DebugModeOn) xil_printf("length %x last command %x %x\r\n",Sequence_length_in_64bit_commands,source[(Sequence_length_in_64bit_commands-1)*2+1],source[(Sequence_length_in_64bit_commands-1)*2]);

	// reset timer
	//XScuTimer_RestartTimer(TimerInstancePtr);

	FireflyControl_start(/*start_pos*/ 0, /*reset_input_buffer*/ TRUE);
	FireflyControl_wait_till_finished(10);

	//CntValue1 = XScuTimer_GetCounterValue(TimerInstancePtr);

	//if (DebugModeOn) xil_printf("Sequence took %d clock cycles\r\n", TIMER_LOAD_VALUE-CntValue1);


}

void firefly_control_test_timer() {
	XTime tStart, tEnd;
	char buf[128];
	XTime_GetTime(&tStart);
	sprintf(buf,"tStart = %llu.\r\n",tStart);
	if (DebugModeOn) xil_printf(buf);
	sleep(1);
	XTime_GetTime(&tEnd);
	sprintf(buf,"tEnd = %llu.\r\n",tEnd);
	if (DebugModeOn) xil_printf(buf);
	long double  c = COUNTS_PER_SECOND;
	sprintf(buf,"COUNTS_PER_SECOND = %15.5Lf s.\r\n",c);
	if (DebugModeOn) xil_printf(buf);
	long double  wait_time_in_seconds2 = 1.0 * (tEnd - tStart);
	sprintf(buf,"ticks during sleep(1) command = %15.5Lf.\r\n",wait_time_in_seconds2);
	if (DebugModeOn) xil_printf(buf);
	wait_time_in_seconds2 /= c;
	sprintf(buf,"wait time during sleep(1) command = %15.5Lf s.\r\n",wait_time_in_seconds2);
	if (DebugModeOn) xil_printf(buf);
}


//UpdateStatusLED is called every time the fast TCPIP timer is called, see RunEventLoop in main.c
//adapt the wait time for the LEDs to whatever you choose as that timers period in platform_zynq.c:
//TimerLoadValue = XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ / (8*128);/// 8;  128 might be too much //ToDo: adjust to make as responsive as needed, but not too fast in order to not waste time on ethernet pulling checks

u8 InFireflyControlLoop = 0;
u8 called_once = 0;
u8 LED0_white = FALSE;
const u32 wait_color_cycles = 8*50;
const u32 wait_white_cycles = 8*10;
u32 LED0_counter = 0;
u8 LED0_color_running = FALSE;


void UpdateStatusLED(struct netif *mynetif) {
	if (Sequence_running) {
		if (!LED0_color_running) {
			SetLED0Color(0,1,0);
			LED0_color_running = 1;
		}
	} else {
		if (LED0_color_running) {
			if (LED0_white) SetLED0Color(1,1,1);
			else if (is_client_connected()) SetLED0Color(0,0,1);
			else SetLED0Color(1,0,0);
			LED0_color_running = 0;
		}
	}
	LED0_counter++;
	if (LED0_white) {
		if (LED0_counter>=wait_white_cycles) {
			LED0_counter=0;
			LED0_white = FALSE;
			if (Sequence_ended_in_error) SetLED0Color(1,0,0);
			else if (LED0_color_running) SetLED0Color(0,1,0);
			else {
				if (is_client_connected()) SetLED0Color(0,0,1);
				else SetLED0Color(0,0,0);
			}
		}
	} else {
		if (LED0_counter>=wait_color_cycles) {
			LED0_counter=0;
			LED0_white = TRUE;
			SetLED0Color(1,1,1);
		}
	}
}


void FireflyFastLoop(struct netif *mynetif) {
	UpdateStatusLED(mynetif);
}



void WriteToSerialPort(u8 NrCommand) {
	//ToDo: serial port communication, from preloaded command list
}


const u8_t IIC_DEVICE_ID[2] = {XPAR_XIICPS_0_DEVICE_ID, XPAR_XIICPS_1_DEVICE_ID};   /* adjust if needed */
#define I2C_BUS_BUSY_TIMEOUT 1000000U
#define I2C_MAX_DATA_SIZE_IN_BYTES 65536
#define I2C_MAX_PORT_NR 1
u8_t I2C_data[I2C_MAX_DATA_SIZE_IN_BYTES];
	

static int firefly_control_wait_for_i2c_idle(XIicPs *Iic, u32 timeout)
{
	while (XIicPs_BusIsBusy(Iic) != 0U) {
		if (timeout == 0U) {
			return XST_FAILURE;
		}
		timeout--;
	}

	return XST_SUCCESS;
}

static int firefly_control_init_i2c0(XIicPs *Iic, u8_t I2C_port, u32 I2C_SCLK_HZ)
{
	XIicPs_Config *Cfg;
	int Status;

	Cfg = XIicPs_LookupConfig(IIC_DEVICE_ID[I2C_port]);
	if (Cfg == NULL) {
		xil_printf("I2C0: config lookup failed\r\n");
		return XST_FAILURE;
	}

	Status = XIicPs_CfgInitialize(Iic, Cfg, Cfg->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("I2C0: init failed: %d\r\n", Status);
		return Status;
	}

	Status = XIicPs_SetSClk(Iic, I2C_SCLK_HZ);
	if (Status != XST_SUCCESS) {
		xil_printf("I2C0: set clock failed: %d\r\n", Status);
		return Status;
	}

	return XST_SUCCESS;
}

int firefly_control_transmit_I2C(u8_t I2C_port, u8_t I2C_ADDR, u32_t send_length, u8_t *send_data, u32_t receive_length, u8_t *receive_data, u32 I2C_SCLK_HZ)
{
	XIicPs Iic;
	u8 reg_addr = 0U;
	int Status = XST_FAILURE;
	if ((send_length == 0U) && (receive_length == 0U)) {
		xil_printf("firefly_control_transmit_I2C: invalid transmit arguments\r\n");
		return XST_FAILURE;
	}

	if ((send_length > 0U) && (send_data == NULL)) {
		xil_printf("firefly_control_transmit_I2C: invalid write arguments\r\n");
		return XST_FAILURE;
	}

	if ((receive_length > 0U) && (receive_data == NULL)) {
		xil_printf("firefly_control_transmit_I2C: receive buffer not provided\r\n");
		return XST_FAILURE;
	}

	if (send_length > 0U) {
		reg_addr = send_data[0];
	}
	Status = firefly_control_init_i2c0(&Iic, I2C_port, I2C_SCLK_HZ);
	if (Status != XST_SUCCESS) {
		xil_printf("firefly_control_transmit_I2C: I2C initialization failed\r\n");
		return Status;
	}

	if ((send_length > 0U) && (receive_length > 0U)) {
		Status = XIicPs_SetOptions(&Iic, XIICPS_REP_START_OPTION);
		if (Status != XST_SUCCESS) {
			xil_printf("firefly_control_transmit_I2C: enable repeated start failed: %d\r\n", Status);
			return Status;
		}
	}

	if (DebugModeOn) {
		xil_printf("firefly_control_transmit_I2C sending %d bytes: [", send_length);
		for (int n = 0; n<send_length; n++) xil_printf("%02x, ", send_data[n]);
		xil_printf("]\r\n");
	}
	if (send_length > 0U) {
		Status = XIicPs_MasterSendPolled(&Iic, send_data, send_length, I2C_ADDR);
		if (Status != XST_SUCCESS) {
			xil_printf("firefly_control_transmit_I2C: send failed: %d\r\n", Status);
			return Status;
		}
	}

	if (receive_length > 0U) {
		if (DebugModeOn) {
			for (u32_t n = 0; n<receive_length; n++) receive_data[n]=0;
		}
		Status = XIicPs_MasterRecvPolled(&Iic, receive_data, receive_length, I2C_ADDR);
		if (send_length > 0U) {
			(void)XIicPs_ClearOptions(&Iic, XIICPS_REP_START_OPTION);
		}
		if (Status != XST_SUCCESS) {
			xil_printf("firefly_control_transmit_I2C: read failed: %d\r\n", Status);
			return Status;
		}
	}
	Status = firefly_control_wait_for_i2c_idle(&Iic, I2C_BUS_BUSY_TIMEOUT);
	if (Status != XST_SUCCESS) {
		xil_printf("firefly_control_transmit_I2C: timeout after read\r\n");
		return Status;
	}

	if (DebugModeOn) {
		if (send_length > 0U) {
			xil_printf("firefly_control_I2C_transmit: wrote %u byte(s), first byte 0x%02x\r\n", send_length, reg_addr);

		}
		if ((send_length > 0U) && (receive_length >= 1U)) {
			xil_printf("firefly_control_I2C_transmit: reg 0x%02x, read %u byte(s), first byte 0x%02x\r\n",
				reg_addr, receive_length, receive_data[0]);
		} else if (receive_length >= 1U) {
			xil_printf("firefly_control_I2C_transmit: read %u byte(s), first byte 0x%02x\r\n",
				receive_length, receive_data[0]);
		}
	}
	if (receive_length >= 1U) {
		if (DebugModeOn) {
			xil_printf("firefly_control_transmit_I2C read %d bytes: [", receive_length);
			u8_t  receive_length_shortened = (receive_length>255) ? 255 : receive_length;
			for (int n = 0; n<receive_length_shortened; n++) xil_printf("%02x, ", receive_data[n]);
			xil_printf("]\r\n");
		}
	}
	return XST_SUCCESS;
}

void FireflyControl_transmit_I2C() {
	if (DebugModeOn) xil_printf("FireflyControl_transmit_I2C\r\n");
	u8 I2C_port = server_read_u8();
	if (I2C_port>I2C_MAX_PORT_NR) I2C_port=I2C_MAX_PORT_NR;
	u8 I2C_destination = server_read_u8();
	u8 I2C_address = server_read_u8();
	u32 I2C_clock_frequency_in_Hz = server_read_u32();
	if (I2C_clock_frequency_in_Hz > 1000000) I2C_clock_frequency_in_Hz = 100000;
	u32 number_bytes_to_send = server_read_u32();
	u32 number_bytes_to_read = server_read_u32();
	if (number_bytes_to_read > I2C_MAX_DATA_SIZE_IN_BYTES) {
		if (DebugModeOn) xil_printf("FireflyControl_transmit_I2C : data exceeds max I2C size. If you really need more, modify the Zynq firmware in Vitis.\r\n");
		return; //ToDo: correct error handling
	}
	if (number_bytes_to_send > I2C_MAX_DATA_SIZE_IN_BYTES) {
		if (DebugModeOn) xil_printf("FireflyControl_transmit_I2C : data exceeds max I2C size. If you really need more, modify the Zynq firmware in Vitis.\r\n");
		return; //ToDo: correct error handling
	}
	if (I2C_port==0) SetCPUI2C0Destination(I2C_destination);
	if (number_bytes_to_send>0) {
		server_set_binary_mode((u8*)(I2C_data), I2C_MAX_DATA_SIZE_IN_BYTES, number_bytes_to_send);
		if (!server_wait_till_digital_tranfer_done()) return;
	}
	int Status = firefly_control_transmit_I2C(I2C_port, I2C_address, number_bytes_to_send, I2C_data, number_bytes_to_read, I2C_data, I2C_clock_frequency_in_Hz);
	if (Status != XST_SUCCESS) {
		if (DebugModeOn) xil_printf("FireflyControl_transmit_I2C : I2C write failed\r\n");
		server_write_u32(0);
	} else {
		if (DebugModeOn) xil_printf("FireflyControl_transmit_I2C : I2C write successful\r\n");
		server_write_u32(1);
		server_write_u32(number_bytes_to_read);
		if (number_bytes_to_read>0) server_send_binary((char*)(I2C_data), number_bytes_to_read, DebugModeOn);
	}
}


void FireflyControl_set_PS_options() {
	if (DebugModeOn) xil_printf("FireflyControl_set_PS_options\r\n");
	u8 options = server_read_u8();
	SetPSOptions(options);
}

u32 lastPLtoPSCommand = 0;
void CheckForPLToPSCommand(){
	u32 currentPLtoPSCommand = GetPLtoPSCommand(/*read_from_last_status*/ FALSE);
	if (lastPLtoPSCommand == currentPLtoPSCommand) return;
	lastPLtoPSCommand = currentPLtoPSCommand;
	u32 subcommand = currentPLtoPSCommand >> 8;
	if (subcommand == 0) {

	} else if (subcommand == 1) {
		if (DebugModeOn) xil_printf("\n\rReceived PL to PS command: 0x%x", currentPLtoPSCommand);
	} else if (subcommand == 2) {
		IgnoreTCPIP = ((currentPLtoPSCommand & 0x01) > 0) && Sequence_running;
		if (DebugModeOn) {
			if (IgnoreTCPIP) {if (DebugModeOn) xil_printf("\n\rIgnore TCPIP");}
			else {if (DebugModeOn) xil_printf("\n\rAccept TCPIP");}
		}
	} else if (subcommand == 3) {
		if (DebugModeOn) xil_printf("\n\rPL to PS command WriteToSerialPort: 0x%x", currentPLtoPSCommand);
		WriteToSerialPort(currentPLtoPSCommand & 0xFF);
	} else {
		if (DebugModeOn) xil_printf("\n\rReceived not yet defined PL to PS command: 0x%x", currentPLtoPSCommand);
	}
}

static double FireflyControl_elapsed_seconds(XTime tStart)
{
	XTime tNow;
	XTime_GetTime(&tNow);
	return (double)(((long double)(tNow - tStart)) / ((long double)COUNTS_PER_SECOND)); //XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ
}


void ProduceHeartBeat() {
	static u8 heartbeat = FALSE;
	static XTime tLastHeartbeat = 0;
	if (fabs(FireflyControl_elapsed_seconds(tLastHeartbeat)) > 0.1) { //5Hz heartbeat
		XTime_GetTime(&tLastHeartbeat);
		heartbeat = !heartbeat;
		SetHeartbeat(heartbeat);
	}
}

bool WaitForInputBufferTransferEnd(double timeout_in_sec) {
	if (DebugModeOn) xil_printf("\r\nWait for input buffer transfer end with timeout %f s\r\n",timeout_in_sec);
	if (UseDMAForInputBuffer) {
		XTime tStart;
		XTime_GetTime(&tStart);
		while ((DMA1Done == 0) && (DMA1Error == 0)) {
			ProduceHeartBeat();
			if (timeout_in_sec>0.001) {
				if (FireflyControl_elapsed_seconds(tStart) >= timeout_in_sec) {
					DMA1Error = 1;
					if (DebugModeOn) xil_printf("\r\nWaitForInputBufferTransferEnd timed out after %f s\r\n", timeout_in_sec);
					return false;
				}
			}
		}
	}
	return true;
}

bool FireflyControl_wait_till_finished(double timeout_in_sec) {
	u32 LastNextDMA0Transfer = NextDMA0Transfer;
	XTime tStart;
	XTime_GetTime(&tStart);
	LatchCoreStatus();
	if (DebugModeOn) xil_printf("\r\nWait for sequence to finish with timeout %f s\r\n",timeout_in_sec);
	//if (DebugModeOn) xil_printf("Add %x cycles %x\r\n", ReadCurrentSequenceAddress(), ReadLatchedWaitCycles());
	while (GetRunning(/*ReadFromLast*/ FALSE)) {
		LatchCoreStatus();
		ProduceHeartBeat();
		if (timeout_in_sec > 0.001) {
			RunEventLoop();
			CheckInputMemBufferReadout(/* LastTransfer */ FALSE);
			CheckForPLToPSCommand();
		}
		if (DebugModeOn) PrintSequenceStatusRegularly();
		//sleep(0.5);
		if (LastNextDMA0Transfer != NextDMA0Transfer){
			//if (DebugModeOn) xil_printf("\r\nnew transfer detected done %d  error %d\r\n", Done, Error);
			if (DMA0Done) {
				LastNextDMA0Transfer = NextDMA0Transfer;
				//CheckTransferSuccessful(NextDMA0Transfer-1);
			}
		}
		if (timeout_in_sec > 0.001) {
			if (FireflyControl_elapsed_seconds(tStart) >= timeout_in_sec) {
				if (DebugModeOn) xil_printf("\r\nFireflyControl_wait_till_finished timed out after %f s\r\n",timeout_in_sec);
				StopSequenceOnError(11);
				FireflyControl_print_statistics();
				return false;
			}
		}
	}
	LatchCoreStatus();
	u32 Input_Mem_Address_in_32_bit_words = ReadInputBufferAddress();
	if (DebugModeOn) xil_printf("\n\rFinished at add 0x%x = %d cmd %x%x bus %x cyc %x in_mem %d\r\n", ReadLatchedExtendedAddress(), ReadLatchedExtendedAddress(), ReadCurrentSequenceCommand_high(), ReadCurrentSequenceCommand_low(), ReadBus(), ReadLatchedWaitCycles(), Input_Mem_Address_in_32_bit_words);
	Sequence_running = FALSE;
	IgnoreTCPIP = FALSE;
	FireflyControl_print_statistics();
	return true;
}

void FireflyControlLoop() {
	//if (DebugModeOn) xil_printf("f");
	if (InFireflyControlLoop) return;
	if (DebugModeOn) xil_printf("fc");
	InFireflyControlLoop = 1;
	if (called_once == 0) {
		configure_axi_gpio();
		called_once = 1;
	}
	CheckInputMemBufferReadout(/* LastTransfer */ FALSE);
	ProduceHeartBeat();
	if (Sequence_running) {
		CheckForPLToPSCommand();
		if (DebugModeOn) PrintSequenceStatusRegularly();
		if (!GetRunning(/*from_last_status_read*/FALSE)) {
			Sequence_running = FALSE;
			IgnoreTCPIP = FALSE;
			FireflyControl_print_statistics();
			if (DebugModeOn) xil_printf("\n\rSequence stopped (next DMA0 Nr %d)", NextDMA0Transfer);
		}
	}
	//if (called_once == 0) {
///		server_send_ascii("send \"help+Enter\" for help");
		//if (DebugModeOn) xil_printf("\r\n");
		//if (DebugModeOn) xil_printf("StartSequence self test\r\n");
		//firefly_control_self_test();
		//if (DebugModeOn) xil_printf("\r\n");
		//if (DebugModeOn) xil_printf("StartSequence DMA test\r\n");
		//firefly_control_execute_test_sequence();
		//if (DebugModeOn) xil_printf("\r\n");
		//if (DebugModeOn) xil_printf("\r\n %d, 0x%x = 0x%x",BRAM_OUTPUT_BUFFER_LENGTH, (BRAM_OUTPUT_BUFFER_LENGTH+1)>>2, (1 << 15));

		//called_once = 1;
	//}
	if (IgnoreTCPIP && Sequence_running) {
		if (DebugModeOn) xil_printf("ignoring TCPIP");
		InFireflyControlLoop = 0;
		return;
	}
	if (DebugModeOn) xil_printf("l ");
	char * command = (char*)server_get_command(/*timeout*/10.0);
	if (command) {
		if (command[strlen(command)-1] != '0') { //ending a command with the character '0' suppresses the answer "Ready\n", which makes fine grained communication speedup possible.
			server_send_ascii("Ready\n"); //confirm receipt of command
		} else {
			command[strlen(command)-1] = 0; //if the command ends in the character '0', we don't send out "Ready\n" and we remove the '0'.
		}
		if (DebugModeOn) xil_printf("Command received: %s\r\n", command);
		if ((strcmp(command,"help") == 0) || (strcmp(command,"h") == 0)) {
			if (DebugModeOn) xil_printf("help: display command list\r\n", command);
			if (DebugModeOn) xil_printf("test1: emit test sequence\r\n", command);
			if (DebugModeOn) xil_printf("test2: simple self test\r\n", command);
			if (DebugModeOn) xil_printf("adc: test adc\r\n", command);
		} else if (strcmp(command,"test2") == 0) {
			if (DebugModeOn) xil_printf("test2: emit test sequence\r\n", command);
			firefly_control_execute_test_sequence();
		} else if (strcmp(command,"test1") == 0) {
			if (DebugModeOn) xil_printf("test1: simple self test\r\n", command);
			firefly_control_self_test();
		} else if (strcmp(command,"test_timer") == 0) {
			if (DebugModeOn) xil_printf("test timer\r\n", command);
			firefly_control_test_timer();
		} else if (strcmp(command,"adc") == 0) {
			if (DebugModeOn) xil_printf("adc_regs: test xadc\r\n", command);
			firefly_control_test_adc();
		} 
		
		else if (strcmp(command,"cs_assemble_sequence") == 0) {
			CS_AssembleSequence();
		} else if (strcmp(command,"cs_add_command") == 0) {
			char * command = (char*)server_get_command(/*timeout*/10.0);
			CS_AddCommand(command);
		} else if (strcmp(command,"cs_execute_sequence") == 0) {			
			char * s_ethernet_check_period = (char*)server_get_command(/*timeout*/10.0);
			unsigned int ethernet_check_period = atoi(s_ethernet_check_period);
			//CommandStatus status =
			CS_ExecuteSequence(ethernet_check_period);
		} else if (strcmp(command,"cs_get_error_message") == 0) {	
			CS_SendErrorMessages();
		} else if (strcmp(command,"cs_stop_sequence") == 0) {	
			CS_StopExecution();
		} else if (strcmp(command,"cs_interrupt_sequence") == 0) {	
			CS_InterruptExecution();
		} else if (strcmp(command,"cs_print_command_table") == 0) {	
			CS_PrintCommandTable();
		} else if (strcmp(command,"cs_print_error_messages") == 0) {	
			CS_PrintErrorMessages();
		} 
		else if (strcmp(command,"close") == 0) {

		} else if (strcmp(command,"select_external_clock_0") == 0) {
			SelectExternalClock();
			SelectExternalClockInput(0);
			ResetSelectExternalClock();
			ResetInternalClockLock();
		} else if (strcmp(command,"select_external_clock_1") == 0) {
			SelectExternalClock();
			SelectExternalClockInput(1);
			ResetSelectExternalClock();
			ResetInternalClockLock();
		} else if (strcmp(command,"select_internal_clock") == 0) {
			SelectInternalClock();
			ResetInternalClockLock();
		} else if (strcmp(command,"reset") == 0) {
			FireflyControl_reset();
		} else if (strcmp(command,"send_sequence") == 0) {
			FireflyControl_send_sequence();
		} else if (strcmp(command,"modify_sequence") == 0) {
			FireflyControl_modify_sequence();
		} else if (strcmp(command,"print_sequence") == 0) {
			FireflyControl_print_sequence(30);
		} else if (strcmp(command,"start") == 0) {
			if (server_last_digital_data_transfer_successful()) FireflyControl_start(/*start_pos*/ 0, /*reset_input_buffer*/ TRUE);
			else if (DebugModeOn) xil_printf("Last send_sequence command not successful, therefore not starting execution.\r\n");
			//FireflyControl_wait_till_finished(60);
		} else if (strcmp(command,"stop") == 0) {
			FireflyControl_stop();
		} else if (strcmp(command,"set_frequency") == 0) {
			FireflyControl_set_frequency();
		} else if (strcmp(command,"get_frequency") == 0) {
			FireflyControl_get_frequency();
		} else if (strcmp(command,"get_mac_address") == 0) {
			char buf[128];
			get_mac_address_as_string(buf);  //change mac address of board in main.c
			server_send_ascii(buf);
		} else if (strcmp(command,"set_trigger") == 0) {
			FireflyControl_set_trigger();
		}else if (strcmp(command,"get_periodic_trigger_error") == 0) {
			server_send_ascii((GetWarningMissedPeriodicTrigger()) ? "1\n" : "0\n");
		} else if (strcmp(command,"wait_till_finished") == 0) {
			double timeout = server_read_double();
			bool success = FireflyControl_wait_till_finished(timeout);
			server_send_ascii((success) ? "1\n" : "0\n"); //send 0 if timed out
		} else if (strcmp(command,"get_current_waveform_point") == 0) {
			FireflyControl_get_current_waveform_point();
		} else if (strcmp(command,"get_sequence_number") == 0) {
			FireflyControl_get_sequence_number();
		} else if (strcmp(command,"check_ready") == 0) {
			//server_send_ascii("Ready\n");
		} else if (strcmp(command,"reset_sequence_number") == 0) {
			SequenceNumber = 0;
		} else if (strcmp(command,"switch_debug_mode_on") == 0) {
			DebugModeOn = TRUE;
		} else if (strcmp(command,"switch_debug_mode_off") == 0) {
			DebugModeOn = FALSE;
		} else if (strcmp(command,"wait_till_end_of_sequence_then_get_input_data") == 0) {
			double sequence_timeout = server_read_double();
			double input_timeout = server_read_double();
			bool success = FireflyControl_wait_till_finished(sequence_timeout);
			server_send_ascii((success) ? "1\n" : "0\n"); //send 0 if timed out
			if (DebugModeOn) xil_printf("Checking input buffer\n\r");
			CheckInputMemBufferReadout(/* LastTransfer */ TRUE);
			if (DebugModeOn) xil_printf("sending missed period trigger warning\n\r");
			server_send_ascii((GetWarningMissedPeriodicTrigger()) ? "1\n" : "0\n");
			if (DebugModeOn) xil_printf("Waiting for input buffer transfer end\n\r");
			success = WaitForInputBufferTransferEnd(input_timeout);
			server_send_ascii((success) ? "1\n" : "0\n"); //send 0 if timed out
			if (DebugModeOn) xil_printf("Sending input buffer\n\r");
			SendInputBuffer();
			if (DebugModeOn) xil_printf("Input buffer sent\n\r");
		} else if (strcmp(command,"get_boot_watchdog") == 0) {
			FireflyControl_get_boot_watchdog();
		} else if (strcmp(command,"set_boot_watchdog") == 0) {
			FireflyControl_set_boot_watchdog();
		} else if (strcmp(command,"transmit_I2C") == 0) {
			FireflyControl_transmit_I2C();
		} else if (strcmp(command,"set_PS_options") == 0) {
			FireflyControl_set_PS_options();
		} else
			if (DebugModeOn) xil_printf("Command not recognized (%s)\r\n", command);
	}
	InFireflyControlLoop = 0;
}

//Install:
//If JTAG programmer not found: install Digilent's Adept runtime.
//to check if JTAG programmer present: install Digilent's Adapt tools and call
//C:\Program Files (x86)\digilent\adeptutilities> .\dadutil enum


//ToDo:
//- design carrier board
//- test fast mode (add command to stop after half a fast mode 64bit command)
//- add RS232
//- test I2C
//- test SPI, IO
//- test fast PL to PS interrupt
//- if fast IRQ working: hook up slow IRQ for input buffer
//- implement input buffer DMA transfer PL to PS
//optional
//- feed more data in from ethernet while running
//- timeouts
//- send error messages back over ethernet
//- add ethernet commands for I2C, SPI, serial port,...
//- interlock safety shutdown sequence
//highly optionally
//- continuously stream data from ZYNQ to ethernet

