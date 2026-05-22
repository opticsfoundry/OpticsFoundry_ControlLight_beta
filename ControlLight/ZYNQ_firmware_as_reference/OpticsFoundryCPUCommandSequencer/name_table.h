#pragma once

#include <stdbool.h>

extern void reset_name_table();
extern bool check_name_table();
//extern int get_max_name_nr();
extern int find_name_index(const char* name);
extern bool add_name(const char* name, int register_number);
//extern int get_name_line(const char* name);
//extern int get_name_table_line_number(const int index);
