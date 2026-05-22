// core/name_table.c
#include "name_table.h"
#include <string.h>
#include <stdio.h>

#define MAX_NAME_LENGTH 32
#define MAX_NAMES 1024

// Represents a name pointing to a line number
typedef struct {
    char name[MAX_NAME_LENGTH];
    unsigned long register_number;  // -1 if not yet resolved
} nameEntry;

int name_table_count = 0;
nameEntry name_table[MAX_NAMES];

void reset_name_table() {
    for (int i = 0; i < name_table_count; ++i) {
        name_table[i].register_number = -2; //-2 indicates not yet used
    }
    name_table_count = 0;
}

bool check_name_table() {
    bool ok = true;
    for (int i = 0; i < name_table_count; ++i) {
        ok = ok && name_table[i].register_number != -1; //-1 indicates a name that was used as a jump destination, but not defined
    }
    return ok;
}

int get_max_name_nr() {
    return name_table_count;
}

int find_name_index(const char* name) {
    for (int i = 0; i < name_table_count; ++i) {
        if (strcmp(name_table[i].name, name) == 0) {
            return name_table[i].register_number;
        }
    }
    return -1;
}

bool add_name(const char* name, int register_number) {
    if (name_table_count >= MAX_NAMES) return false;

    int idx = find_name_index(name);
    if (idx != -1) {
        //add_error("name already defined: ");
        //add_error(name);
        return false;
    }
    strncpy(name_table[name_table_count].name, name, MAX_NAME_LENGTH);
    name_table[name_table_count].register_number = register_number;
    name_table_count++;
    return true;
}

int get_name_line(const char* name) {
    int idx = find_name_index(name);
    if (idx == -1) return -1;
    return name_table[idx].register_number;
}

int get_name_table_register_number(const int index) {
    if (index >= name_table_count) return -1;
    return name_table[index].register_number;
}