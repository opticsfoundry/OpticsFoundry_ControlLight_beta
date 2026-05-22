#include "command_defs.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "OpticsFoundryFPGA.h"
#include "label_table.h"
#include "executor.h"
#include "registers.h"
#include "command_errors.h"
#include <math.h>

#ifndef _MSC_VER

#include "../echo.h"
#include "xil_printf.h"

#endif


// Macros for common logic
#define ARG(i) (args[i])
#define REG(i) ((ARG(i) < MAX_REGISTERS) ? &registers[ARG(i)] : &registers[MAX_REGISTERS-1]) //add error code if register number too high



// ----------- Arithmetic and String Ops -----------


void cmd_equal(uint16_t* args) {
    Register* dst = REG(0), * a = REG(1);
    if (a->type == REG_INT) {
        dst->type = REG_INT;
        dst->value.i = a->value.i;
    }
    else if (a->type == REG_DOUBLE) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d;
    }
    else if (a->type == REG_STRING) {
        dst->type = REG_STRING;
        snprintf(dst->value.s, MAX_STRING_LENGTH, "%s", a->value.s);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "Add type mismatch");
    }
}

void cmd_add(uint16_t* args) {
    Register* dst = REG(0), * a = REG(1), * b = REG(2);
    if (a->type == REG_INT && b->type == REG_INT) {
        dst->type = REG_INT;
        dst->value.i = a->value.i + b->value.i;
    }
    else if (a->type == REG_DOUBLE && b->type == REG_DOUBLE) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d + b->value.d;
    }
    else if (a->type == REG_DOUBLE && b->type == REG_INT) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d + b->value.i;
    }
    else if (a->type == REG_INT && b->type == REG_DOUBLE) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.i + b->value.d;
    }
    else if (a->type == REG_STRING && b->type == REG_STRING) {
        dst->type = REG_STRING;
        char help[256];
        snprintf(help, 256, "%s%s", a->value.s, b->value.s);
        strncpy(dst->value.s, help, MAX_STRING_LENGTH);
        dst->value.s[MAX_STRING_LENGTH - 1] = '\0';
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "Add type mismatch");
    }
}

void cmd_sub(uint16_t* args) {
    Register* dst = REG(0), * a = REG(1), * b = REG(2);
    if (a->type == REG_INT && b->type == REG_INT) {
        dst->type = REG_INT;
        dst->value.i = a->value.i - b->value.i;
    }
    else if (a->type == REG_DOUBLE && b->type == REG_DOUBLE) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d - b->value.d;
    }
    else if (a->type == REG_DOUBLE && b->type == REG_INT) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d - b->value.i;
    }
    else if (a->type == REG_INT && b->type == REG_DOUBLE) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.i - b->value.d;
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "Sub type mismatch");
    }
}

void cmd_mul(uint16_t* args) {
    Register* dst = REG(0), * a = REG(1), * b = REG(2);
    if (a->type == REG_INT && b->type == REG_INT) {
        dst->type = REG_INT;
        dst->value.i = a->value.i * b->value.i;
    }
    else if (a->type == REG_DOUBLE && b->type == REG_DOUBLE)
    {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d * b->value.d;
    }
    else if (a->type == REG_DOUBLE && b->type == REG_INT)
    {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d * b->value.i;
    }
    else if (a->type == REG_INT && b->type == REG_DOUBLE)
    {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.i * b->value.d;
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "Mul type mismatch");
    }
}

void cmd_div(uint16_t* args) {
    Register* dst = REG(0), * a = REG(1), * b = REG(2);
    if ((b->type == REG_INT && b->value.i == 0) ||
        (b->type == REG_DOUBLE && b->value.d == 0.0)) {
        CS_SetError(CMD_RUNTIME_ERROR, "Division by zero");
        return;
    }

    if (a->type == REG_INT && b->type == REG_INT) {
        dst->type = REG_INT;
        dst->value.i = a->value.i / b->value.i;
    }
    else if (a->type == REG_DOUBLE && b->type == REG_DOUBLE) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d / b->value.d;
    }
    else if (a->type == REG_DOUBLE && b->type == REG_INT) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.d / b->value.i;
    }
    else if (a->type == REG_INT && b->type == REG_DOUBLE) {
        dst->type = REG_DOUBLE;
        dst->value.d = a->value.i / b->value.d;
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "Div type mismatch");
    }
}

