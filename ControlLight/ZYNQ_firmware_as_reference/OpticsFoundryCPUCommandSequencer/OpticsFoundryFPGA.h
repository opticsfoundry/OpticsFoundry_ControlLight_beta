#pragma once


#ifdef _MSC_VER
using namespace std;
#include <string>

#define xil_printf printf

extern void server_send_ascii(char* buf);
extern char* server_get_command(double timeout);

extern bool DebugModeOn;

extern void SwitchDebugMode(bool OnOff);

#else

#include "../echo.h"
#include "../FireflyControl.h"

#endif

extern void print_double(double val);
extern void PrintUnsignedLongLong(unsigned long long value);

// FPGA sequence control functions

extern void OpticsFoundryFPGA_ExecuteFPGASequenceFrom(unsigned long long buffer_pos);
extern void OpticsFoundryFPGA_WaitTillSequenceFinished(unsigned long long timeout_in_sec);
extern unsigned long OpticsFoundryFPGA_GetInputBufferValue(unsigned long buffer_pos);
extern unsigned long long OpticsFoundryFPGA_GetFPGABuffer(unsigned long long buffer_pos);
extern void OpticsFoundryFPGA_SetFPGABuffer(unsigned long long buffer_pos, unsigned long long value);
extern void OpticsFoundryFPGA_SetAnalogOutAtBufferPosition(unsigned long long buffer_pos, long long value);
extern void OpticsFoundryFPGA_CheckEthernetCommand(int check_ethernet_period_ms);
extern void OpticsFoundryFPGA_SetAD9858FrequencyAtBufferPosition(unsigned long long buffer_pos, long long value);
extern void OpticsFoundryFPGA_SetAD9858AmplitudeAtBufferPosition(unsigned long long buffer_pos, long long value);
extern void OpticsFoundryFPGA_SetAD9858PhaseAtBufferPosition(unsigned long long buffer_pos, long long value);
extern void OpticsFoundryFPGA_SetAD9858UpdateClockAtBufferPosition(unsigned long long buffer_pos, long long value);
extern void OpticsFoundryFPGA_SetAD9858RampRateClockAtBufferPosition(unsigned long long buffer_pos, long long value);
extern void OpticsFoundryFPGA_SetAD9858ControlWordAtBufferPosition(unsigned long long buffer_pos, long long value);
extern void OpticsFoundryFPGA_RunFastVCOLoop(unsigned long start_buffer_pos, unsigned long AD9854_frequency_buffer_pos);
extern void OpticsFoundryFPGA_PrintFPGABuffer(unsigned long print_length);

