#include "OpticsFoundryFPGA.h"

#ifndef _MSC_VER

#include "stdio.h"
#include "../FireflyControl.h"
#include "../main.h"
#include "command_errors.h"
#include "xil_printf.h"

void OpticsFoundryFPGA_ExecuteFPGASequenceFrom(unsigned long long buffer_pos) {
	//we do not reset the DRAM input buffer just yet, so that we are able to work with the last input while the next one is acquired
	//this only works if the input length is shorter than the BRAM input buffer.
	if (server_last_digital_data_transfer_successful()) FireflyControl_start(buffer_pos, /*reset_input_buffer*/ FALSE);
	else {
		CS_SetError(CMD_EXECUTION_ERROR, "ExecuteFPGASequenceFrom: Last send_sequence command not successful, therefore not starting execution.");
		if (DebugModeOn) xil_printf("ExecuteFPGASequenceFrom: Last send_sequence command not successful, therefore not starting execution.");
	}
}

void OpticsFoundryFPGA_WaitTillSequenceFinished(unsigned long long timeout_in_sec) {
	FireflyControl_wait_till_finished(timeout_in_sec);
	if (DebugModeOn) xil_printf("Checking input buffer\n\r");
	//only now we reset the DRAM input buffer, then we read it.
	//this only works if the input length is shorter than the BRAM input buffer.
	ResetInputMemBuffer();
	CheckInputMemBufferReadout(/* LastTransfer */ TRUE);
	WaitForInputBufferTransferEnd(timeout_in_sec);
}

unsigned long OpticsFoundryFPGA_GetInputBufferValue(unsigned long buffer_pos) {
	if (InputBufferContentsLength == 0) {
		CS_SetError(CMD_INPUT_BUFFER_EMPTY, "GetInputBufferValue : Input buffer empty");
		return 0;
	}
	if (buffer_pos > InputBufferContentsLength) {
		char buf[300];
		sprintf(buf, "GetInputBufferValue(%lu); buffer contains only %lu values", buffer_pos, InputBufferContentsLength);
		CS_SetError(CMD_INPUT_BUFFER_LOW, buf);
		return 0;
	}
	return DDR_input_buffer[buffer_pos];
}



void OpticsFoundryFPGA_SetFPGABuffer(unsigned long long buffer_pos, unsigned long long value) {
	if (buffer_pos >= Sequence_length_in_64bit_commands) {
		char buf[300];
		sprintf(buf, "SetFPGABuffer(%llu): sequence buffer contains only %lu values", buffer_pos, Sequence_length_in_64bit_commands);
		CS_SetError(CMD_OUTPUT_BUFFER_LOW, buf);
		return;
	}
	((u64*)(DDR_MEMORY))[buffer_pos] = value;
}

unsigned long long OpticsFoundryFPGA_GetFPGABuffer(unsigned long long buffer_pos) {
	if (buffer_pos >= Sequence_length_in_64bit_commands) {
		char buf[300];
		sprintf(buf, "GetFPGABuffer(%llu): sequence buffer contains only %lu values", buffer_pos, Sequence_length_in_64bit_commands);
		CS_SetError(CMD_OUTPUT_BUFFER_LOW, buf);
		return 0;
	}
	return ((u64*)(DDR_MEMORY))[buffer_pos];
}

void OpticsFoundryFPGA_SetAD9858Value(unsigned long long buffer_pos, long long value, unsigned int length_in_bytes) {
	if (buffer_pos >= (Sequence_length_in_64bit_commands - length_in_bytes - 1)) {
		char buf[300];
		sprintf(buf, "SetAD9858AmplitudeAtBufferPosition(%llu): sequence buffer contains only %lu values", buffer_pos, Sequence_length_in_64bit_commands);
		CS_SetError(CMD_OUTPUT_BUFFER_LOW, buf);
		return;
	}

	/*
	 CMD_STEP:begin
			wait_time[30:0] <= command[35:5];
			wait_time[47:31] <= 0;
			bus_data <= command[63:36]; // [31:4]
			if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
			strobe_generator_state <= DELAY_CYCLE;
		end
	*/
	//we only need to update the bus data, i.e. bits [31:4] of the high buffer words
	// of those, we only need to update the DDS data, which is using D8..D15 of the data bus,
	//  [11:4] is D7..D0
	//  [19:12] is D15..D8, i.e. what we need to update
	for (unsigned int n = 0; n < length_in_bytes; n++) {
		((u32*)(DDR_MEMORY))[2 * (buffer_pos + n) + 1] &= ~0xFF000;
		((u32*)(DDR_MEMORY))[2 * (buffer_pos + n) + 1] |= (0xFF000 & (value << 12));
		value = value >> 8;
	}
}

void OpticsFoundryFPGA_SetAD9858FrequencyAtBufferPosition(unsigned long long buffer_pos, long long value) {
	OpticsFoundryFPGA_SetAD9858Value(buffer_pos, value, 6);
}

void OpticsFoundryFPGA_SetAD9858AmplitudeAtBufferPosition(unsigned long long buffer_pos, long long value)
{
	OpticsFoundryFPGA_SetAD9858Value(buffer_pos, value, 2);
}

void OpticsFoundryFPGA_SetAD9858PhaseAtBufferPosition(unsigned long long buffer_pos, long long value) {
	OpticsFoundryFPGA_SetAD9858Value(buffer_pos, value, 2);
}

void OpticsFoundryFPGA_SetAD9858UpdateClockAtBufferPosition(unsigned long long buffer_pos, long long value) {
	OpticsFoundryFPGA_SetAD9858Value(buffer_pos, value, 4);
}

