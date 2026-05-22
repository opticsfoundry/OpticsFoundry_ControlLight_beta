/*
 * Copyright (C) 2009 - 2019 Xilinx, Inc.
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

#include <stdio.h>
#include <string.h>


#include "lwip/tcp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif

//to enable timer functionality
#include "platform.h"
#include "xparameters.h"
#include "xtime_l.h"
//typedef unsigned char bool;
#include <stdbool.h>

extern bool DebugModeOn;
extern void RunEventLoop(); //defined in main.c

u8 client_connected = 0;

u8 is_client_connected() {
	//if (DebugModeOn) xil_printf("icc ");
	return client_connected;
}

int transfer_data() {
	//if (DebugModeOn) xil_printf("transfer_data called ");
	return 0;
}

void print_app_header()
{
#if (LWIP_IPV6==0)
	{xil_printf("\n\r\n\r----- FireflyControl TCP server IP4 ------\n\r");}
#else
	{if (DebugModeOn) xil_printf("\n\r\n\r----- FireflyControl TCP server IP6 ------\n\r");}
#endif
	{xil_printf("listening on port 7\n\r");}
}

//command format: *command#
#define MaxCommandBuffer 64  //ToDo: adjust to have enough space
#define MaxCommandLength  128 //ToDo: adjust to have enough space, but not too much, as this is a fixed size buffer in PS RA
unsigned char CommandBuffer [MaxCommandBuffer*MaxCommandLength];

u8 ReveiveMode = 0; //0: ASCII commands, 1: binary data into buffer
u8 NextEmptyCommandBufferNumber = 0;
u8 WritePositionInNextEmptyCommandBuffer = 0;
u8 CommandStartDetected = 0;
u8 NextCommandToDo = 0;
u8 NumberCommandsInBuffer = 0;
u8* BinaryBufferStart = NULL;
u32 BinaryBufferLengthInBytes = 0;
u32 BinaryBufferReceivedInBytes = 0;
u32 BinaryBufferExpectedInBytes = 0;
struct tcp_pcb *last_tpcb;
u8 Digital_data_successfully_received = 0;

void server_clear(u32* a_buffer, u32 a_length){
	if (DebugModeOn) xil_printf("server_clear called ");
	ReveiveMode = 0;
	NextEmptyCommandBufferNumber = 0;
	WritePositionInNextEmptyCommandBuffer = 0;
	CommandStartDetected = 0;
	NextCommandToDo = 0;
	NumberCommandsInBuffer = 0;
	BinaryBufferStart = NULL;
	BinaryBufferLengthInBytes = 0;
	BinaryBufferReceivedInBytes = 0;
	BinaryBufferExpectedInBytes = 0;
	Digital_data_successfully_received = 0;
}

void server_set_ascii_mode() {
	if (DebugModeOn) xil_printf("server_set_ascii_mode called ");
	ReveiveMode = 0;
}


unsigned char *server_get_command(long double timeout_in_seconds) {
	if (DebugModeOn) xil_printf("sgc ");
	if (NumberCommandsInBuffer == 0) {
		XTime tStart, tEnd;
    	XTime_GetTime(&tStart);
    	long double wait_time_in_seconds = 0;
		while ((NumberCommandsInBuffer == 0) && (wait_time_in_seconds < timeout_in_seconds)) {
			RunEventLoop();
			//if (NumberCommandsInBuffer == 0) { 
				//sleep(0.001); 	//such a sleep should maybe be in FireflyControlLoop, as that code can decide if it can afford to sleep or needs to check on commands from PL to PS
			//}
			XTime_GetTime(&tEnd);
			wait_time_in_seconds = (long double)((tEnd - tStart) *2)/(long double)XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ;
			//float c = COUNTS_PER_SECOND;
			//wait_time_in_seconds = 1.0 * (tEnd - tStart);
			//wait_time_in_seconds /= c;
		}	
		if (!(wait_time_in_seconds < timeout_in_seconds)) {
			//unsigned char buf[256];
			//sprintf(buf, "server_get_command took %13.5lf sec, which is longer than allowed timeout of %13.5lf sec.\r\n",wait_time_in_seconds, timeout_in_seconds);
			//if (DebugModeOn) xil_printf(buf);
			if (ReveiveMode == 1) { //digital data expected
				server_set_ascii_mode();
				{if (DebugModeOn) xil_printf("Timeout during digital data transfer. Switching back to ASCII mode.\r\n");}
			}
		}
		if (NumberCommandsInBuffer == 0) return NULL;
	}
	unsigned char * return_command = CommandBuffer + MaxCommandLength*NextCommandToDo;
	NextCommandToDo++;
	if (NextCommandToDo>=MaxCommandBuffer) NextCommandToDo = 0;
	NumberCommandsInBuffer--;
	return return_command;
}

u32_t server_read_u32() {
	unsigned char *u32_as_string = server_get_command(/*timeout*/10.0);
	if (u32_as_string == NULL) {
		if (DebugModeOn) xil_printf("server_read_u32 : no data received\r\n");
		return 0;
	}
	return atoi((char*)u32_as_string);
}

