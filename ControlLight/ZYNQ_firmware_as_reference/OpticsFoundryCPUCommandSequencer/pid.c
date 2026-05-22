// commands/command_pid.c
//#include "registers.h"
//#include "utils/error_handling.h"
//#include "commands.h"
#include "command_defs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "registers.h"
//
//typedef struct {
//    double integral;
//    double last_error;
//} PIDState;

//#define MAX_PID_STATES 16
//static PIDState pid_states[MAX_PID_STATES];

// Converts a string register like "PID0" to an index (0..MAX_PID_STATES-1)
//static int parse_pid_state_index(const char* str) {
//    if (strncmp(str, "PID", 3) != 0) return -1;
//    int idx = atoi(str + 3);
//    if (idx < 0 || idx >= MAX_PID_STATES) return -1;
//    return idx;
//}

//usage
//CS_AddCommand("PID(out, target, measure, p, i, d, last_error, last_integral);"); 

void cmd_pid(uint16_t* args)
{
  //  if (num_args != 8) return add_error("PID: Expected 8 arguments");

    int r_out = args[0];
    int r_target = args[1];
    int r_measure = args[2];
    int r_kp = args[3];
    int r_ki = args[4];
    int r_kd = args[5];
    int r_last_error = args[6];
    int r_integral = args[7];
    
    double target;  
    get_register_as_double(r_target, &target);
    double measure; get_register_as_double(r_measure, &measure);
    double kp;  get_register_as_double(r_kp, &kp);
    double ki; get_register_as_double(r_ki, &ki);
    double kd; get_register_as_double(r_kd,&kd);
    double kerr; get_register_as_double(r_last_error, &kerr);
    double kint; get_register_as_double(r_integral, &kint);
    //int64_t state_index; get_register_as_int(r_state,&state_index);// parse_pid_state_index(state_str);
    //if (state_index < 0) return add_error("PID: Invalid state register name (e.g. PID0)");

//    PIDState* s = &pid_states[state_index];
    double error = target - measure;

    kint += error;
    double derivative = error - kerr;
    kerr = error;

    double output = kp * error + ki * kint + kd * derivative;
    set_register_double(r_out, output);
    set_register_double(r_last_error, error);
    set_register_double(r_integral, kint);
}