void OpticsFoundryFPGA_SetAD9858RampRateClockAtBufferPosition(unsigned long long buffer_pos, long long value) {
	OpticsFoundryFPGA_SetAD9858Value(buffer_pos, value, 3);
}

void OpticsFoundryFPGA_SetAD9858ControlWordAtBufferPosition(unsigned long long buffer_pos, long long value) {
	OpticsFoundryFPGA_SetAD9858Value(buffer_pos, value, 1);
}

void OpticsFoundryFPGA_SetAnalogOutAtBufferPosition(unsigned long long buffer_pos, long long value) {
	if (buffer_pos >= (Sequence_length_in_64bit_commands - 7)) { //-7: 6 for 48bit frequency tuning word and 1 for stop command
		char buf[300];
		sprintf(buf, "SetAnalogOutAtBufferPosition(%llu): sequence buffer contains only %lu values", buffer_pos, Sequence_length_in_64bit_commands);
		CS_SetError(CMD_OUTPUT_BUFFER_LOW, buf);
		return;
	}
	/*
	 CMD_STEP:begin
			wait_time[30:0] <= command[35:5];
			wait_time[47:31] <= 0;
			bus_data <= command[63:36]; // [31:4]
			if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
			strobe_generator_state <= DELAY_CYCLE;
		end
	*/
	//we only need to update the bus data, i.e. bits [31:4] of the high buffer words
	// of those, we only need to update the DDS data, which is using D0..D15 of the data bus,
	//  [19:4] is D15..D0, i.e. what we need to update
	((u32*)(DDR_MEMORY))[2 * (buffer_pos)+1] &= ~0xFFFF0;
	((u32*)(DDR_MEMORY))[2 * (buffer_pos)+1] |= (0xFFFF0 & (value << 4));
}

void OpticsFoundryFPGA_CheckEthernetCommand(int check_ethernet_period_ms) {
	//ToDo: only run every check_ethernet_period_ms
	RunEventLoop();
}



void OpticsFoundryFPGA_RunFastVCOLoop(unsigned long start_buffer_pos, unsigned long AD9854_frequency_buffer_pos) {
	FireflyControl_RunFastVCOLoop(start_buffer_pos, AD9854_frequency_buffer_pos);
}

void OpticsFoundryFPGA_PrintFPGABuffer(unsigned long print_length) {
	FireflyControl_print_sequence(print_length);
}




#else

#include <iostream>

bool DebugModeOn = false;

void SwitchDebugMode(bool OnOff) {
	DebugModeOn = OnOff;
	if (DebugModeOn) {
		std::cout << "Debug mode ON" << std::endl;
	}
	else {
		std::cout << "Debug mode OFF" << std::endl;
	}
}

// External Ethernet and debug functions

void server_send_ascii(char* buf) {
	printf("%s", buf);
}

// Dummy server Ethernet I/O
char* server_get_command(double timeout) {
	static char buffer[128] = "42";
	return buffer;
}

void OpticsFoundryFPGA_ExecuteFPGASequenceFrom(unsigned long long buffer_pos) {}
void OpticsFoundryFPGA_WaitTillSequenceFinished(unsigned long long timeout_in_sec) {}
unsigned long OpticsFoundryFPGA_GetInputBufferValue(unsigned long buffer_pos) { return 0; }
void OpticsFoundryFPGA_SetAD9858FrequencyAtBufferPosition(unsigned long long buffer_pos, long long value) {}
void OpticsFoundryFPGA_SetFPGABuffer(unsigned long long buffer_pos, unsigned long long value) {}
unsigned long long OpticsFoundryFPGA_GetFPGABuffer(unsigned long long buffer_pos) { return 0; }
void OpticsFoundryFPGA_SetAD9858AmplitudeAtBufferPosition(unsigned long long buffer_pos, long long value) {}
void OpticsFoundryFPGA_SetAnalogOutAtBufferPosition(unsigned long long buffer_pos, long long value) {}
void OpticsFoundryFPGA_CheckEthernetCommand(int check_ethernet_period_ms) {}
void OpticsFoundryFPGA_SetAD9858PhaseAtBufferPosition(unsigned long long buffer_pos, long long value) {}
void OpticsFoundryFPGA_SetAD9858UpdateClockAtBufferPosition(unsigned long long buffer_pos, long long value) {}
void OpticsFoundryFPGA_SetAD9858RampRateClockAtBufferPosition(unsigned long long buffer_pos, long long value) {}
void OpticsFoundryFPGA_SetAD9858ControlWordAtBufferPosition(unsigned long long buffer_pos, long long value) {}
void OpticsFoundryFPGA_RunFastVCOLoop(unsigned long start_buffer_pos, unsigned long AD9854_frequency_buffer_pos) {}
void OpticsFoundryFPGA_PrintFPGABuffer(unsigned long print_length) {}

#endif


void print_double(double val) {
	if (val < 0) {
		xil_printf("-");
		val = -val;
	}

	int int_part = (int)val;
	int frac_part = (int)((val - int_part) * 1000000.0 + 0.5); // rounding

	xil_printf("%d.%06d", int_part, frac_part);
}

void PrintUnsignedLongLong(unsigned long long value) {
	int64_t val = value;
	uint32_t high = val >> 32;
	uint32_t low = (uint32_t)(val & 0xFFFFFFFF);
	xil_printf("[0x%08x 0x%08x] = [%u %u]", high, low, high, low);
}
