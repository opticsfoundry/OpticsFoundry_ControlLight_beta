#include "command_defs.h"
#include <string.h>
#include <stdio.h>
#include "label_table.h"
#include "name_table.h"
#include "pid.h"
#include "command_errors.h"
#include "registers.h"

CommandEntry command_table[MAX_COMMANDS];
int command_count = 0;



void CS_DefineVariables(void) {
    // Set ContinueExecution to 1
    registers[MAX_USER_REGISTERS].type = REG_INT;
    registers[MAX_USER_REGISTERS].value.i = 1;
    CS_AddCommand("Equal(ContinueExecution,1);"); //thos will define variable ContinueExecution in registers[MAX_USER_REGISTERS] 
    CS_AddCommand("Equal(pi,3.1415926535897932384626433);");
    CS_AddCommand("Equal(e,0.57721566490153286060651209008240243104215933593992);");
    CS_AddCommand("Equal(c,299792458);");
    CS_AddCommand("Equal(hbar,1.05457182E-34);");
    CS_AddCommand("Equal(h,6.62607015E-34);");
}

void CS_StopExecution() {
    // Set ContinueExecution to 0
    registers[MAX_USER_REGISTERS].type = REG_INT;
    registers[MAX_USER_REGISTERS].value.i = 0;
}

void CS_AssembleSequence(void) {
    command_count = 0;
    reset_label_table();
    reset_name_table();
    CS_Reset_errors();
    CS_DefineVariables();
}


// Prototypes from command_impl.c
extern void cmd_equal(uint16_t*);
extern void cmd_add(uint16_t*);
extern void cmd_sub(uint16_t*);
extern void cmd_mul(uint16_t*);
extern void cmd_div(uint16_t*);
extern void cmd_neg(uint16_t*);

extern void cmd_acos(uint16_t*);
extern void cmd_asin(uint16_t*);
extern void cmd_atan(uint16_t*);
extern void cmd_cos(uint16_t*);
extern void cmd_cosh(uint16_t*);
extern void cmd_exp(uint16_t*);
extern void cmd_fabs(uint16_t*);
extern void cmd_log(uint16_t*);
extern void cmd_log10(uint16_t*);
extern void cmd_sin(uint16_t*);
extern void cmd_sinh(uint16_t*);
extern void cmd_sqrt(uint16_t*);
extern void cmd_tan(uint16_t*);
extern void cmd_tanh(uint16_t*);
extern void cmd_pow(uint16_t*);

extern void cmd_toint(uint16_t*);
extern void cmd_todouble(uint16_t*);
extern void cmd_tostring(uint16_t*);
extern void cmd_cmpeq(uint16_t*);
extern void cmd_cmpne(uint16_t*);
extern void cmd_cmplt(uint16_t*);
extern void cmd_cmpgt(uint16_t*);
extern void cmd_cmple(uint16_t*);
extern void cmd_cmpge(uint16_t*);

extern void cmd_bitand(uint16_t* args);
extern void cmd_bitor(uint16_t* args);
extern void cmd_bitxor(uint16_t* args);
extern void cmd_bitshiftleft(uint16_t* args);
extern void cmd_bitshiftright(uint16_t* args);
extern void cmd_bitnot(uint16_t* args);

extern void cmd_jumpifzero(uint16_t*);
extern void cmd_jumpifnotzero(uint16_t*);
extern void cmd_jump(uint16_t*);
extern void cmd_recvint(uint16_t*);
extern void cmd_recvdouble(uint16_t*);
extern void cmd_recvstring(uint16_t*);
extern void cmd_send(uint16_t*);
extern void cmd_print(uint16_t*);
extern void cmd_printhex(uint16_t*);
extern void cmd_printbinary(uint16_t*);
extern void cmd_setregister(uint16_t*);
extern void cmd_getregister(uint16_t*);
extern void cmd_executeFPGAsequence(uint16_t*);
extern void cmd_waittillsequencefinished(uint16_t*);
extern void cmd_getinputbuffervalue(uint16_t*);
extern void cmd_getFPGAbuffer(uint16_t*);
extern void cmd_setFPGAbuffer(uint16_t*);

//AD9854 commands
extern void cmd_setAD9854frequency(uint16_t*);
extern void cmd_setAD9854amplitude(uint16_t*);
extern void cmd_setAD9854phase(uint16_t*);
extern void cmd_setAD9854updateclock(uint16_t*);
extern void cmd_setAD9854ramprateclock(uint16_t*);
extern void cmd_setAD9854controlword(uint16_t*);



