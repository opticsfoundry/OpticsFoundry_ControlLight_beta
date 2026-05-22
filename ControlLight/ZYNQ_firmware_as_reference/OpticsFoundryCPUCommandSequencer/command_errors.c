#include "command_defs.h"
#include <string.h>
#include <stdio.h>
#include "executor.h"
#include "OpticsFoundryFPGA.h"


#ifndef _MSC_VER

#include "xil_printf.h"

#endif

static char error_messages[MAX_ERRORS][128];
static int error_count = 0;

const char* CS_GetErrorMessage(const int nr) {
    static char result[1024];
    result[0] = '\0'; // Initialize result string
	if (nr < 0 || nr >= error_count) {
		snprintf(result, sizeof(result), "Error message index %d out of range (0 to %d)", nr, error_count - 1);
		return result;
	}
	strcat(result, error_messages[nr]);
    strcat(result, "\n");
    return result;
}

const int CS_GetNrErrorMessages(void) {
    return error_count;
}

void CS_Reset_errors() {
	error_count = 0;
}

void CS_SetError(CommandStatus code, const char* message) {
    if (error_count >= MAX_ERRORS) return;
	//int code_line = -1;
	if (CS_ExecutionIsRunning()) {
		int code_line = CS_GetNextCommandLineNumber();
		snprintf(error_messages[error_count++], 128, "Execution error %d in line %d: %s, command %s", code, code_line, message, command_defs[command_table[code_line].opcode].name);
	} 
	else
		snprintf(error_messages[error_count++], 128, "Parsing error %d in line %d: %s", code, CS_GetCommandCount(), message);
	if (DebugModeOn) {
		// Print the error message to the console
		xil_printf("Error %d: %s\r\n", code, message);
	}
}


void CS_PrintErrorMessages() {
	int Nr = CS_GetNrErrorMessages();
	if (Nr == 0) {
		xil_printf("No error messages\r\n");
	}
	else {
		for (int i = 0; i < Nr; i++) {
			const char* msg = CS_GetErrorMessage(i);
#ifdef _MSC_VER
			printf("%s\r\n", msg);
#else
			xil_printf("%s\r\n", msg);
#endif
		}
	}
}

void CS_SendErrorMessages() {
	int Nr = CS_GetNrErrorMessages();
	char buf[256];
	sprintf(buf,"%d\n",Nr);
	server_send_ascii(buf);
	for (int i = 0; i < Nr; i++) {
		const char* msg = CS_GetErrorMessage(i);
		sprintf(buf,"%s\n", msg);
		server_send_ascii(buf);
	}
	error_count = 0;
}

