// core/label_table.c
#include "label_table.h"
#include <string.h>
#include <stdio.h>

#define MAX_LABEL_LENGTH 32
#define MAX_LABELS 1024

// Represents a label pointing to a line number
typedef struct {
    char name[MAX_LABEL_LENGTH];
    long line_number;  // -1 if not yet resolved
} LabelEntry;

int label_table_count = 0;
LabelEntry label_table[MAX_LABELS];

void reset_label_table() {
    for (int i = 0; i < label_table_count; ++i) {
		label_table[i].line_number = -2; //-2 indicates not yet used
    }
    label_table_count = 0;
}

bool check_label_table(const char* *bad_label) {
    bool ok = true;
    for (int i = 0; i < label_table_count; ++i) {
        if (label_table[i].line_number == -1) {//-1 indicates a label that was used as a jump destination, but not defined
             *bad_label = label_table[i].name;
            ok = false;
        }
    }
    return ok;
}

int get_max_label_nr() {
    return label_table_count;
}

int find_label_index(const char* label) {
    for (int i = 0; i < label_table_count; ++i) {
        if (strcmp(label_table[i].name, label) == 0) {
            return i;
        }
    }
    return -1;
}

bool add_label(const char* label, int *idx, int line_number) {
    if (label_table_count >= MAX_LABELS) return false;

    *idx = find_label_index(label);
    if (*idx != -1 && label_table[*idx].line_number >= 0) { 
        //add_error("Label already defined: ");
        //add_error(label);
        return false;
    }

    if (*idx == -1) {
        strncpy(label_table[label_table_count].name, label, MAX_LABEL_LENGTH);
        label_table[label_table_count].line_number = line_number;
        *idx = label_table_count;
        label_table_count++;
    }
    else {
        label_table[*idx].line_number = line_number;
    }
    return true;
}

int get_label_line(const char* label) {
    int idx = find_label_index(label);
    if (idx == -1) return -1;
    return label_table[idx].line_number;
}

int get_label_table_line_number(const int index) {
    if (index >= label_table_count) return -1;
    return label_table[index].line_number;
}