#include "command_defs.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "label_table.h"
#include "name_table.h"
#include "registers.h"
#include "command_errors.h"

extern CommandDef command_defs[];
extern const int num_commands;

static int find_command_opcode(const char* token) {
    // Check for numeric opcode
    if (isdigit((unsigned char)token[0])) {
        int val = atoi(token);
        if (val >= 0 && val < num_commands)
            return val;
        return -1;
    }

    // Match by name
    for (int i = 0; i < num_commands; ++i) {
        if (strcmp(token, command_defs[i].name) == 0)
            return i;
    }
    return -1;
}

static const char* skip_whitespace(const char* s) {
    while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t') ++s;
    return s;
}

static const char* parse_token(const char* s, char* out, size_t max_len) {
    s = skip_whitespace(s);
    size_t i = 0;
    while (*s && !isspace((unsigned char)*s) && *s != '(' && *s != ',' && *s != ')' && *s != ':') {
        if (i < max_len - 1) out[i++] = *s;
        ++s;
    }
    if (*s == ':') {
        out[i] = ':';
        i++;
        ++s;
    }
    out[i] = '\0';
    return s;
}

static const char* parse_one_argument(const char* s, char* out, size_t max_len) {
    s = skip_whitespace(s);
    size_t i = 0;
    while (*s && *s != '(' && *s != ',' && *s != ')' && *s != ':') {
        if (i < max_len - 1) out[i++] = *s;
        ++s;
    }
    if (*s == ':') {
        out[i] = ':';
        i++;
        ++s;
    }
    out[i] = '\0';
    return s;
}

static const char* parse_label(const char* s, char* out, size_t max_len) {
    s = skip_whitespace(s);
    size_t i = 0;
    while (*s && !isspace((unsigned char)*s) && *s != ':') {
        if (i < max_len - 1) out[i++] = *s;
        ++s;
    }
    out[i] = '\0';
    s++;
    return s;
}

static const char* parse_argument(const char* s, uint16_t* out_index) {
    s = skip_whitespace(s);
    static int const_index = MAX_USER_REGISTERS;
    *out_index = 0;

    if (s[0] == '"') {
        // String literal
        s++;
        //const char* start = arg;
        char* dst = registers[const_index].value.s;
        size_t len = 0;
        while (s && *s != '"') {
            if (len < MAX_STRING_LENGTH - 1) {
                dst[len++] = *s++;
            }
            else {
                char buf[256];
                dst[len] = '\0'; // truncate to avoid overflow
                snprintf(buf, sizeof(buf), "String constant too long: %s", dst);
                CS_SetError(CMD_STRING_TOO_LONG, buf);
                return NULL;
            }
        }
        dst[len] = '\0';
        if (*s != '"') {
            char buf[256];
            snprintf(buf, sizeof(buf), "Missing closing quote: %s", dst);
            CS_SetError(CMD_PARSE_ERROR, buf);
            return NULL;
        }
        registers[const_index].type = REG_STRING;
        *out_index = const_index++;
        return ++s;
    }


    char arg[64];
    s = parse_one_argument(s, arg, sizeof(arg));

    if ((strncmp(arg, "R", 1) == 0) && (isdigit((unsigned char)arg[1]))) { //register
        //s++;
        //char token[32];
        //parse_token(s, token, 32);
        uint16_t index = (uint16_t)atoi(&arg[1]);
        if (*out_index >= MAX_USER_REGISTERS) {
            CS_SetError(CMD_STRING_TOO_LONG, "User register number too high");
            return NULL;
        }
        *out_index = index;
        //while (isdigit(*s)) ++s;
        return s;
    }
    else if (strchr(arg, '.')) {
        // double
        registers[const_index].type = REG_DOUBLE;
        registers[const_index].value.d = atof(arg);
        *out_index = const_index++;
        //while (*s && (isdigit(*s) || *s == '.' || *s == '-')) ++s;
        return s;
    }
    else if (strchr(arg, ':')) {
        // label
        char label[32];
        parse_label(arg, label, sizeof(label));
        int label_index;
        //bool ok =
        add_label(label, &label_index, -1);
        registers[const_index].type = REG_INT;
        registers[const_index].value.i = label_index;
        *out_index = const_index++;
        //while (*s && (*s == ':')) ++s;
        return s;
    }
    else if ((arg[0] >= 'a' && arg[0] <= 'z') || (arg[0] >= 'A' && arg[0] <= 'Z') || (arg[0] == '_')) { //variable name
        // name
        int name_index = find_name_index(arg);
        if (name_index < 0) {
            if (!add_name(arg, const_index)) {
                CS_SetError(CMD_STRING_TOO_LONG, "couldn't register variable name");
                return NULL;
            }
            name_index = const_index;
            const_index++;
            registers[name_index].type = REG_INT; //type not defined at this moment, so initialize it with INT 0 as default
            registers[name_index].value.i = 0;
        }
        *out_index = name_index;
        return s;
    }
    else {
        // Int
        registers[const_index].type = REG_INT;
        char* endPtr;
        long long value;
        char* arg_ptr = arg;
        if (strncmp(arg, "0b", 2) == 0 || strncmp(arg, "0B", 2) == 0) {
            arg_ptr += 2;
            value = strtoll(arg_ptr, &endPtr, 2);  // skip "0b" and parse as binary
        }
        else {
            value = strtoll(arg_ptr, &endPtr, 0);  // auto-detect hex/octal/decimal
        }
        registers[const_index].value.i = value;
        *out_index = const_index++;
        //while (*s && (isdigit(*s) || *s == '-')) ++s;
        return s;
    }
}