void cmd_neg(uint16_t* args) {
    Register* dst = REG(0), * a = REG(1);
    if (a->type == REG_INT) {
        dst->type = REG_INT;
        dst->value.i = -a->value.i;
    }
    else if (a->type == REG_DOUBLE) {
        dst->type = REG_DOUBLE;
        dst->value.d = -a->value.d;
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "Neg type mismatch");
    }
}

// ----------- Mathematics -----------

#define DEFINE_MATH(name, op) \
void name(uint16_t* args) { \
    Register* dst = REG(0), * a = REG(1); \
    dst->type = REG_DOUBLE; \
    if (a->type == REG_INT || a->type == REG_DOUBLE) {   \
        dst->value.d = op((a->type == REG_DOUBLE) ? a->value.d : (double)a->value.i); \
    } \
    else CS_SetError(CMD_TYPE_MISMATCH, #name " type mismatch"); \
}


DEFINE_MATH(cmd_acos, acos)
DEFINE_MATH(cmd_asin, asin)
DEFINE_MATH(cmd_atan, atan)
DEFINE_MATH(cmd_cos, cos)
DEFINE_MATH(cmd_cosh, cosh)
DEFINE_MATH(cmd_exp, exp)
DEFINE_MATH(cmd_fabs, fabs)
DEFINE_MATH(cmd_log, log)
DEFINE_MATH(cmd_log10, log10)
DEFINE_MATH(cmd_sin, sin)
DEFINE_MATH(cmd_sinh, sinh)
DEFINE_MATH(cmd_sqrt, sqrt)
DEFINE_MATH(cmd_tan, tan)
DEFINE_MATH(cmd_tanh, tanh)

//etc. add functions as needed

#define DEFINE_MATH2(name, op) \
void name(uint16_t* args) { \
    Register* dst = REG(0), * a = REG(1), * b = REG(1); \
    dst->type = REG_DOUBLE; \
    if ((a->type == REG_INT || a->type == REG_DOUBLE) && (b->type == REG_INT || b->type == REG_DOUBLE)) {   \
        double d_a= (a->type == REG_DOUBLE) ? a->value.d : (double)a->value.i; \
        double d_b = (b->type == REG_DOUBLE) ? b->value.d : (double)b->value.i; \
        dst->value.d = op(d_a, d_b); \
    } \
    else CS_SetError(CMD_TYPE_MISMATCH, #name " type mismatch"); \
}


DEFINE_MATH2(cmd_pow, pow)



// ----------- Type Conversions -----------

void cmd_toint(uint16_t* args) {
    Register* dst = REG(0), * src = REG(1);
    dst->type = REG_INT;
    if (src->type == REG_DOUBLE) dst->value.i = (int64_t)(src->value.d);
    else if (src->type == REG_STRING) dst->value.i = atoll(src->value.s);
    else dst->value.i = src->value.i;
}

void cmd_todouble(uint16_t* args) {
    Register* dst = REG(0), * src = REG(1);
    dst->type = REG_DOUBLE;
    if (src->type == REG_INT) dst->value.d = (double)(src->value.i);
    else if (src->type == REG_STRING) dst->value.d = atof(src->value.s);
    else dst->value.d = src->value.d;
}

void cmd_tostring(uint16_t* args) {
    Register* dst = REG(0), * src = REG(1);
    dst->type = REG_STRING;
    switch (src->type) {
    case REG_INT:    snprintf(dst->value.s, MAX_STRING_LENGTH, "%lld", src->value.i); break;
    case REG_DOUBLE: snprintf(dst->value.s, MAX_STRING_LENGTH, "%f", src->value.d); break;
    case REG_STRING: strncpy(dst->value.s, src->value.s, MAX_STRING_LENGTH); break;
    }
}

// ----------- Comparisons -----------

#define DEFINE_CMP(name, op) \
void name(uint16_t* args) { \
    Register *dst = REG(0), *a = REG(1), *b = REG(2); \
    dst->type = REG_INT; \
    if (a->type == REG_INT && b->type == REG_INT) dst->value.i = (a->value.i op b->value.i); \
    else if (a->type == REG_DOUBLE && b->type == REG_DOUBLE) dst->value.i = (a->value.d op b->value.d); \
    else if (a->type == REG_DOUBLE && b->type == REG_INT) dst->value.i = (a->value.d op b->value.i); \
    else if (a->type == REG_INT && b->type == REG_DOUBLE) dst->value.i = (a->value.i op b->value.d); \
    else CS_SetError(CMD_TYPE_MISMATCH, #name " type mismatch"); \
}

DEFINE_CMP(cmd_cmpeq, == )
DEFINE_CMP(cmd_cmpne, != )
DEFINE_CMP(cmd_cmplt, < )
DEFINE_CMP(cmd_cmpgt, > )
DEFINE_CMP(cmd_cmple, <= )
DEFINE_CMP(cmd_cmpge, >= )


    // ----------- Bit manipulation -----------

#define DEFINE_BIT(name, op) \
void name(uint16_t* args) { \
    Register *dst = REG(0), *a = REG(1), *b = REG(2); \
    dst->type = REG_INT; \
    if (a->type == REG_INT && b->type == REG_INT) dst->value.i = (a->value.i op b->value.i); \
    else CS_SetError(CMD_TYPE_MISMATCH, #name " type mismatch"); \
}

DEFINE_BIT(cmd_bitand, &)
DEFINE_BIT(cmd_bitor, | )
DEFINE_BIT(cmd_bitxor, ^)
DEFINE_BIT(cmd_bitshiftleft, << )
DEFINE_BIT(cmd_bitshiftright, >> )

void cmd_bitnot(uint16_t* args) {
    Register* dst = REG(0), * a = REG(1);
    dst->type = REG_INT;
    if (a->type == REG_INT) dst->value.i = ~a->value.i;
    else CS_SetError(CMD_TYPE_MISMATCH, "BitNot type mismatch");
}

// ----------- Flow Control -----------

void cmd_jumpifzero(uint16_t* args) {
    Register* cond = REG(0), * target_label = REG(1);
    if ((cond->type != REG_INT) || (target_label->type != REG_INT)) {
        CS_SetError(CMD_TYPE_MISMATCH, "cmd_jumpifzero type mismatch");
        return;
    }
    if (cond->value.i != 0) return;
    if (target_label->value.i >= get_max_label_nr()) {
        CS_SetError(CMD_INVALID_LABEL, "cmd_jumpifzero label unknown");
        return;
    }
    CS_ExecutionJumpTo(get_label_table_line_number(target_label->value.i));
}

void cmd_jumpifnotzero(uint16_t* args) {
    Register* cond = REG(0), * target_label = REG(1);
    if ((cond->type != REG_INT) || (target_label->type != REG_INT)) {
        CS_SetError(CMD_TYPE_MISMATCH, "cmd_jumpifzero type mismatch");
        return;
    }
    if (cond->value.i == 0) return;
    if (target_label->value.i >= get_max_label_nr()) {
        CS_SetError(CMD_INVALID_LABEL, "cmd_jumpifzero label unknown");
        return;
    }
    CS_ExecutionJumpTo(get_label_table_line_number(target_label->value.i));
}

void cmd_jump(uint16_t* args) {
    Register* target_label = REG(0);
    if (target_label->type != REG_INT) {
        CS_SetError(CMD_TYPE_MISMATCH, "cmd_jump type mismatch");
        return;
    }
    if (target_label->value.i >= get_max_label_nr()) {
        CS_SetError(CMD_INVALID_LABEL, "cmd_jump label unknown");
        return;
    }
    CS_ExecutionJumpTo(get_label_table_line_number(target_label->value.i));
}

// ----------- Ethernet I/O -----------

void cmd_recvint(uint16_t* args) {
    Register* dst = REG(0);
    dst->type = REG_INT;
    dst->value.i = atoll((char*)server_get_command(10.0));
}

void cmd_recvdouble(uint16_t* args) {
    Register* dst = REG(0);
    dst->type = REG_DOUBLE;
    dst->value.d = atof((char*)server_get_command(10.0));
}

void cmd_recvstring(uint16_t* args) {
    Register* dst = REG(0);
    dst->type = REG_STRING;
    strncpy(dst->value.s, (char*)server_get_command(10.0), MAX_STRING_LENGTH - 1);
    dst->value.s[MAX_STRING_LENGTH - 1] = '\0';
}

void cmd_send(uint16_t* args) {
    Register* a = REG(0);
    char buf[MAX_STRING_LENGTH];
    switch (a->type) {
    case REG_INT:    snprintf(buf, MAX_STRING_LENGTH, "%lld", a->value.i); break;
    case REG_DOUBLE: snprintf(buf, MAX_STRING_LENGTH, "%f", a->value.d); break;
    case REG_STRING: strncpy(buf, a->value.s, MAX_STRING_LENGTH); break;
    }
    server_send_ascii(buf);
}

// ----------- Debug Print -----------

void cmd_print(uint16_t* args) {
    Register* a = REG(0);
    char buf[MAX_STRING_LENGTH];
    switch (a->type) {
    case REG_INT:    snprintf(buf, MAX_STRING_LENGTH, "%lld", a->value.i); break;
    case REG_DOUBLE: snprintf(buf, MAX_STRING_LENGTH, "%f", a->value.d); break;
    case REG_STRING: strncpy(buf, a->value.s, MAX_STRING_LENGTH); break;
    }
    xil_printf(buf);
}

void cmd_printhex(uint16_t* args) {
    Register* a = REG(0);
    char buf[MAX_STRING_LENGTH];
    switch (a->type) {
    case REG_INT: {
        unsigned int w0 = (unsigned long long)a->value.i & 0xFFFF;
        unsigned int w1 = ((unsigned long long)a->value.i >> 16) & 0xFFFF;
        unsigned int w2 = ((unsigned long long)a->value.i >> 32) & 0xFFFF;
        unsigned int w3 = ((unsigned long long)a->value.i >> 48) & 0xFFFF;
        snprintf(buf, MAX_STRING_LENGTH, "%04X%04X %04X%04X", w3, w2, w1, w0);
        break;
    }
    case REG_STRING: strncpy(buf, a->value.s, MAX_STRING_LENGTH); break;
    case REG_DOUBLE: snprintf(buf, MAX_STRING_LENGTH, "%f", a->value.d); break;
    }
    xil_printf(buf);
}

void cmd_printbinary(uint16_t* args) {
    Register* a = REG(0);
    char buf[MAX_STRING_LENGTH];
    switch (a->type) {
    case REG_INT: {
        char* buf_ptr = buf;
        unsigned long long value = a->value.i;
        for (int i = 63; i >= 0; i--) {
            *buf_ptr = ((value & (1ULL << i)) ? '1' : '0');
            buf_ptr++;
            if (i % 8 == 0) {
                *buf_ptr = ' '; // optional: space every 8 bits
                buf_ptr++;
            }
        }
        *buf_ptr = '\0'; // terminate string
        break;
    }
    case REG_STRING: strncpy(buf, a->value.s, MAX_STRING_LENGTH); break;
    case REG_DOUBLE: snprintf(buf, MAX_STRING_LENGTH, "%f", a->value.d); break;
    }
    xil_printf(buf);
}


void cmd_getregister(uint16_t* args) {
    //GetRegister(dest,source_register_index)
    // copies the value of register with index source_register_index into register dest
    //e.g.
    //GetRegister(R0,20)
    Register* dst = REG(0), * reg_nr = REG(1);
    if (reg_nr->type == REG_INT) {
        if (reg_nr->value.i < 0 || reg_nr->value.i >= MAX_REGISTERS) {
            CS_SetError(CMD_REGISTER_OVERFLOW, "GetRegister: Register index out of bounds");
            return;
        }
        Register* source = &registers[reg_nr->value.i];
        *dst = *source;
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "GetRegister type mismatch");
    }
}


void cmd_setregister(uint16_t* args) {
    //SetRegister(dest_register_index, source)
    // copies the value of register with index source_register_index into register dest
    //e.g.
    //SetRegister(20, R0)
    Register* reg_nr = REG(0), * src = REG(1);
    if (reg_nr->type == REG_INT) {
        if (reg_nr->value.i < 0 || reg_nr->value.i >= MAX_REGISTERS) {
            CS_SetError(CMD_REGISTER_OVERFLOW, "SetRegister: Register index out of bounds");
            return;
        }
        Register* dest = &registers[reg_nr->value.i];
        *dest = *src;
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "SetRegister type mismatch");
    }
}



//FPGA related commands

void cmd_executeFPGAsequence(uint16_t* args) {
    Register* a = REG(0);
    if (a->type == REG_INT) {
        OpticsFoundryFPGA_ExecuteFPGASequenceFrom(a->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "ExecuteFPGASequence type mismatch");
    }
}

void cmd_waittillsequencefinished(uint16_t* args) {
    Register* a = REG(0);
    if (a->type == REG_INT) {
        OpticsFoundryFPGA_WaitTillSequenceFinished(a->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "WaitTillSequenceFinished type mismatch");
    }
}

void cmd_getinputbuffervalue(uint16_t* args) {
    Register* dst = REG(0), * a = REG(1);
    if (a->type == REG_INT) {
        dst->type = REG_INT;
        dst->value.i = OpticsFoundryFPGA_GetInputBufferValue(a->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "GetInputBufferValue type mismatch");
    }
}

extern void cmd_getFPGAbuffer(uint16_t* args) {
    Register* dest = REG(0), * buffer_pos = REG(1);
    if ((buffer_pos->type == REG_INT)) {
        dest->type = REG_INT;
        dest->value.i = OpticsFoundryFPGA_GetFPGABuffer(buffer_pos->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "GetFPGABuffer type mismatch");
    }
}

extern void cmd_setFPGAbuffer(uint16_t* args) {
    Register* buf_pos = REG(0), * value = REG(1);
    if ((buf_pos->type == REG_INT) || (value->type == REG_INT)) {
        OpticsFoundryFPGA_SetFPGABuffer(buf_pos->value.i, value->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "SetFPGABuffer type mismatch");
    }
}

#define DEFINE_AD9854(name, op) \
void name(uint16_t* args) { \
    Register* buf_pos = REG(0), * value = REG(1);\
    if ((buf_pos->type == REG_INT) || (value->type == REG_INT)) {\
        op(buf_pos->value.i, value->value.i);\
    }\
    else {\
        CS_SetError(CMD_TYPE_MISMATCH, #name " type mismatch");\
    }\
}

DEFINE_AD9854(cmd_setAD9854frequency, OpticsFoundryFPGA_SetAD9858FrequencyAtBufferPosition)
DEFINE_AD9854(cmd_setAD9854amplitude, OpticsFoundryFPGA_SetAD9858AmplitudeAtBufferPosition)
DEFINE_AD9854(cmd_setAD9854phase, OpticsFoundryFPGA_SetAD9858PhaseAtBufferPosition)
DEFINE_AD9854(cmd_setAD9854updateclock, OpticsFoundryFPGA_SetAD9858UpdateClockAtBufferPosition)
DEFINE_AD9854(cmd_setAD9854ramprateclock, OpticsFoundryFPGA_SetAD9858RampRateClockAtBufferPosition)
DEFINE_AD9854(cmd_setAD9854controlword, OpticsFoundryFPGA_SetAD9858ControlWordAtBufferPosition)


extern void cmd_setanalogout(uint16_t* args) {
    Register* buf_pos = REG(0), * value = REG(1);
    if ((buf_pos->type == REG_INT) || (value->type == REG_INT)) {
        OpticsFoundryFPGA_SetAnalogOutAtBufferPosition(buf_pos->value.i, value->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "SetAnalogOut type mismatch");
    }
}

extern void cmd_checkethernetcommand(uint16_t* args) {
    Register* a = REG(0);
    if (a->type == REG_INT) {
        OpticsFoundryFPGA_CheckEthernetCommand(a->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "CheckEthernetCommand type mismatch");
    }
}

extern void cmd_runfastvcoloop(uint16_t* args) {
    Register* buf_start_pos = REG(0), * buf_AD9854_frequency_pos = REG(1);
    if ((buf_start_pos->type == REG_INT) || (buf_AD9854_frequency_pos->type == REG_INT)) {
		OpticsFoundryFPGA_RunFastVCOLoop(buf_start_pos->value.i, buf_AD9854_frequency_pos->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "RunFastVCOLoop type mismatch");
    }
}

extern void cmd_printfpgabuffer(uint16_t* args) {
    Register* print_length = REG(0);
    if ((print_length->type == REG_INT)) {
        OpticsFoundryFPGA_PrintFPGABuffer(print_length->value.i);
    }
    else {
        CS_SetError(CMD_TYPE_MISMATCH, "PrintFPGABuffer type mismatch");
    }
}