u8_t server_read_u8() {
	unsigned char *u8_as_string = server_get_command(/*timeout*/10.0);
	if (u8_as_string == NULL) {
		if (DebugModeOn) xil_printf("server_read_u32 : no data received\r\n");
		return 0;
	}
	return atoi((char*)u8_as_string);
}

double server_read_double() {
	unsigned char *double_as_string = server_get_command(/*timeout*/10.0);
	if (double_as_string == NULL) {
		if (DebugModeOn) xil_printf("server_read_double : no data received\r\n");
		return 0;
	}
	return atof((char*)double_as_string);
}

/*unsigned char *server_get_command(int timeout) {
	if (NumberCommandsInBuffer == 0) {
		if (timeout>0){
			while (NumberCommandsInBuffer == 0) RunEventLoop();//ToDo: implement timeout
			if (NumberCommandsInBuffer == 0) return NULL;
		} else return NULL;
	}
	unsigned char * return_command = CommandBuffer + MaxCommandLength*NextCommandToDo;
	NextCommandToDo++;
	if (NextCommandToDo>=MaxCommandBuffer) NextCommandToDo = 0;
	NumberCommandsInBuffer--;
	return return_command;
}*/

//ToDo: use one of the following two commands to check if digital data transfer was successful before doing anything with that data
u8 server_last_digital_data_transfer_successful() {
	return Digital_data_successfully_received;
}

bool server_wait_till_digital_tranfer_done() {
	XTime tStart, tEnd;
	XTime_GetTime(&tStart);
	long double wait_time_in_seconds = 0;
	long double timeout_in_seconds = 5.0;
	while ((!server_last_digital_data_transfer_successful()) && (wait_time_in_seconds < timeout_in_seconds)) {
		RunEventLoop();
		XTime_GetTime(&tEnd);
		wait_time_in_seconds = (long double)((tEnd - tStart) *2)/(long double)XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ;
	}
	if (!(wait_time_in_seconds < timeout_in_seconds)) {
		if (DebugModeOn) xil_printf("server_wait_till_digital_tranfer_done : didn't receive modification data\r\n");
		return false;
	}
	return true;
}

