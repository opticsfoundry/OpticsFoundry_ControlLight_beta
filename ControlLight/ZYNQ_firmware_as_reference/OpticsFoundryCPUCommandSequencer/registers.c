
#include "registers.h"

#include <string.h>


Register registers[MAX_REGISTERS];

bool set_register_int(int index, int64_t value) {
    if (index < 0 || index >= MAX_REGISTERS) return false;
    registers[index].type = REG_INT;
    registers[index].value.i = value;
    return true;
}

bool set_register_double(int index, double value) {
    if (index < 0 || index >= MAX_REGISTERS) return false;
    registers[index].type = REG_DOUBLE;
    registers[index].value.d = value;
    return true;
}

bool set_register_string(int index, const char* value) {
    if (index < 0 || index >= MAX_REGISTERS) return false;
    registers[index].type = REG_STRING;
    strncpy(registers[index].value.s, value, MAX_STRING_LENGTH);
    registers[index].value.s[MAX_STRING_LENGTH - 1] = '\0';
    return true;
}

bool get_register_as_int(int index, int64_t* out) {
    if (index < 0 || index >= MAX_REGISTERS || out == NULL) return false;
    switch (registers[index].type) {
    case REG_INT:
        *out = registers[index].value.i;
        return true;
    case REG_DOUBLE:
        *out = (int64_t)(registers[index].value.d);
        return true;
    default:
        return false;
    }
}

bool get_register_as_double(int index, double* out) {
    if (index < 0 || index >= MAX_REGISTERS || out == NULL) return false;
    switch (registers[index].type) {
    case REG_INT:
        *out = (double)(registers[index].value.i);
        return true;
    case REG_DOUBLE:
        *out = registers[index].value.d;
        return true;
    default:
        return false;
    }
}

bool get_register_as_string(int index, char* out, int max_len) {
    if (index < 0 || index >= MAX_REGISTERS || out == NULL) return false;
    if (registers[index].type != REG_STRING) return false;
    strncpy(out, registers[index].value.s, max_len);
    out[max_len - 1] = '\0';
    return true;
}

bool convert_register_to_type(int index, RegType target_type) {
    if (index < 0 || index >= MAX_REGISTERS) return false;
    Register* reg = &registers[index];
    if (reg->type == target_type) return true;

    switch (target_type) {
    case REG_INT:
        if (reg->type == REG_DOUBLE) {
            reg->value.i = (int64_t)(reg->value.d);
            reg->type = REG_INT;
            return true;
        }
        return false;

    case REG_DOUBLE:
        if (reg->type == REG_INT) {
            reg->value.d = (double)(reg->value.i);
            reg->type = REG_DOUBLE;
            return true;
        }
        return false;

    case REG_STRING:
    default:
        return false;
    }
}
