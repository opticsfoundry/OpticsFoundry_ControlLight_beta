
#pragma once

#include <stdint.h>
#include <stdbool.h>


#define MAX_USER_REGISTERS   1024
#define MAX_REGISTERS        2048
#define MAX_STRING_LENGTH    128

typedef enum {
    REG_INT,
    REG_DOUBLE,
    REG_STRING
} RegType;

typedef struct {
    RegType type;
    union {
        int64_t i;
        double d;
        char s[MAX_STRING_LENGTH]; // modifiable string
    } value;
} Register;

extern Register registers[MAX_REGISTERS];

bool set_register_int(int index, int64_t value);
bool set_register_double(int index, double value);
bool set_register_string(int index, const char* value);
bool get_register_as_int(int index, int64_t* out);
bool get_register_as_double(int index, double* out);
bool get_register_as_string(int index, char* out, int max_len);
bool convert_register_to_type(int index, RegType target_type);