err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                               struct pbuf *p, err_t err)
{
	/* do not read the packet if we are not in ESTABLISHED state */
	last_tpcb = tpcb;
	if (DebugModeOn) xil_printf("r ");
	if (!p) {
		if (DebugModeOn) xil_printf("\r\nTCP disconnected\r\n");
		client_connected = 0;
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}

	/* indicate that the packet has been received */
	tcp_recved(tpcb, p->len);

		//original code from echo server example
		/* echo back the payload */
		/* in this case, we assume that the payload is < TCP_SND_BUF */
		/*if (tcp_sndbuf(tpcb) > p->len) {
			err = tcp_write(tpcb, p->payload, p->len, 1);
		} else
			if (DebugModeOn) xil_printf("no space in tcp_sndbuf\n\r");*/

	u32 plen = p->len;

	if (ReveiveMode == 1) {  // binary mode
		if (DebugModeOn) xil_printf("server_recv_callback : binary receive %d \r\n", plen);
		if (BinaryBufferStart == NULL){
			if (DebugModeOn) xil_printf("server_recv_callback : no binary buffer set\r\n");
			pbuf_free(p);
			return ERR_MEM;
		}
		u32 ToRead = plen;
		if ((ToRead + BinaryBufferReceivedInBytes) > BinaryBufferExpectedInBytes)
			ToRead = BinaryBufferExpectedInBytes - BinaryBufferReceivedInBytes;
		for (u32 n = 0; n<ToRead; n++) {
			BinaryBufferStart[BinaryBufferReceivedInBytes] = ((u8*)p->payload)[n];
			BinaryBufferReceivedInBytes++;
		}
		
		if (BinaryBufferReceivedInBytes == BinaryBufferExpectedInBytes) {
			ReveiveMode = 0; //switch back to ascii mode
			Digital_data_successfully_received = 1;
		}

		if (ToRead < plen) { //if any data left in input buffer, put it at correct place for ascii mode decoder below
			u32 c = 0;
			for (u32 n = ToRead; n<p->len; n++) {
				((u8*)p->payload)[c] = ((u8*)p->payload)[n];
				c++;
			}
			plen = plen - ToRead;   //this assures that ascii mode decoder uses only correct number of bytes
		} else plen = 0;

	}

	if (ReveiveMode == 0) {  //ASCII mode
		if (DebugModeOn) xil_printf("server_recv_callback : ASCII receive %d \r\n", plen);
		if (plen > (MaxCommandLength - 1 - WritePositionInNextEmptyCommandBuffer)) {
			if (DebugModeOn) xil_printf("server_recv_callback : command too long for command buffer (received len = %d > %d, nextCmd= %d )\r\n", WritePositionInNextEmptyCommandBuffer + plen, MaxCommandLength - 1, NextEmptyCommandBufferNumber );
			//if (DebugModeOn) {
			//	if (plen<100) {
			//		((u8*)p->payload)[plen-1] = 0; 
			//		xil_printf("received = %s\r\n",((u8*)p->payload));
			//	}
			//}
			pbuf_free(p);
			WritePositionInNextEmptyCommandBuffer = 0;
			return ERR_OK; //we just eat the bad data and hope for better times. If we would send ERR_MEM back, we would just repeatedly be presented with same bad data
		}
		if (plen>0) CommandStartDetected = 1;
		for (u8 n = 0; n<plen ; n++) {
			//if (((u8*)p->payload)[n] == '*') {
				//CommandStartDetected = 1;
			//} else
			if (((u8*)p->payload)[n] == '\n') {
				CommandStartDetected = 0;
				//if (WritePositionInNextEmptyCommandBuffer > 0) { //the user might send an empty string by purpose
				if (DebugModeOn) xil_printf("server_recv_callback : ASCII stored, length %d \r\n", WritePositionInNextEmptyCommandBuffer);
				CommandBuffer[NextEmptyCommandBufferNumber*MaxCommandLength + WritePositionInNextEmptyCommandBuffer] = 0; //terminate string with 0
				NumberCommandsInBuffer++;
				if (DebugModeOn) xil_printf("server_recv_callback: received %s\r\n",(char *)(&(CommandBuffer[NextEmptyCommandBufferNumber*MaxCommandLength])));
				NextEmptyCommandBufferNumber++;
				if (NextCommandToDo == NextEmptyCommandBufferNumber) {
					if (DebugModeOn) xil_printf("server_recv_callback : too many untreated commands in command buffer\r\n");
					pbuf_free(p);
					//return ERR_OK; //we just eat the bad data and hope for better times. If we would send ERR_MEM back, we would just repeatedly be presented with same bad data
					return ERR_MEM; //we keep the data in buffer, i.e. we'll get it again, hopefully after some commands have been treated and there is space in buffer
				}
				if (NextEmptyCommandBufferNumber >= MaxCommandBuffer){
					NextEmptyCommandBufferNumber = 0;
				}
				WritePositionInNextEmptyCommandBuffer = 0;
				if (n<(plen-1)) CommandStartDetected = 1;
				//}
			} else if (CommandStartDetected) {
				if (((u8*)p->payload)[n] != '\r') {
					CommandBuffer[NextEmptyCommandBufferNumber*MaxCommandLength + WritePositionInNextEmptyCommandBuffer] = ((u8*)p->payload)[n];
					WritePositionInNextEmptyCommandBuffer++;
					if (WritePositionInNextEmptyCommandBuffer > MaxCommandLength) {
						if (DebugModeOn) xil_printf("server_recv_callback : command exceeds maximum length without new line character. Clipping beginning of command.\r\n");
						WritePositionInNextEmptyCommandBuffer = 0;
					}
				}
			}
		}
	}

	/* free the received pbuf */
	pbuf_free(p);

	return ERR_OK;
}

err_t server_send_ascii(char* buf) { //if *command# structure is needed, I assume calling function has added * and #
	size_t plen = strlen(buf);
	//send it to last client from which we received data
	if (tcp_sndbuf(last_tpcb) > plen) {
		err_t err = tcp_write(last_tpcb, buf, plen, 1);
		if (DebugModeOn) xil_printf("server_send_ascii: sending %s\r\n",buf);
		return err;
	} else {
		if (DebugModeOn) xil_printf("server_send_ascii: no space in tcp_sndbuf\n\r");
		return ERR_MEM;
	}
}

void server_write_u32(u32_t a_u32) {
	char buf[20];
	sprintf(buf, "%u\n",a_u32);
	server_send_ascii(buf);
	if (DebugModeOn) xil_printf("server_write_u32 : wrote %lu\r\n", a_u32);
}