extern void cmd_setanalogout(uint16_t*);
extern void cmd_runfastvcoloop(uint16_t*);
extern void cmd_printfpgabuffer(uint16_t*);
extern void cmd_pid(uint16_t*);
extern void cmd_checkethernetcommand(uint16_t*);


CommandDef command_defs[] = {
    { "Equal",     cmd_equal,     2 },
    { "Add",       cmd_add,       3 },
    { "Sub",       cmd_sub,       3 },
    { "Mul",       cmd_mul,       3 },
    { "Div",       cmd_div,       3 },
    { "Neg",       cmd_neg,       2 },

    { "ACos",       cmd_acos,       2 },
    { "ASin",       cmd_asin,       2 },
	{ "ATan",       cmd_atan,       2 },
	{ "Cos",        cmd_cos,        2 },
	{ "Cosh",       cmd_cosh,       2 },
	{ "Exp",        cmd_exp,        2 },
	{ "Fabs",       cmd_fabs,       2 },
	{ "Log",        cmd_log,        2 },
	{ "Log10",      cmd_log10,      2 },
	{ "Sin",        cmd_sin,        2 },
	{ "Sinh",       cmd_sinh,       2 },
	{ "Sqrt",       cmd_sqrt,       2 },
	{ "Tan",        cmd_tan,        2 },
	{ "Tanh",       cmd_tanh,       2 },
	{ "Pow",        cmd_pow,        3 },
    
    { "ToInt",     cmd_toint,     2 },
    { "ToDouble",  cmd_todouble,  2 },
    { "ToString",  cmd_tostring,  2 },
    { "CmpEq",     cmd_cmpeq,     3 },
    { "CmpNe",     cmd_cmpne,     3 },
    { "CmpLt",     cmd_cmplt,     3 },
    { "CmpGt",     cmd_cmpgt,     3 },
    { "CmpLe",     cmd_cmple,     3 },
    { "CmpGe",     cmd_cmpge,     3 },
	{ "And",    cmd_bitand,    3 },
	{ "Or",     cmd_bitor,     3 },
	{ "Xor",    cmd_bitxor,    3 },
	{ "ShiftLeft", cmd_bitshiftleft, 3 },
	{ "ShiftRight",cmd_bitshiftright,3 },
	{ "Not",    cmd_bitnot,    2 },
    { "JumpIfZero",    cmd_jumpifzero,    2 },
    { "JumpIfNotZero", cmd_jumpifnotzero, 2 },
	{ "Jump",     cmd_jump,      1 },
    { "RecvInt",   cmd_recvint,   1 },
    { "RecvDouble",cmd_recvdouble,1 },
    { "RecvString",cmd_recvstring,1 },
    { "Send",cmd_send,1 },
    { "Print",  cmd_print,  1 },
    { "PrintHex",  cmd_printhex,  1 },
    { "PrintBinary",  cmd_printbinary,  1 },
    { "SetRegister", cmd_setregister, 2 },
	{ "GetRegister", cmd_getregister, 2 },
    { "ExecuteFPGASequence", cmd_executeFPGAsequence, 1 },
	{ "WaitTillSequenceFinished", cmd_waittillsequencefinished, 1 },
	{ "GetInputBufferValue", cmd_getinputbuffervalue, 2 },
	{ "GetFPGABuffer", cmd_getFPGAbuffer, 1 },
	{ "SetFPGABuffer", cmd_setFPGAbuffer, 2 },




    //AD9854 commands
	{ "SetAD9854Frequency", cmd_setAD9854frequency, 2 },
    { "SetAD9854Amplitude", cmd_setAD9854amplitude, 2 },
	{ "SetAD9854Phase", cmd_setAD9854phase, 2 },
	{ "SetAD9854UpdateClock", cmd_setAD9854updateclock, 2 },
	{ "SetAD9854RampRateClock", cmd_setAD9854ramprateclock, 2 },
	{ "SetAD9854ControlWord", cmd_setAD9854controlword, 2 },

    { "SetAnalogOut", cmd_setanalogout, 2 },
    
    { "RunFastVCOLoop", cmd_runfastvcoloop, 2 },
    { "PrintFPGABuffer", cmd_printfpgabuffer, 1},
	{ "CheckEthernetCommand", cmd_checkethernetcommand, 1 },
	{ "PID", cmd_pid, 7 } //Usage: PID(out_register, target, measurement, p, i, d, pid_nr);

};

const int num_commands = sizeof(command_defs) / sizeof(CommandDef);


//void RegisterAllCommands(void) {
    // Currently static; in future, support dynamic registration
//}