int CS_GetCommandCount(void) { return command_count; }

CommandStatus CS_AddCommand(const char* s) {
    bool end_of_command = false;
    while (!end_of_command && *s) {

        char token[32];
        s = parse_token(s, token, sizeof(token));
        if (strchr(token, ':')) {
            // label
            token[strlen(token) - 1] = '\0';
            int label_index;
            bool ok = add_label(token, &label_index, command_count);
            if (!ok) {
                char buf[200];
                sprintf(buf, "Label already defined: %s", token);
                CS_SetError(CMD_PARSE_ERROR, buf);
                return CMD_PARSE_ERROR;
            }
        }
        else {
            int opcode = find_command_opcode(token);
            if (opcode < 0) {
                CS_SetError(CMD_INVALID_OPCODE, token);
                return CMD_INVALID_OPCODE;
            }

            const CommandDef* def = &command_defs[opcode];

            s = skip_whitespace(s);
            if (*s != '(') return CMD_PARSE_ERROR;
            ++s;

            uint16_t args[MAX_ARGUMENTS] = { 0 };
            for (int i = 0; i < def->arg_count; ++i) {
                s = parse_argument(s, &args[i]);
                if (!s) return CMD_PARSE_ERROR;
                s = skip_whitespace(s);
                if (i < def->arg_count - 1 && *s == ',') ++s;
            }

            if (*s != ')') {
                CS_SetError(CMD_PARSE_ERROR, "Missing ')'");
                return CMD_PARSE_ERROR;
            }
            ++s;
            s = skip_whitespace(s);
            if (*s == ';') ++s;

            if (command_count >= MAX_COMMANDS) {
                CS_SetError(CMD_SEQUENCE_OVERFLOW, "Too many commands");
                return CMD_SEQUENCE_OVERFLOW;
            }

            command_table[command_count].opcode = (uint8_t)opcode;
            memcpy(command_table[command_count].args, args, sizeof(args));
            //command_table[command_count].line_number = add_command_call_count++;

            ++command_count;
        }

        s = skip_whitespace(s);
        if ((*s == '/') || (*s == '\0')) { // command ends in comment, which we skip, or command string ends
            end_of_command = true;
        }
    }
    return CMD_SUCCESS;
}

