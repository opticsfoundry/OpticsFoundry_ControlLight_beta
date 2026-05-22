#pragma once

#include "command_defs.h"

extern const char* CS_GetErrorMessage(const int nr);
extern const int CS_GetNrErrorMessages(void);
extern void CS_Reset_errors();
extern void CS_SetError(CommandStatus code, const char* message);
extern void CS_PrintErrorMessages();
extern void CS_SendErrorMessages();