err_t server_send_binary(char* buf, u32 length, bool DebugModeOn) { //if *command# structure is needed, I assume calling function has added * and #
	//size_t plen = length;
	//send it to last client from which we received data
	/*if (tcp_sndbuf(last_tpcb) > plen) {
		err_t err = tcp_write(last_tpcb, buf, plen, 1);
		//if (DebugModeOn) xil_printf("server_send_binary: sending %s\r\n",buf);
		return err;
	} else {
		if (DebugModeOn) xil_printf("server_send_binary: no space in tcp_sndbuf\n\r");
		return ERR_MEM;
	}*/
	//if length > tcp_sndbuf(last_tpcb) we need to split the data into multiple packets
	if (DebugModeOn) xil_printf("server_send_binary: need to send %u bytes\r\n",length);
	if (length == 0) return ERR_OK;
	u32 n = 0;
	err_t err;
	while (n < length) {
		u32 free_buf = tcp_sndbuf(last_tpcb); // get available buffer space for sending (in bytes). 
		if (free_buf > (length - n)) {
			if (DebugModeOn) xil_printf("server_send_binary: sending last %u bytes\r\n",length - n);
			err = tcp_write(last_tpcb, buf + n, length - n, 1);
			return err;
		} else if (free_buf > 0) {
			if (DebugModeOn) xil_printf("server_send_binary: sending %u bytes\r\n",free_buf);
			err = tcp_write(last_tpcb, buf + n, free_buf, 1);
			n += free_buf;
			if (err != ERR_OK) {
				if (DebugModeOn) xil_printf("server_send_binary: no space in tcp_sndbuf\n\r");
				return ERR_MEM;
			}
		}
		//invoke communication to send the data
		err = tcp_output(last_tpcb);
		if (err != ERR_OK) {
			if (DebugModeOn) xil_printf("server_send_binary: could not send\n\r");
			return ERR_MEM;
		}
		RunEventLoop();
	}
	return err;
}

void server_set_binary_mode(u8* a_buffer, u32 a_length, u32 a_expected_length) {
	if (DebugModeOn) xil_printf("server_set_binary_mode : expecting %d bytes\r\n", a_expected_length);
	BinaryBufferStart = a_buffer;
	BinaryBufferLengthInBytes = a_length;
	BinaryBufferReceivedInBytes = 0;
	BinaryBufferExpectedInBytes = a_expected_length; //switch back to ascii mode once this number of bytes has been received
	if (BinaryBufferExpectedInBytes > BinaryBufferLengthInBytes) {
		if (DebugModeOn) xil_printf("server_set_binary_mode : buffer size (%d) not big enough for expected binary data size (%d)\n\r", BinaryBufferLengthInBytes, BinaryBufferExpectedInBytes);
	}
	ReveiveMode = 1;
	Digital_data_successfully_received = 0;
	server_send_ascii("Ready\n"); //This Ready should be needed to avoid receiving binary data in ASCII mode. However, it works without and it doesn't work if I put it. FS 2025 04 02
}

static int echo_connection = 1;

int get_connection_number() {
	if (DebugModeOn) xil_printf("get_connection_number called\r\n");
	return echo_connection-1;
}

// accept_callback is called when a new client connects to this TCP server
err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{

	if (DebugModeOn) xil_printf("\r\nNew TCP client connected\r\n");
	/* set the receive callback for this connection */
	tcp_recv(newpcb, recv_callback);

	/* just use an integer number indicating the connection id as the
	   callback argument */
	tcp_arg(newpcb, (void*)(UINTPTR)echo_connection);

	/* increment for subsequent accepted connections */
	echo_connection++;
	client_connected = 1;

	return ERR_OK;
}


int start_application()
{
	struct tcp_pcb *pcb;
	err_t err;
	unsigned port = 57978;

	/* create new TCP PCB structure */
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		if (DebugModeOn) xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* bind to specified @port */
	err = tcp_bind(pcb, IP_ANY_TYPE, port);
	if (err != ERR_OK) {
		if (DebugModeOn) xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
		return -2;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(pcb, NULL);

	/* listen for connections */
	pcb = tcp_listen(pcb);
	if (!pcb) {
		if (DebugModeOn) xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	/* specify callback to use for incoming connections */
	tcp_accept(pcb, accept_callback);

	if (DebugModeOn) xil_printf("TCP echo server started @ port %d\n\r", port);

	if (DebugModeOn) xil_printf("enter \"help\" for help\r\n");


	return 0;
}
