#pragma once

#include "command_defs.h"
#include <stdbool.h>

extern void CS_ExecutionJumpTo(int command_line);
extern void CS_PrintCommandTable(void);
extern int CS_GetNextCommandLineNumber();
extern void CS_StopExecution();
extern void CS_InterruptExecution(void);

extern bool CS_ExecutionIsRunning(void);

#ifndef _MSC_VER


extern CommandStatus CS_ExecuteSequence(int check_ethernet_period_ms);


#else

extern CommandStatus CS_ExecuteSequence(int check_ethernet_period_ms = 100);

#endif
