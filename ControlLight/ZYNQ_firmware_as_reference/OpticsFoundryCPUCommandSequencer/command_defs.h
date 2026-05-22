#pragma once

#include <stdint.h>


#define MAX_ARGUMENTS        8
#define MAX_ERRORS           16
#define MAX_COMMANDS         16384



typedef enum {
    CMD_SUCCESS = 0,
    CMD_INVALID_OPCODE,
    CMD_INVALID_ARGUMENT,
    CMD_SEQUENCE_OVERFLOW,
    CMD_PARSE_ERROR,
    CMD_REGISTER_OVERFLOW,
    CMD_STRING_TOO_LONG,
    CMD_TYPE_MISMATCH,
    CMD_RUNTIME_ERROR,
    CMD_INVALID_LABEL,
    CMD_UNDEFINED_LABEL,
	CMD_INPUT_BUFFER_EMPTY,
	CMD_INPUT_BUFFER_LOW,
	CMD_OUTPUT_BUFFER_LOW,
	CMD_EXECUTION_ERROR
} CommandStatus;

typedef void (*CommandFunc)(uint16_t* args);

typedef struct {
    const char* name;
    CommandFunc func;
    uint8_t arg_count;
} CommandDef;

typedef struct {
    uint8_t opcode;                  // index into command_defs[]
    uint16_t args[MAX_ARGUMENTS];   // register indices
    uint32_t line_number;           // index of CS_AddCommand call
} CommandEntry;

// Public Globals
extern CommandEntry command_table[MAX_COMMANDS];
extern int command_count;
//extern int add_command_call_count;
extern int debug_mode;

extern int CS_GetCommandCount(void);

// Command registration and execution
//extern void RegisterAllCommands(void);
CommandStatus CS_AddCommand(const char* command_str);
extern void CS_AssembleSequence(void);


// Debug / control
extern void CS_PrintCommandTable(void);
//int GetLastCommandLineNumber(void);

extern CommandDef command_defs[];
extern const int num_commands;


