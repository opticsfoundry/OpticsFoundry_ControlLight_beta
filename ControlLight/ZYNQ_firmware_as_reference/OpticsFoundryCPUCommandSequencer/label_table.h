#pragma once

#include <stdbool.h>

extern void reset_label_table();
extern bool check_label_table(const char* *bad_label);
extern int get_max_label_nr();
extern int find_label_index(const char* label);
extern bool add_label(const char* label, int *idx, int line_number);
extern int get_label_line(const char* label);
extern int get_label_table_line_number(const int index);
