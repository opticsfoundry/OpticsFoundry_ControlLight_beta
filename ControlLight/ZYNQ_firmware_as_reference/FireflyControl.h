#pragma once

#include <stdbool.h>
#include "netif/xadapter.h"

#define XPAR_AXI_BRAM_CTRL_0_BASEADDR 	XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR
#define XPAR_AXI_BRAM_CTRL_0_HIGHADDR 	XPAR_AXI_BRAM_CTRL_0_S_AXI_HIGHADDR
#define BRAM_MEMORY 					XPAR_AXI_BRAM_CTRL_0_BASEADDR
#define RESET_LOOP_COUNT	10	// Number of times to check reset is done
#define DDR_MEMORY (XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x04000000) // pass all code and data sections; 0x00200000 was enough. I increased it to 32x that for safety.

extern u32 BRAM_OUTPUT_BUFFER_LENGTH;
extern u32 InputBufferContentsLength;
extern u32* DDR_input_buffer;
extern u32 Sequence_length_in_64bit_commands;

extern u8 server_last_digital_data_transfer_successful();
extern bool FireflyControl_start(unsigned long long start_pos, bool reset_input_buffer);
extern bool FireflyControl_wait_till_finished(double timeout_in_sec);
extern bool CheckInputMemBufferReadout(bool FinalTransfer);
extern bool WaitForInputBufferTransferEnd(double timeout_in_sec);
extern void ResetInputMemBuffer();
extern void FireflyFastLoop(struct netif *mynetif);
extern void FireflyControlLoop();
extern void FireflyControl_print_sequence(u32 print_length);
extern void FireflyControl_RunFastVCOLoop(unsigned long start_buffer_pos, unsigned long AD9854_frequency_buffer_pos);
extern bool DebugModeOn;
