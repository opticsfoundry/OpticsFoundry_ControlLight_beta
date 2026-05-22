`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 24.08.2023 19:40:34
// Design Name: 
// Module Name: core
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


//Enable or disable features
//`define USE_ZYNQ_ADC
`define USE_AD9854_AS_VCO
`define USE_INPUT_EVENT_TAGGER
`define USE_INPUT_EVENT_TAGGER_FIFO_8_ENTRIES

module core(
    command,
    address,
    reset,
    int_out,
    int_clear,
    bus_data,
    address_latched,
    address_extension_latched,
    wait_cycles_latched,
    input_buf_mem_address,
    input_buf_mem_data,
    input_buf_mem_write,
    start,
    pause,
    clock,
    trigger_0,
    trigger_1,
    trigger_PS,
    trigger_out,
    condition_0,
    condition_1,
    condition_PS,
    latch_current_state,
    
    bus_clock_or_strobe,
    I2C_SCL,
    I2C_SDA_OUT,
    I2C_SDA_IN_0,
    I2C_SDA_IN_1,
    I2C_SELECT_0,
    I2C_SELECT_1,
    I2C_0_Destination,
    SPI_IN_0,
    SPI_IN_1,
    SPI_OUT,
    SPI_SCK,
    SPI_SELECT_0,
    SPI_SELECT_1,
    SPI_READY,
    error_detected,
    last_DMA_memory_margin,
    running,
    options,
    clock_cycles_this_run,
    periodic_trigger_count, 
    waiting_for_periodic_trigger, 
    warning_missed_periodic_trigger,
    core_dig_in,
    secondary_PS_PL_interrupt,
    reset_secondary_PS_PL_interrupt,
    input_mem_PS_PL_interrupt,
    reset_input_mem_PS_PL_interrupt,
    adc_enable,
    adc_register_address,
    adc_conversion_start,
    adc_result_in,
    adc_write_enable,
    adc_programming_out,
    adc_ready,
    adc_channel,
    PL_to_PS_command,
    SPI_chip_select,
    periodic_trigger_signal
    );
    
    input [63:0] command;
    output reg [15:0] address = 0;
    input reset;
    output reg int_out;
    input int_clear;
    output reg [27:0] bus_data = 0;
    output reg [15:0] address_latched = 0;
    output reg [12:0] address_extension_latched = 0;
    output reg [47:0] wait_cycles_latched = 0;
    output reg [12:0] input_buf_mem_address = 0;
    output reg [31:0] input_buf_mem_data = 0;
    reg [31:0] input_buf_data_high_32_bit = 0;
    output reg input_buf_mem_write = 0;
    output reg [47:0] clock_cycles_this_run = 0;
    input start;
    input pause;
    input clock;
    input trigger_0;
    input trigger_1;
    input trigger_PS;
    output reg trigger_out = 0;
    input condition_0;
    input condition_1;
    input condition_PS;
    input latch_current_state;
    input SPI_IN_0;
    input SPI_IN_1;
    input SPI_READY;
    input I2C_SDA_IN_0;
    input I2C_SDA_IN_1;
    output reg SPI_OUT = 0;
    output reg bus_clock_or_strobe = 0;
    output reg I2C_SCL = 1;
    output reg I2C_SDA_OUT = 1;
    output reg I2C_SELECT_0 = 0;
    output reg I2C_SELECT_1 = 0;
    output reg I2C_0_Destination = 0;
    output reg SPI_SCK = 0;
    output reg SPI_SELECT_0;
    output reg SPI_SELECT_1;
    output reg error_detected;
    output reg [15:0] last_DMA_memory_margin;
    output reg [47:0] periodic_trigger_count;
    output reg waiting_for_periodic_trigger;
    output reg warning_missed_periodic_trigger;
    output reg running = 0;
    output reg [31:0] options = 0; 
    input reg [7:0] core_dig_in;
    output reg secondary_PS_PL_interrupt = 0;
    input reset_secondary_PS_PL_interrupt;
    output reg input_mem_PS_PL_interrupt = 0;
    input reset_input_mem_PS_PL_interrupt;
    output reg adc_enable = 0;
    output reg adc_conversion_start = 0;
    input [15:0] adc_result_in;
    output reg [6:0] adc_register_address = 0;
    output reg adc_write_enable = 0;
    output reg [15:0] adc_programming_out = 0;
    input adc_ready;
    input [4:0] adc_channel;
    output  reg [15:0] PL_to_PS_command = 0;
    output reg [3:0] SPI_chip_select = 8; 
    // SPI_chip_select[2:0] go to 3:8 decoder and select CS0. The decoder output is enabled if SPI_chip_select[3] is low, otherwise all decoder outputs are high.
    // If only one CS is needed, i.e. no decoder is used, SPI_chip_select[3] can be used as sole CS line.


reg synchronous_reset = 0;
reg synchronous_reset_secondary_PS_PL_interrupt = 0;
reg synchronous_reset_input_mem_PS_PL_interrupt = 0;

reg int_clear_happened = 0 ;
reg reset_input_mem_PS_PL_interrupt_happened = 0;
reg [31:0] input_buf_mem_data_SPI = 0;
reg [12:0] address_extension = 0;
reg [47:0] wait_cycles = 0;
reg bus_clock = 0;
reg bus_strobe = 0;
reg bus_strobe_first_part = 0;
reg bus_strobe_second_part = 1;
reg bus_data15_used_as_strobe = 0;
reg bus_data15_second_part = 0;
reg bus_strobe_idle_part = 0;
reg bus_data15_idle_part = 0;
reg [3:0] strobe_choice = 0; 
reg [7:0] strobe_delay = 0; 
reg [7:0] strobe_first_part_length = 0; 
reg [7:0] strobe_second_part_length = 0; 
reg [117:0] register = 0;
reg [63:0] command_buffer = 0;
reg [63:0] extended_command = 0;
reg [32:0] loop_count = 0;

`ifdef USE_AD9854_AS_VCO
reg [47:0] AD9854FTWIntermediate = 0;
reg [47:0] AD9854FTWShifted = 0;
reg [47:0] AD9854FTW = 0;
reg [47:0] AD9854FTWbyteshifted = 0;
reg send_AD9854_ftw = 0;
reg AD9854_ftw_byte_shifts_to_do = 0;
`endif

`ifdef USE_INPUT_EVENT_TAGGER
reg [7:0] input_event_tagger_last_input = 0;
reg [0:0] input_event_tagger_mark_counter_overflow = 0;
reg [0:0] input_event_tagger_fifo_overflow = 0;

`ifdef USE_INPUT_EVENT_TAGGER_FIFO_8_ENTRIES
reg [30:0] input_event_tagger_fifo [0:7];
reg [2:0] input_event_tagger_fifo_write_ptr = 0;
reg [2:0] input_event_tagger_fifo_read_ptr = 0;
reg [3:0] input_event_tagger_fifo_count = 0;
`else
//only 4 entries in FIFO
reg [30:0] input_event_tagger_fifo [0:3];
reg [1:0] input_event_tagger_fifo_write_ptr = 0;
reg [1:0] input_event_tagger_fifo_read_ptr = 0;
reg [2:0] input_event_tagger_fifo_count = 0;
`endif

`endif

reg [47:0] periodic_trigger_count_internal = 0;
reg [47:0] periodic_trigger_period = 0;  //FS; don't set this to zero when receiving a reset signal. Only program it through CMD_SET_PERIODIC_TRIGGER_PERIOD command
reg [47:0] periodic_trigger_allowed_wait_cycles = 0;
reg [47:0] periodic_trigger_wait_cycles = 0;
reg [55:0] cycle_count_since_startup = 0;
//only for debug
output reg periodic_trigger_signal = 0;
reg [15:0] ANA_OUT_VALUE = 0;
reg [15:0] ana_in_data = 0;
reg [4:0] ana_in_channel = 0;



always @(*)
begin
    case (strobe_choice)
    0: begin
        bus_clock_or_strobe <= bus_clock;
    end
    1: begin
       bus_clock_or_strobe <= bus_strobe;
    end
    2: begin
       bus_clock_or_strobe <= 0;
    end
    3: begin
       bus_clock_or_strobe <= 1;
    end
    4: begin
       bus_clock_or_strobe <= bus_data[27];
    end
    default: begin
        bus_clock_or_strobe <= bus_clock;
    end
    endcase
end         


enum logic [2:0] {PERIODIC_TRIGGER_IDLE,
                PERIODIC_TRIGGER_START,
                PERIODIC_TRIGGER_WAIT1,    
                PERIODIC_TRIGGER_WAIT2,    
                PERIODIC_TRIGGER_WAIT3,    
                PERIODIC_TRIGGER_WAIT4,    
                PERIODIC_TRIGGER_WAIT5,    
                PERIODIC_TRIGGER_STOP
                } PERIODIC_TRIGGER_state = PERIODIC_TRIGGER_IDLE;



enum logic [4:0] {I2C_IDLE,
            I2C_START,
            I2C_START_SCL_LOW,
            I2C_DATA_LOAD,
            I2C_CLOCK_HIGH,
            I2C_CLOCK_LOW,
            I2C_WRITE_ACK_SETUP,
            I2C_WRITE_ACK_CLOCK_HIGH,
            I2C_WRITE_ACK_CLOCK_LOW,
            I2C_PAUSE_BEFORE_READ,
            I2C_RESTART_SCL_HIGH,
            I2C_RESTART_SDA_LOW,
            I2C_READ_SETUP,
            I2C_READ_CLOCK_HIGH,
            I2C_READ_CLOCK_LOW,
            I2C_READ_ACK_SETUP,
            I2C_READ_ACK_CLOCK_HIGH,
            I2C_READ_ACK_CLOCK_LOW,
            I2C_WRITE_TO_INPUT_MEMORY,
            I2C_STOP,
            I2C_STOP_SCL_HIGH,
            I2C_STOP_SDA_RELEASE } I2C_state = I2C_IDLE;
 
reg [7:0] I2C_delay = 0;
reg [7:0] I2C_bit = 0;
reg [117:0] I2C_data = 0;
reg [7:0] I2C_out_length = 0;
reg [5:0] I2C_in_length = 0;
reg [1:0] I2C_SELECT_NEXT = 0;
reg [31:0] input_buf_mem_data_I2C = 0;
reg [5:0] I2C_read_bit = 0;
reg I2C_TX_READ_ADDRESS = 0;
reg [7:0] I2C_delay_start_stop = 60;
reg [7:0] I2C_delay_data_setup = 40;
reg [7:0] I2C_delay_clock_high = 60;
reg [7:0] I2C_delay_clock_low = 150;
reg [7:0] I2C_delay_pause_before_read = 0;

enum logic [4:0] {SPI_IDLE, //0
            SPI_START,//1
            SPI_STATE_WAIT_FOR_READY_INACTIVE, //2
            SPI_STATE_WAIT_FOR_READY_ACTIVE, //3
            SPI_DATA_OUT_DRIVE_PHASE,  //4
            SPI_DATA_OUT_SAMPLE_PHASE,  //5
            SPI_PAUSE_BEFORE_READ,  //6
            SPI_DATA_IN_PRE_SAMPLE,  //7
            SPI_DATA_IN_DRIVE_PHASE_WAIT,  //8
            SPI_DATA_IN_SAMPLE_PHASE_WAIT,  //9
            SPI_WRITE_TO_INPUT_MEMORY,  //10
            SPI_FINAL_PAUSE, //11
            SPI_STOP //12
            } SPI_state = SPI_IDLE;

reg SPI_OUT_ENDED = 0;
reg [11:0] SPI_delay = 0;
reg [7:0] SPI_OUT_bit_nr = 0;
reg [6:0] SPI_IN_bit_nr = 0;
reg [117:0] SPI_data = 0;
reg SPI_READY_ACTIVE = 0;
reg SPI_WAIT_FOR_READY_EDGE_TO_ACTIVE = 0;
reg SPI_WAIT_FOR_READY_ACTIVE = 0;
reg SPI_RESTART_INPUT_REPEAT_WAIT_ON_READY_ACTIVE = 0;
reg INPUT_REPEAT_set_to_delay_zero_flag = 0;
reg SPI_READY_meta = 0;
reg SPI_READY_sync = 0;
(* ASYNC_REG = "TRUE" *) reg condition_0_meta = 0;
(* ASYNC_REG = "TRUE" *) reg condition_0_sync = 0;
(* ASYNC_REG = "TRUE" *) reg condition_1_meta = 0;
(* ASYNC_REG = "TRUE" *) reg condition_1_sync = 0;
(* ASYNC_REG = "TRUE" *) reg condition_PS_meta = 0;
(* ASYNC_REG = "TRUE" *) reg condition_PS_sync = 0;
(* ASYNC_REG = "TRUE" *) reg trigger_0_meta = 0;
(* ASYNC_REG = "TRUE" *) reg trigger_0_sync = 0;
(* ASYNC_REG = "TRUE" *) reg trigger_1_meta = 0;
(* ASYNC_REG = "TRUE" *) reg trigger_1_sync = 0;
(* ASYNC_REG = "TRUE" *) reg trigger_PS_meta = 0;
(* ASYNC_REG = "TRUE" *) reg trigger_PS_sync = 0;
(* ASYNC_REG = "TRUE" *) reg [7:0] core_dig_in_meta = 0;
(* ASYNC_REG = "TRUE" *) reg [7:0] core_dig_in_sync = 0;
reg SPI_CPOL = 0;
reg SPI_CPHA = 0;
// CPHA=0: sample on leading phase, change on trailing phase.
// CPHA=1: change on leading phase, sample on trailing phase.
wire SPI_SCK_idle_phase = SPI_CPOL;
wire SPI_SCK_leading_phase = ~SPI_CPOL;
wire SPI_SCK_trailing_phase = SPI_CPOL;
wire SPI_SCK_drive_phase = SPI_CPHA ? SPI_SCK_leading_phase : SPI_SCK_trailing_phase;
wire SPI_SCK_sample_phase = SPI_CPHA ? SPI_SCK_trailing_phase : SPI_SCK_leading_phase;
wire SPI_READY_INACTIVE = ~SPI_READY_ACTIVE;
reg [6:0] SPI_OUT_length = 0;
reg [5:0] SPI_IN_length = 0;
reg [1:0] SPI_SEL_next = 1;

reg [9:0] SPI_delay_CS_low_start_wait = 4;
reg [9:0] SPI_delay_write = 4;
reg [11:0] SPI_delay_pause_before_read = 4;
reg [9:0] SPI_delay_read = 23;
reg [9:0] SPI_delay_CS_low_end_wait = 4; 


enum logic [1:0] {ANA_IN_IDLE,
            ANA_IN_START,
            ANA_IN_TRIGGER_READ_WRITE,
            ANA_IN_INCREASE_INPUT_MEM_ADDRESS} ANA_IN_state = ANA_IN_IDLE;

reg [7:0] ANA_IN_delay = 0;
reg ana_in_data_expected = 0;

enum logic [2:0] {INPUT_MEM_IDLE,
                  INPUT_MEM_WRITE_64BIT,
                  INPUT_MEM_WRITE_2_64BIT,
                  INPUT_MEM_WRITE_3_64BIT,
                  INPUT_MEM_WRITE_4_64BIT,
                  INPUT_MEM_WRITE,
                  INPUT_MEM_WRITE_2,
                  INPUT_MEM_STOP} INPUT_MEM_state = INPUT_MEM_IDLE;


//every time the adc presents a new conversion result or a new register read output, store that data in our input buffer
//without the "posedge clock" the flipflops of ana_in_data_reg and ana_in_channel_reg don't get a clock input.
//always @(posedge clock, posedge adc_ready)
//begin
//    if (adc_ready) begin
 //       ana_in_data <= adc_result_in;
  //      ana_in_channel <= adc_channel;
  //  end
//end

//code that creates trouble:
//without the "posedge clock" the flipflops of ana_in_data_reg and ana_in_channel_reg don't get a clock input.

//always @(posedge adc_ready)
//begin
//    if (ana_in_data_expected) begin
//        ana_in_data <= adc_result_in;
//        ana_in_channel <= adc_channel;
//    end
//end


enum logic [0:0] {DIG_IN_IDLE,
            DIG_IN_START} DIG_IN_state = DIG_IN_IDLE;

enum logic [2:0] {INPUT_REPEAT_IDLE,//0
            INPUT_REPEAT_START,//1
            INPUT_REPEAT_STARTED,//2
            INPUT_REPEAT_WAIT,//3
            INPUT_REPEAT_DIG_EVENT//4
            } INPUT_REPEAT_state = INPUT_REPEAT_IDLE;

reg [23:0] INPUT_REPEAT_wait = 0;
reg [24:0] INPUT_REPEAT_delay = 0;
reg [19:0] INPUT_REPEAT_repeats = 0;
reg [20:0] INPUT_REPEAT_nr = 0;
reg INPUT_REPEAT_trigger_secondary_interrupt_when_finished = 0;

typedef enum logic [2:0] {  IN_REP_CMD_IDLE, //0
                            IN_REP_CMD_SPI, //1
                            IN_REP_CMD_DIG_IN, //2
                            IN_REP_CMD_DIG_EVENT, //3
                            IN_REP_CMD_ANA_IN //4
                            } input_repeat_command_t;

input_repeat_command_t INPUT_REPEAT_command = IN_REP_CMD_IDLE;

enum logic [1:0] {  STROBE_GEN_IDLE,
                    DELAY_CYCLE,
                    PULSE_CYCLE } strobe_generator_state = STROBE_GEN_IDLE;


`ifdef USE_AD9854_AS_VCO
enum logic [1:0] {  CALC_AD9854_FTW_IDLE,
                    CALC_AD9854_FTW_START,
                    CALC_AD9854_FTW_STOP } calc_ad9854_ftw_state = CALC_AD9854_FTW_IDLE;

enum logic [1:0] {  AD9854_BYTE_SHIFT_IDLE,
                    AD9854_BYTE_SHIFT_STEP_1,
                    AD9854_BYTE_SHIFT_STEP_2 } ad9854_ftw_byte_shift_state = AD9854_BYTE_SHIFT_IDLE;
`endif

//commands are contained in bits [4:0], i.e. up to 31 commands
//bits [63:5] are used for command parameters
typedef enum logic [4:0] {  CMD_STOP, //0
                            CMD_STEP, //1
                            CMD_STEP_AND_ENTER_FAST_MODE, //2
                            CMD_SET_OPTIONS, //3
                            CMD_LOAD_REG_LOW, //4
                            CMD_LOAD_REG_HIGH, //5
                            CMD_LATCH_STATE, //6
                            CMD_RESET_WAIT_CYCLES, //7
                            CMD_LONG_WAIT, //8
                            CMD_SET_STROBE_OPTIONS, //9
                            CMD_SET_INPUT_BUF_MEM, //10
                            CMD_WAIT_FOR_TRIGGER, //11
                            CMD_SET_LOOP_COUNT, //12
                            CMD_CONDITIONAL_JUMP_FORWARD, //13
                            CMD_CONDITIONAL_JUMP_BACKWARD, //14
                            CMD_I2C_OUT, //15
                            CMD_SPI_OUT_IN, //16
                            CMD_INPUT_REPEATED_OUT_IN, //17
                            CMD_SET_PERIODIC_TRIGGER_PERIOD, //18
                            CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME, //19
                            CMD_WAIT_FOR_PERIODIC_TRIGGER, //20
                            CMD_WAIT_FOR_WAIT_CYCLE_NR, //21
                            CMD_DIG_IN, //22
                            CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT, //23
                            CMD_ANALOG_IN_OUT, //24
                            CMD_PL_TO_PS_COMMAND, //25
                            CMD_LOAD_COMMAND_BUFFER, //26
                            CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM, //27
                            CMD_CALC_AD9854_FREQUENCY_TUNING_WORD, //28
                            CMD_LOAD_EXTENDED_COMMAND, //29
                            CMD_STEP_SPI //30
                             } type_command; //up to 31 commands

//extended commands contain CMD_LOAD_EXTENDED_COMMAND in bits [4:0] and
//the extended command itself in bits [10:5] (i.e. up to 63 extended commands)
//the remaining 52 bits are available for command parameters
typedef enum logic [5:0] {  EXTENDED_CMD_STOP, //0
                            EXTENDED_CMD_LOAD_SPI_TIMING, //1
                            EXTENDED_CMD_SET_SPI_MODE, //2
                            EXTENDED_CMD_SET_I2C_PARAMETERS //3
                             } type_extended_command; //up to 63 extended commands
    
reg [48:0] wait_time;
reg [48:0] waited;
reg start_high = 0;
reg fast_mode = 0;
reg fast_mode_step = 0;

reg extended_command_active = 0;
wire [4:0] current_command = command[4:0]; 
wire [5:0] current_extended_command = extended_command[10:5]; 


//helper names for 
//CMD_CONDITIONAL_JUMP_FORWARD
//and
//CMD_CONDITIONAL_JUMP_BACKWARD
wire dig_in_jump_enabled;
wire [2:0] dig_in_jump_index;
wire selected_core_dig_in;
wire [7:0] jump_offset;

assign dig_in_jump_enabled = command_buffer[8];
assign dig_in_jump_index = command_buffer[7:5];
assign selected_core_dig_in = core_dig_in_sync[dig_in_jump_index];
assign jump_offset = command_buffer[39:32];
                            
always @(posedge clock)
begin
   if ((latch_current_state) || (current_command == CMD_LATCH_STATE)) begin
        address_latched <= address;
        address_extension_latched <= address_extension; 
        wait_cycles_latched <= wait_cycles; 
   end else begin
    
    end
end

always @(posedge clock, posedge synchronous_reset)
begin
  if (synchronous_reset) begin
     wait_cycles <= 0;
  end else begin
      case (current_command)
        CMD_RESET_WAIT_CYCLES : begin
            wait_cycles <= 0;
        end
       default: wait_cycles <= wait_cycles +1;
    endcase
  end
end

/*
//create a reset signal that's synchronous with the clock
reg [2:0] reset_counter = 0;
always @(posedge clock)
begin
  if (reset) begin
     synchronous_reset <= 1;
     reset_counter <= 3;
  end 
  if (reset_counter > 0) reset_counter <= reset_counter -1;
  if (reset_counter == 0) synchronous_reset <= 0;
end

//create a reset signal that's synchronous with the clock
reg [2:0] reset_secondary_PS_PL_interrupt_counter = 0;
always @(posedge clock)
begin
  if (reset_secondary_PS_PL_interrupt) begin
     synchronous_reset_secondary_PS_PL_interrupt <= 1;
     reset_secondary_PS_PL_interrupt_counter <= 3;
  end 
  if (reset_secondary_PS_PL_interrupt_counter > 0) reset_secondary_PS_PL_interrupt_counter <= reset_secondary_PS_PL_interrupt_counter -1;
  if (reset_secondary_PS_PL_interrupt_counter == 0) synchronous_reset_secondary_PS_PL_interrupt <= 0;
end

//create a reset signal that's synchronous with the clock
reg [2:0] reset_input_mem_PS_PL_interrupt_counter = 0;
always @(posedge clock)
begin
  if (reset_input_mem_PS_PL_interrupt) begin
     synchronous_reset_input_mem_PS_PL_interrupt <= 1;
     reset_input_mem_PS_PL_interrupt_counter <= 3;
  end 
  if (reset_input_mem_PS_PL_interrupt_counter > 0) reset_input_mem_PS_PL_interrupt_counter <= reset_input_mem_PS_PL_interrupt_counter -1;
  if (reset_input_mem_PS_PL_interrupt_counter == 0) synchronous_reset_input_mem_PS_PL_interrupt <= 0;
end
*/


always @(posedge clock, posedge reset)
begin
  if (reset) begin
     synchronous_reset <= 1;
  end else begin
      synchronous_reset <= 0;
  end
end


//synchronize external, asynchronous SPI_READY signal to clock
always @(posedge clock, posedge reset)
begin
  if (reset) begin
      SPI_READY_meta <= 0;
      SPI_READY_sync <= 0;
  end else begin
      SPI_READY_meta <= SPI_READY;
      SPI_READY_sync <= SPI_READY_meta;
  end
end

//synchronize external, asynchronous condition, trigger and digital input signals to clock
always @(posedge clock, posedge reset)
begin
  if (reset) begin
      condition_0_meta <= 0;
      condition_0_sync <= 0;
      condition_1_meta <= 0;
      condition_1_sync <= 0;
      condition_PS_meta <= 0;
      condition_PS_sync <= 0;
      trigger_0_meta <= 0;
      trigger_0_sync <= 0;
      trigger_1_meta <= 0;
      trigger_1_sync <= 0;
      trigger_PS_meta <= 0;
      trigger_PS_sync <= 0;
      core_dig_in_meta <= 0;
      core_dig_in_sync <= 0;
  end else begin
      condition_0_meta <= condition_0;
      condition_0_sync <= condition_0_meta;
      condition_1_meta <= condition_1;
      condition_1_sync <= condition_1_meta;
      condition_PS_meta <= condition_PS;
      condition_PS_sync <= condition_PS_meta;
      trigger_0_meta <= trigger_0;
      trigger_0_sync <= trigger_0_meta;
      trigger_1_meta <= trigger_1;
      trigger_1_sync <= trigger_1_meta;
      trigger_PS_meta <= trigger_PS;
      trigger_PS_sync <= trigger_PS_meta;
      core_dig_in_meta <= core_dig_in;
      core_dig_in_sync <= core_dig_in_meta;
  end
end

always @(posedge clock, posedge reset_secondary_PS_PL_interrupt)
begin
  if (reset_secondary_PS_PL_interrupt) begin
     synchronous_reset_secondary_PS_PL_interrupt <= 1;
  end else begin
      synchronous_reset_secondary_PS_PL_interrupt <= 0;
  end
end

always @(posedge clock, posedge reset_input_mem_PS_PL_interrupt)
begin
  if (reset_input_mem_PS_PL_interrupt) begin
     synchronous_reset_input_mem_PS_PL_interrupt <= 1;
  end else begin
      synchronous_reset_input_mem_PS_PL_interrupt <= 0;
  end
end




always @(posedge clock)
begin
    if (int_clear && (!synchronous_reset)) begin //2024 08 07 strange: was "if (int_clear && (!int_clear_happened)) begin"
        int_clear_happened <= 1;
    end 
    if (synchronous_reset_secondary_PS_PL_interrupt) begin
        secondary_PS_PL_interrupt <= 0;
    end 
    if (synchronous_reset_input_mem_PS_PL_interrupt && (!synchronous_reset)) begin //2024 08 07 strange, was "if (synchronous_reset_input_mem_PS_PL_interrupt && (!reset_input_mem_PS_PL_interrupt_happened)) begin"
        reset_input_mem_PS_PL_interrupt_happened <= 1;    
    end 
    
    cycle_count_since_startup <= cycle_count_since_startup + 1;
    
    //if (current_command != CMD_SET_PERIODIC_TRIGGER_PERIOD) begin
    if (periodic_trigger_count_internal < periodic_trigger_period) begin
        periodic_trigger_count_internal <= periodic_trigger_count_internal +1;
        periodic_trigger_count <= periodic_trigger_count_internal; //pipelining needed to meet timing constraints
    end else begin   
        periodic_trigger_count_internal <= 0;
        periodic_trigger_count <= 0;
        PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_START;
    end
    //end else periodic_trigger_count_internal <= 0;
    if (current_command != CMD_WAIT_FOR_PERIODIC_TRIGGER) begin
        waiting_for_periodic_trigger <= 0;
        periodic_trigger_wait_cycles <= 0;            
    end else begin //2024 08 07 "else" was missing here
        waiting_for_periodic_trigger <= 1;
        periodic_trigger_wait_cycles <= periodic_trigger_wait_cycles + 1;
    end
    
    case (PERIODIC_TRIGGER_state)
        PERIODIC_TRIGGER_IDLE: begin
            periodic_trigger_signal <= 0;  
        end
        PERIODIC_TRIGGER_START: begin
            periodic_trigger_signal <= 1;  
            PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_WAIT1; 
        end
        PERIODIC_TRIGGER_WAIT1: begin    
            PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_WAIT2;
        end
        PERIODIC_TRIGGER_WAIT2: begin    
            PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_WAIT3;
        end
        PERIODIC_TRIGGER_WAIT3: begin    
            PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_WAIT4;
        end
        PERIODIC_TRIGGER_WAIT4: begin    
            PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_WAIT5;
        end
        PERIODIC_TRIGGER_WAIT5: begin    
            PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_STOP;
        end
        PERIODIC_TRIGGER_STOP: begin    
            PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_IDLE;
        end
        default: PERIODIC_TRIGGER_state <= PERIODIC_TRIGGER_IDLE;
    endcase
   
    
    if (synchronous_reset) begin
        start_high <= 0;
        address <= 0;
        wait_time <= 1;
        waited <= 0;
        running <= 0;
        strobe_generator_state <= STROBE_GEN_IDLE;
        int_out <= 0;
        int_clear_happened <= 0;
        input_mem_PS_PL_interrupt <= 0;
        secondary_PS_PL_interrupt <= 0;
        reset_input_mem_PS_PL_interrupt_happened <= 0;
        address_extension <= 0;
        input_buf_mem_address <= 0;
        input_buf_mem_data <= 0;
        input_buf_data_high_32_bit <= 0;
        options <= 0;
        strobe_choice <= 0; 
        strobe_delay <= 0; 
        strobe_first_part_length <= 0;
        strobe_second_part_length <= 0; 
        bus_clock <= 0;
        bus_strobe <= 0;
        bus_strobe_first_part <= 0;
        bus_strobe_second_part <= 1;
        bus_data15_used_as_strobe <= 0;
        bus_data15_second_part <= 0;
        bus_strobe_idle_part <= 0;
        bus_data15_idle_part <= 0;
        clock_cycles_this_run <= 0;
        loop_count <= 0;
        fast_mode <= 0;
        fast_mode_step <= 0;
        I2C_delay <= 0;
        I2C_bit <= 0;
        I2C_read_bit <= 0;
        input_buf_mem_data_I2C <= 0;
        I2C_TX_READ_ADDRESS <= 0;
        strobe_generator_state <= STROBE_GEN_IDLE;
        I2C_state <= I2C_IDLE;
        error_detected <= 0;
        INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;   
        INPUT_REPEAT_command <= IN_REP_CMD_IDLE;                   
        SPI_state <= SPI_IDLE;
        DIG_IN_state <= DIG_IN_IDLE;
        register <= 0;
        trigger_out <= 0;
        adc_conversion_start <= 0;
        ANA_IN_state <= ANA_IN_IDLE;
        adc_write_enable <= 0;
        PL_to_PS_command <= 0;
        SPI_chip_select <= 8;
        SPI_delay_CS_low_start_wait <= 4;
        SPI_delay_write <= 4;
        SPI_delay_pause_before_read <= 4;
        SPI_delay_read <= 23;
        SPI_delay_CS_low_end_wait <= 4;
        extended_command <= 0;
        extended_command_active <= 0;
        I2C_SCL <= 1;
        I2C_SDA_OUT <= 1;
        I2C_SELECT_0 <= 0;
        I2C_SELECT_1 <= 0;
        I2C_0_Destination <= 0;
        I2C_delay_start_stop <= 60;
        I2C_delay_data_setup <= 40;
        I2C_delay_clock_high <= 60;
        I2C_delay_clock_low <= 150;
        I2C_delay_pause_before_read <= 0;
        INPUT_REPEAT_set_to_delay_zero_flag <= 0;
        SPI_CPOL <= 0;
        SPI_CPHA <= 0;
`ifdef USE_AD9854_AS_VCO
        calc_ad9854_ftw_state <= CALC_AD9854_FTW_IDLE;
        send_AD9854_ftw <= 0;
        AD9854FTW <= 0;
        ad9854_ftw_byte_shift_state <= AD9854_BYTE_SHIFT_IDLE;
`endif
`ifdef USE_INPUT_EVENT_TAGGER
        input_event_tagger_last_input <= 0;
        input_event_tagger_mark_counter_overflow <= 0;
        input_event_tagger_fifo_write_ptr <= 0;
        input_event_tagger_fifo_read_ptr <= 0;
        input_event_tagger_fifo_count <= 0;
        input_event_tagger_fifo_overflow <= 0;
        input_event_tagger_fifo[0] <= 0;
        input_event_tagger_fifo[1] <= 0;
        input_event_tagger_fifo[2] <= 0;
        input_event_tagger_fifo[3] <= 0;

`ifdef USE_INPUT_EVENT_TAGGER_FIFO_8_ENTRIES        
        input_event_tagger_fifo[4] <= 0;
        input_event_tagger_fifo[5] <= 0;
        input_event_tagger_fifo[6] <= 0;
        input_event_tagger_fifo[7] <= 0;
`endif

`endif
    end else begin
        if (start) begin  //we really start when "start" signal goes low again
            start_high <= 1;
            running <= 0;
            clock_cycles_this_run <= 0;
        end 
        if (start_high) begin  //can only happen if start has been 1 and now is again 0
            start_high <= 0;
            address <= 0;
            wait_time <= 1;
            waited <= 0;
            running <= 1;  //in next clock cycle we really start
            trigger_out <= 1;
        end 
        if (running) begin
            clock_cycles_this_run <= clock_cycles_this_run + 1;
          
            if (waited < wait_time) begin
                waited <= waited +1;
            end else begin       
              
                if (address[14:0] == 1) begin  //load next data block by DMA
                    if (!int_clear_happened) int_out <= 1;
                    else int_out <= 0;
                end else if (address[14:0] > 1) begin  //load next data block by DMA
                    if (int_clear_happened) begin 
                        int_out <= 0;
                        int_clear_happened <= 0;
                    end 
                end 
            
                if (address == 'hFFFF) begin
                    address_extension <= address_extension + 1;
                end 
                
                if (input_buf_mem_address[11:0] == 1) begin
                    if (!reset_input_mem_PS_PL_interrupt_happened) input_mem_PS_PL_interrupt <= 1;
                    else input_mem_PS_PL_interrupt <= 0;    
                end else if (input_buf_mem_address[11:0] > 1 ) begin
                    if (reset_input_mem_PS_PL_interrupt_happened) begin
                        input_mem_PS_PL_interrupt <= 0;    
                        reset_input_mem_PS_PL_interrupt_happened <= 0;
                    end 
                end 
                
                
                waited <= 0;
                
                
                //Fast Mode start
                if (fast_mode == 1) begin   
                    // in fast mode there are two consecutive bus commands stored in one 64bit command at the expense of only 24 bit long data and 8 bit long wait times. 
                    // if the wait time is 255 then the command will not be executed and from the next command on the code will be interpreted normally again.
                    // [63:40] 24bit data_1 | [39:32] 8bit wait_1 | [31:8] 24bit data_0 | [7:0] 8bit wait_0
                    // make sure to never jump into a fast mode command using CMD_CONDITIONAL_JUMP_FORWARD or CMD_CONDITIONAL_JUMP_BACKWARD
                    if (fast_mode_step == 0) begin
                        if (command[7:0] == 255) begin
                            wait_time[47:0] <= 0;
                            fast_mode <= 0;
                            address <= address + 1;
                        end else begin
                            wait_time[7:0] <= command[7:0];
                            wait_time[47:8] <= 0;
                            bus_data[23:0] <= command[31:8];     
                            fast_mode_step <= 1;
                        end
                    end else begin
                        if (command[39:32] == 255) begin
                            wait_time[47:0] <= 0;
                            fast_mode <= 0;
                            address <= address + 1;
                        end else begin
                            wait_time[7:0] <= command[39:32];
                            wait_time[47:8] <= 0;
                            bus_data[23:0] <= command[63:40];     
                            fast_mode_step <= 0;
                            fast_mode <= command[0:0];
                            address <= address + 1;
                        end
                    end
                    if (bus_clock) bus_clock <= 0; else bus_clock <= 1;     
                    bus_data15_used_as_strobe <= 0;              
                    strobe_generator_state <= DELAY_CYCLE;                  
                end else 
                
                begin
                    if (extended_command_active == 1) begin
                       case (current_extended_command)
                            EXTENDED_CMD_LOAD_SPI_TIMING: begin
                                extended_command[10:5] <= 0;
                                SPI_delay_CS_low_start_wait[9:0] <= extended_command[20:11];
                                SPI_delay_write[9:0] <= extended_command[30:21];
                                SPI_delay_pause_before_read[11:0] <= extended_command[42:31];
                                SPI_delay_read[9:0] <= extended_command[52:43];
                                SPI_delay_CS_low_end_wait[9:0] <= extended_command[62:53];
                                extended_command_active <= 0;
                            end
                            EXTENDED_CMD_SET_SPI_MODE: begin
                                extended_command[10:5] <= 0;
                                SPI_CPHA <= extended_command[11:11];
                                SPI_CPOL  <= extended_command[12:12];
                                extended_command_active <= 0;
                            end
                            EXTENDED_CMD_SET_I2C_PARAMETERS: begin
                                extended_command[10:5] <= 0;
                                I2C_0_Destination <= extended_command[16:16];
                                I2C_delay_start_stop <= (extended_command[24:17] == 0) ? 60 : extended_command[24:17];
                                I2C_delay_data_setup <= (extended_command[32:25] == 0) ? 40 : extended_command[32:25];
                                I2C_delay_clock_high <= (extended_command[40:33] == 0) ? 60 : extended_command[40:33];
                                I2C_delay_clock_low <= (extended_command[48:41] == 0) ? 150 : extended_command[48:41];
                                I2C_delay_pause_before_read <= extended_command[56:49];
                                extended_command_active <= 0;
                            end
                            default: extended_command_active <= 0;
                       endcase
                   end
                   case (current_command)
                        CMD_STOP:begin
                            running <= 0;
                            trigger_out <= 0;
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_SET_OPTIONS:begin
                            options <= command_buffer[63:32];
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_LOAD_COMMAND_BUFFER:begin
                            command_buffer <= command[63:0];
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_LOAD_EXTENDED_COMMAND:begin
                            extended_command <= command[63:0];
                            extended_command_active <= 1;
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_STEP:begin
                            wait_time[30:0] <= command[35:5];
                            wait_time[47:31] <= 0;
`ifdef USE_AD9854_AS_VCO
                            if (send_AD9854_ftw) begin
                                bus_data[7:0] <= command[43:36];
                                bus_data[15:8] <= AD9854FTW[7:0];
                                bus_data[27:16] <= command[63:52];
                                ad9854_ftw_byte_shift_state <= AD9854_BYTE_SHIFT_STEP_1;
                            end else begin
                                bus_data <= command[63:36];
                            end  
`else
                            bus_data <= command[63:36];
`endif                      
                            bus_data15_used_as_strobe <= 0;
                            if (bus_clock) bus_clock <= 0; else bus_clock <= 1;                   
                            strobe_generator_state <= DELAY_CYCLE;   
                            address <= address + 1; 
                        end
                        CMD_STEP_SPI:begin
                            wait_time[30:0] <= command[30:5];
                            wait_time[47:31] <= 0;
                            bus_data <= command[63:36];
                            //the following settings are only used if bus_data15_used_as_strobe == 1
                            //bus_data[15] can be used as strobe, if transparent latches are used and bus_strobe == 1 
                            //with the following settings, we can smoothly switch between normal strobe generation mode and 
                            //having bus_strobe permanently 1 and generating the strobe on bus_data[15]

                            //bus_data15_used_as_strobe can be used to safe the CPU that prepares the sequence work, 
                            //as it doesn't need to copy bus_data[15] data and strobe data into the higher command bits if 
                            // bus_data15_used_as_strobe ==0, i.e. in the many cases in which 
                            //the normal strobe is used and bus_data[15] is not used as strobe                            
                            bus_data15_used_as_strobe <= 1;  
                            //bus_data[15] at beginning of strobe is set already, no need to store
                            bus_data15_second_part <= command[31:31];  
                            bus_data15_idle_part <= command[32:32];
                            //the following settings are required to enable smooth switching between normal strobe mode and strobe on bus_data[15] with bus_strobe = 1
                            bus_strobe_first_part <= command[33:33];
                            bus_strobe_second_part <= command[34:34];
                            bus_strobe_idle_part <= command[35:35];

                            if (bus_clock) bus_clock <= 0; else bus_clock <= 1;                   
                            strobe_generator_state <= DELAY_CYCLE;   
                            address <= address + 1; 
                        end
                        CMD_STEP_AND_ENTER_FAST_MODE: begin
                            wait_time[30:0] <= command[35:5];
                            wait_time[47:31] <= 0;
                            bus_data <= command[63:36];     
                            if (bus_clock) bus_clock <= 0; else bus_clock <= 1;   
                            bus_data15_used_as_strobe <= 0;                
                            strobe_generator_state <= DELAY_CYCLE;    
                            fast_mode <= 1;
                            fast_mode_step <= 0;
                            address <= address + 1;
                        end
                        CMD_LONG_WAIT:begin
                            wait_time[47:0] <= command_buffer[52:5];  
                            address <= address + 1;  
                        end
                        CMD_LOAD_REG_LOW:begin
                            register[58:0] <= command[63:5];
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_LOAD_REG_HIGH:begin
                            register[117:59] <= command[63:5];
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_SET_STROBE_OPTIONS: begin
                            strobe_choice <= command_buffer[11:8]; // 3 bit
                            strobe_first_part_length <= command_buffer[23:16]; // 8 bit
                            strobe_second_part_length <= command_buffer[31:24]; // 8 bit
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_SET_INPUT_BUF_MEM: begin
                            //output reg [11:0] input_buf_mem_address = 0;
                            //output reg [31:0] input_buf_mem_data = 0;
                            if (command_buffer[7:7] == 1) input_buf_mem_address <= command_buffer[20:8];
                            //else input_buf_mem_address <= input_buf_mem_address + 1;
                            input_buf_mem_data <= command_buffer[63:32];
                            INPUT_MEM_state <= INPUT_MEM_WRITE;
                            wait_time <= 1;
                            address <= address + 1;             
                        end 
                        CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM: begin
                            //output reg [11:0] input_buf_mem_address = 0;
                            //output reg [31:0] input_buf_mem_data = 0;
                            //if (command_buffer[7:7] == 1) input_buf_mem_address <= command_buffer[20:8];
                            //else input_buf_mem_address <= input_buf_mem_address + 1;
                            input_buf_mem_data <= cycle_count_since_startup[31:0];
                            input_buf_data_high_32_bit[23:0] <= cycle_count_since_startup[55:32]; //reg [55:0] cycle_count_since_startup = 0;
                            input_buf_data_high_32_bit[31:24] <= 0;
                            INPUT_MEM_state <= INPUT_MEM_WRITE_64BIT;
                            wait_time <= 1;
                            address <= address + 1;             
                        end 
                        
                        
                        CMD_WAIT_FOR_TRIGGER: begin
                            if ((trigger_0_sync && (command_buffer[8:8] == 1)) || (trigger_1_sync && (command_buffer[9:9] == 1)) || (trigger_PS_sync && (command_buffer[10:10] == 1))) address <= address + 1;
                            wait_time <= 1;
                        end
                        
                        CMD_SET_LOOP_COUNT: begin
                            loop_count <= command_buffer[63:32];
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_CONDITIONAL_JUMP_FORWARD: begin  //here we assume that the program assembling the sequence has made sure that the jump is within the current BRAM half
                            if ((selected_core_dig_in && dig_in_jump_enabled) || (condition_0_sync && (command_buffer[9:9] == 1)) || (condition_1_sync && (command_buffer[10:10] == 1)) || (condition_PS_sync && (command_buffer[11:11] == 1)) || (command_buffer[12:12] == 1)) 
                                address <= address + jump_offset;
                            else address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_CONDITIONAL_JUMP_BACKWARD: begin  //here we assume that the program assembling the sequence has made sure that the jump is within the current BRAM half
                            if ((   (selected_core_dig_in && dig_in_jump_enabled) ||   (condition_0_sync && (command_buffer[9:9] == 1)) || (condition_1_sync && (command_buffer[10:10] == 1)) || (condition_PS_sync && (command_buffer[11:11] == 1)) || 
                                 (command_buffer[12:12] == 1) || ((command_buffer[13:13] == 1) && (loop_count > 0)))) 
                                address <= address - jump_offset;
                            else address <= address + 1;
                            loop_count <= loop_count - 1;
                            wait_time <= 1;
                        end
                        CMD_I2C_OUT : begin
                            I2C_out_length <= command_buffer[14:8];
                            I2C_in_length <= command_buffer[20:15];
                            I2C_SELECT_NEXT <= command_buffer[21:20];
                            I2C_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_I2C_OUT to copy 117-bit I2C data to register                
                            I2C_state <= I2C_START;
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        
                        
                        CMD_SPI_OUT_IN : begin
                            SPI_READY_ACTIVE <= command_buffer[5:5];
                            SPI_WAIT_FOR_READY_EDGE_TO_ACTIVE <= command_buffer[6:6];
                            SPI_WAIT_FOR_READY_ACTIVE <= command_buffer[7:7];
                            SPI_OUT_length <= command_buffer[14:8];
                            SPI_IN_length <= command_buffer[21:16];
                            SPI_IN_bit_nr <= command_buffer[21:16];
                            SPI_SEL_next <= command_buffer[33:32];
                            SPI_chip_select[2:0] <= command_buffer[36:34];
                            SPI_chip_select[3:3] <= 1;
                            SPI_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_SPI_OUT_IN to copy 117-bit I2C data to register    
                            //input_buf_mem_data[15:0] <= 0;                                      
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command_buffer[40:40] == 0) SPI_state <= SPI_START;    
                            address <= address + 1;      
                            wait_time <= 1;                                               
                        end       
                        CMD_INPUT_REPEATED_OUT_IN : begin   //This is a two cycle operation. The last state has to be LOAD_EXTENDED_DATA, in order to avoid writing the flg given here to the channels. The opcode I2C_OUT is encountered in that state, and argument stored. Here we use this stored argument.
                            SPI_RESTART_INPUT_REPEAT_WAIT_ON_READY_ACTIVE <= command_buffer[7:7];
                            INPUT_REPEAT_repeats <= command_buffer[27:8]; 
                            INPUT_REPEAT_wait <= command_buffer[55:32]; //Use LOAD_EXTENDED_DATA before INPUT_REPEATED_OUT to copy 64-bit channel content to extended_data                     
                            INPUT_REPEAT_trigger_secondary_interrupt_when_finished <= command_buffer[56:56];
                            INPUT_REPEAT_state <= INPUT_REPEAT_START;
                            INPUT_REPEAT_command <= input_repeat_command_t'(command_buffer[59:57]);
                            if (bus_clock) bus_clock <= 0; else bus_clock <= 1;      
                            bus_data15_used_as_strobe <= 0;                                    
                            strobe_generator_state <= DELAY_CYCLE;    
                            address <= address + 1;   
                            wait_time <= 1;               
                        end           
                        
                        
                        CMD_WAIT_FOR_WAIT_CYCLE_NR: begin //execute next command only when wait_cycles has grown to specified value
                            if (wait_cycles == command_buffer[55:8]) address <= address + 1;
                            wait_time <= 1;
                        end
                        
                        
                        CMD_SET_PERIODIC_TRIGGER_PERIOD: begin  
                            periodic_trigger_period <= command_buffer[55:8];
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME: begin 
                            periodic_trigger_allowed_wait_cycles <= command_buffer[55:8];
                            address <= address + 1;
                            wait_time <= 1;
                        end 
                        CMD_WAIT_FOR_PERIODIC_TRIGGER: begin 
                            if (periodic_trigger_signal == 1) begin //note: if this if section is commented out the program indeed stalls
                                if (periodic_trigger_wait_cycles < periodic_trigger_allowed_wait_cycles) warning_missed_periodic_trigger <= 0;
                                else warning_missed_periodic_trigger <= 1;  
                                address <= address + 1;
                            end
                            wait_time <= 1;
                        end 
                        
                        
                        CMD_DIG_IN: begin
                            input_buf_mem_data[7:0] <= core_dig_in_sync;
                            input_buf_mem_data[31:8] <= command_buffer[31:8];
                            //input_buf_mem_address <= input_buf_mem_address + 1;
                            INPUT_MEM_state <= INPUT_MEM_WRITE;
                            address <= address + 1;
                            wait_time <= 1;
                        end
                        CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT: begin
                            secondary_PS_PL_interrupt <= 1;
                            address <= address + 1;
                            wait_time <= 1;
                        end
`ifdef USE_ZYNQ_ADC                        
                        CMD_ANALOG_IN_OUT: begin
                            adc_register_address <= command_buffer[14:8];  //to read standard analog in, this should be 3, see Xilinx user guide UG480
                            adc_write_enable <= command_buffer[15:15];
                            adc_programming_out <= command_buffer[31:16];   
                            wait_time[29:0] <= command_buffer[63:34];
                            wait_time[47:30] <= 0;                         
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command_buffer[32:32] == 0) begin  //if command[32:32] is high, the actual reading will be started trhough CMD_REPEAT
                                if (command_buffer[33:33] == 0) ANA_IN_state <= ANA_IN_START; //conversion and register read
                                else ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE; //only register read or write
                            end
                            address <= address + 1;  
                        end
`endif                        
                        CMD_PL_TO_PS_COMMAND: begin
                            PL_to_PS_command <= command_buffer[23:8]; 
                            address <= address + 1;
                            wait_time <= 1;
                        end
`ifdef USE_AD9854_AS_VCO
                        CMD_CALC_AD9854_FREQUENCY_TUNING_WORD: begin
                            //we assume that the result of the ADC conversion is available in input_buf_mem_data_SPI
                            AD9854FTWIntermediate[15:0] <= input_buf_mem_data_SPI[15:0];
                            AD9854FTWIntermediate[47:16] <= 0;
                            calc_ad9854_ftw_state <= CALC_AD9854_FTW_START;
                            address <= address + 1;
                            wait_time <= 3; //wait for ftw calculation to finish. a wait_time of 2 should be sufficient. 3 for safety.
                        end
`endif                        

                        default: begin
                            address <= address + 1;
                            wait_time <= 1;
                        end
                    endcase
                end
            end    


`ifdef USE_AD9854_AS_VCO
//The frequency tuning word is proportional to the period of the DDS output frequency, i.e. it's 1/frequency
            // As with all Analog Devices DDS devices, the value of the frequency tuning word is determined by
            // FTW = (Desired Output Frequency × 2^N)/SYSCLK
            // where:
            // N is the phase accumulator resolution (48 bits in this instance).
            // Desired Output Frequency is expressed in hertz.
            // FTW (frequency tuning word) is a decimal number. 
            //
            //The desired frequency is
            // f = f0 + c * voltage = f0 + deltaf
            // voltage is the 16-bit value provided by the ADC 
            //We don't want to use a multiplication. Instead we approximate, using epsilon = deltaf/f0 << 1.
            // delta f = c* voltage
            // ftw = 1/f = 1/(f0 + deltaf) = 1/ (f0 * (1 + deltaf/f0)) = (1/f0)*(1/(1+epsilon)) ~ ftw0 * (1-epsilon) 
            //     = ftw0 - ftw0*deltaf/f0 = ftw0 - ftw0 * ftw0 * c * voltage = ftw0 - scale * ADC_value
            //We replace the multiplication by a bitshift, i.e. we allow scale = 2^n with n=[0...32].
            //
            // Assuming the ADC provides 16 bits
            //	min frequency change: (1 << bit_shift) SYSCLK / (2<<48)  frequency range:  (1 << (bit_shift+16)) SYSCLK / (2<<48)
            //	2^48=	281,474,976,710,656	SYSCLCK	80000000
            //	bitshift	1<<bitshift	deltaf_min	deltaf_max
            //	0	1			2.84217E-07	0.018626451
            //	1	2			5.68434E-07	0.037252903
            //	2	4			1.13687E-06	0.074505806
            //	3	8			2.27374E-06	0.149011612
            //	4	16			4.54747E-06	0.298023224
            //	5	32			9.09495E-06	0.596046448
            //	6	64			1.81899E-05	1.192092896
            //	7	128			3.63798E-05	2.384185791
            //	8	256			7.27596E-05	4.768371582
            //	9	512			0.000145519	9.536743164
            //	10	1024		0.000291038	19.07348633
            //	11	2048		0.000582077	38.14697266
            //	12	4096		0.001164153	76.29394531
            //	13	8192		0.002328306	152.5878906
            //	14	16384		0.004656613	305.1757813
            //	15	32768		0.009313226	610.3515625
            //	16	65536		0.018626451	1220.703125
            //	17	131072		0.037252903	2441.40625
            //	18	262144		0.074505806	4882.8125
            //	19	524288		0.149011612	9765.625
            //	20	1048576		0.298023224	19531.25
            //	21	2097152		0.596046448	39062.5
            //	22	4194304		1.192092896	78125
            //	23	8388608		2.384185791	156250
            //	24	16777216	4.768371582	312500
            //	25	33554432	9.536743164	625000
            //	26	67108864	19.07348633	1250000
            //	27	134217728	38.14697266	2500000
            //	28	268435456	76.29394531	5000000
            //	29	536870912	152.5878906	10000000
            //	30	1073741824	305.1757813	20000000
            //	31	2147483648	610.3515625	40000000
            //
            case (calc_ad9854_ftw_state)
                CALC_AD9854_FTW_IDLE: begin
                    send_AD9854_ftw <= 0;
                    end
                CALC_AD9854_FTW_START: begin
                    //shift 16-bit ADC value by up to 32 bit to align with 48-bit ftw
                    //here we assume unsigend bit shift. For signed, use <<<
                    AD9854FTWShifted <= AD9854FTWIntermediate << command_buffer[12:8]; 
                    calc_ad9854_ftw_state <= CALC_AD9854_FTW_STOP;
                end
                CALC_AD9854_FTW_STOP: begin
                   AD9854FTW <= command_buffer[63:16] - AD9854FTWShifted; //user provided FTW - shifted ADC value
                   calc_ad9854_ftw_state <= CALC_AD9854_FTW_IDLE;
                   send_AD9854_ftw <= 1;
                   AD9854_ftw_byte_shifts_to_do <= 6;
                end
                default: begin
                   calc_ad9854_ftw_state <= CALC_AD9854_FTW_IDLE;
                end
            endcase

            case (ad9854_ftw_byte_shift_state)
                AD9854_BYTE_SHIFT_IDLE: begin
                    end
                AD9854_BYTE_SHIFT_STEP_1: begin
                    //the last 8 bits are written twice, as they are also needed for the IOUpdate command to the AD9854 DDS
                    //therefore only byte shift 5 times, not 6.
                    if (AD9854_ftw_byte_shifts_to_do > 1) begin
                        AD9854FTWbyteshifted[39:0] <= AD9854FTW[47:8];
                        AD9854FTWbyteshifted[47:40] <= 0;
                    end
                    AD9854_ftw_byte_shifts_to_do <= AD9854_ftw_byte_shifts_to_do - 1;
                    ad9854_ftw_byte_shift_state <= AD9854_BYTE_SHIFT_STEP_2;
                end
                AD9854_BYTE_SHIFT_STEP_2: begin
                    //after the 6th call, the ftw has been written out and we go back to normal CMD_STEP mode
                    if (AD9854_ftw_byte_shifts_to_do == 0) send_AD9854_ftw <= 0;
                    AD9854FTW <= AD9854FTWbyteshifted;
                    ad9854_ftw_byte_shift_state <= AD9854_BYTE_SHIFT_IDLE;
                end
                default: begin
                   ad9854_ftw_byte_shift_state <= AD9854_BYTE_SHIFT_IDLE;
                end
            endcase
`endif                 

                   
            case (strobe_generator_state)
                STROBE_GEN_IDLE: begin
                        if (bus_data15_used_as_strobe) begin
                            
                        end else begin
                            bus_strobe <= 0;
                        end
                        strobe_delay <= 0;
                    end
                DELAY_CYCLE: begin
                    if (strobe_delay < strobe_first_part_length) begin                
                        strobe_delay <= strobe_delay + 1;
                        if (bus_data15_used_as_strobe) begin
                            bus_strobe <= bus_strobe_first_part;
                        end else begin
                            bus_strobe <= 0;
                        end
                    end else begin
                        strobe_delay <= 0;
                        if (bus_data15_used_as_strobe) begin
                            bus_strobe <= bus_strobe_second_part;
                            bus_data[15] <= bus_data15_second_part;
                        end else begin
                            bus_strobe <= 1;
                        end                                     
                        strobe_generator_state <= PULSE_CYCLE;
                    end
                end
                PULSE_CYCLE: begin
                    if (strobe_delay < strobe_second_part_length) begin                
                        strobe_delay <= strobe_delay + 1;
                        //bus_strobe <= 1;
                    end else begin
                        strobe_delay <= 0;                        
                        if (bus_data15_used_as_strobe) begin
                            bus_strobe <= bus_strobe_idle_part;
                            bus_data[15] <= bus_data15_idle_part;
                        end else begin
                            bus_strobe <= 0;
                        end
                        strobe_generator_state <= STROBE_GEN_IDLE;
                    end
                end
                default: begin
                   strobe_generator_state <= STROBE_GEN_IDLE;
                   strobe_delay <= 0;
                   bus_strobe <= 0;
                end
            endcase
            
            

            //I2C transmission; would be nice to have this in second "always" block, but then the "two values on one net" error occurs
             //here we assume a 100MHz clock, i.e. 10ns per cycle
             case (I2C_state)
                I2C_IDLE: begin
                        I2C_SCL <= 1;
                        I2C_SDA_OUT <= 1;
                        I2C_delay <= 0;
                        I2C_SELECT_0 <= 0;
                        I2C_SELECT_1 <= 0;
                        I2C_TX_READ_ADDRESS <= 0;
                    end
                I2C_START: begin
                    if (I2C_delay<I2C_delay_start_stop) begin  //pull SDA low while SCL is high
                        I2C_SELECT_0 <= I2C_SELECT_NEXT[0:0];
                        I2C_SELECT_1 <= I2C_SELECT_NEXT[1:1];                        
                        I2C_SCL <= 1;
                        I2C_SDA_OUT <= 0;
                        I2C_bit <= 0;
                        I2C_read_bit <= 0;
                        input_buf_mem_data_I2C <= 0;
                        I2C_TX_READ_ADDRESS <= 0;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_START_SCL_LOW;
                    end
                end
                I2C_START_SCL_LOW: begin
                    if (I2C_delay<I2C_delay_clock_low) begin
                        I2C_SCL <= 0;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_bit <= 0;
                        if (I2C_TX_READ_ADDRESS) begin
                            I2C_state <= I2C_DATA_LOAD;
                        end else if (I2C_out_length != 0) begin
                            I2C_TX_READ_ADDRESS <= 0;
                            I2C_state <= I2C_DATA_LOAD;
                        end else if (I2C_in_length != 0) begin
                            I2C_TX_READ_ADDRESS <= 1;
                            I2C_state <= I2C_DATA_LOAD;
                        end else begin
                            I2C_state <= I2C_STOP;
                        end
                    end
                end
                I2C_DATA_LOAD: begin
                    if (I2C_delay<I2C_delay_data_setup) begin
                        if (I2C_TX_READ_ADDRESS)
                            I2C_SDA_OUT <= I2C_data[I2C_out_length + 7 - I2C_bit];
                        else
                            I2C_SDA_OUT <= I2C_data[I2C_out_length - 1 - I2C_bit];
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;                
                        I2C_state <= I2C_CLOCK_HIGH;
                    end
                end
                I2C_CLOCK_HIGH: begin
                    if (I2C_delay<I2C_delay_clock_high) begin
                        I2C_SCL <= 1;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;         
                        I2C_bit <= I2C_bit +1;       
                        I2C_state <= I2C_CLOCK_LOW;
                    end
                end
                I2C_CLOCK_LOW: begin
                    if (I2C_delay<I2C_delay_clock_low) begin
                        I2C_SCL <= 0;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;           
                        if (I2C_TX_READ_ADDRESS) begin
                            if (I2C_bit[2:0] == 0) I2C_state <= I2C_WRITE_ACK_SETUP;
                            else I2C_state <= I2C_DATA_LOAD;
                        end else if (I2C_bit < I2C_out_length) begin
                            if (I2C_bit[2:0] == 0) I2C_state <= I2C_WRITE_ACK_SETUP;
                            else I2C_state <= I2C_DATA_LOAD;
                        end else begin
                            if (I2C_bit[2:0] == 0) I2C_state <= I2C_WRITE_ACK_SETUP;
                            else if (I2C_in_length != 0) I2C_state <= I2C_PAUSE_BEFORE_READ;
                            else I2C_state <= I2C_STOP;
                        end 
                    end
                end
                I2C_WRITE_ACK_SETUP: begin
                    if (I2C_delay<I2C_delay_data_setup) begin
                        I2C_SDA_OUT <= 1; //release SDA for slave ACK
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_WRITE_ACK_CLOCK_HIGH;
                    end
                end
                I2C_WRITE_ACK_CLOCK_HIGH: begin
                    if (I2C_delay<I2C_delay_clock_high) begin
                        I2C_SCL <= 1;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        if ((I2C_SDA_IN_0 & I2C_SELECT_NEXT[0:0]) | (I2C_SDA_IN_1 & I2C_SELECT_NEXT[1:1])) error_detected <= 1;
                        I2C_state <= I2C_WRITE_ACK_CLOCK_LOW;
                    end
                end
                I2C_WRITE_ACK_CLOCK_LOW: begin
                    if (I2C_delay<I2C_delay_clock_low) begin
                        I2C_SCL <= 0;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        if (I2C_TX_READ_ADDRESS) begin
                            I2C_read_bit <= 0;
                            I2C_state <= I2C_READ_SETUP;
                        end else if (I2C_bit < I2C_out_length) begin
                            I2C_state <= I2C_DATA_LOAD;
                        end else if (I2C_in_length != 0) begin
                            I2C_state <= I2C_PAUSE_BEFORE_READ;
                        end else begin
                            I2C_state <= I2C_STOP;
                        end
                    end
                end
                I2C_PAUSE_BEFORE_READ: begin
                    if (I2C_delay<I2C_delay_pause_before_read) begin
                        I2C_SCL <= 0;
                        I2C_SDA_OUT <= 1;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_RESTART_SCL_HIGH;
                    end
                end
                I2C_RESTART_SCL_HIGH: begin
                    if (I2C_delay<I2C_delay_start_stop) begin
                        I2C_SCL <= 1;
                        I2C_SDA_OUT <= 1;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_RESTART_SDA_LOW;
                    end
                end
                I2C_RESTART_SDA_LOW: begin
                    if (I2C_delay<I2C_delay_start_stop) begin
                        I2C_SCL <= 1;
                        I2C_SDA_OUT <= 0;
                        I2C_bit <= 0;
                        I2C_TX_READ_ADDRESS <= 1;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_START_SCL_LOW;
                    end
                end
                I2C_READ_SETUP: begin
                    if (I2C_delay<I2C_delay_data_setup) begin
                        I2C_SDA_OUT <= 1; //release SDA for slave data
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_READ_CLOCK_HIGH;
                    end
                end
                I2C_READ_CLOCK_HIGH: begin
                    if (I2C_delay<I2C_delay_clock_high) begin
                        I2C_SCL <= 1;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        input_buf_mem_data_I2C[I2C_in_length - 1 - I2C_read_bit] <= (I2C_SDA_IN_0 & I2C_SELECT_NEXT[0:0]) | (I2C_SDA_IN_1 & I2C_SELECT_NEXT[1:1]);
                        I2C_read_bit <= I2C_read_bit + 1;
                        I2C_state <= I2C_READ_CLOCK_LOW;
                    end
                end
                I2C_READ_CLOCK_LOW: begin
                    if (I2C_delay<I2C_delay_clock_low) begin
                        I2C_SCL <= 0;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        if ((I2C_read_bit == I2C_in_length) || (I2C_read_bit[2:0] == 0))
                            I2C_state <= I2C_READ_ACK_SETUP;
                        else
                            I2C_state <= I2C_READ_SETUP;
                    end
                end
                I2C_READ_ACK_SETUP: begin
                    if (I2C_delay<I2C_delay_data_setup) begin
                        I2C_SDA_OUT <= (I2C_read_bit == I2C_in_length) ? 1 : 0; //NACK final byte, ACK intermediate bytes
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_READ_ACK_CLOCK_HIGH;
                    end
                end
                I2C_READ_ACK_CLOCK_HIGH: begin
                    if (I2C_delay<I2C_delay_clock_high) begin
                        I2C_SCL <= 1;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_READ_ACK_CLOCK_LOW;
                    end
                end
                I2C_READ_ACK_CLOCK_LOW: begin
                    if (I2C_delay<I2C_delay_clock_low) begin
                        I2C_SCL <= 0;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        if (I2C_read_bit == I2C_in_length)
                            I2C_state <= I2C_WRITE_TO_INPUT_MEMORY;
                        else
                            I2C_state <= I2C_READ_SETUP;
                    end
                end
                I2C_WRITE_TO_INPUT_MEMORY: begin
                    input_buf_mem_data[31:0] <= input_buf_mem_data_I2C[31:0];
                    INPUT_MEM_state <= INPUT_MEM_WRITE;
                    I2C_state <= I2C_STOP;
                end
                I2C_STOP: begin
                    if (I2C_delay<I2C_delay_clock_low) begin
                        I2C_SCL <= 0;
                        I2C_SDA_OUT <= 0;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;              
                        I2C_state <= I2C_STOP_SCL_HIGH;
                    end
                end
                I2C_STOP_SCL_HIGH: begin
                    if (I2C_delay<I2C_delay_start_stop) begin
                        I2C_SCL <= 1;
                        I2C_SDA_OUT <= 0;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;              
                        I2C_state <= I2C_STOP_SDA_RELEASE;
                    end
                end
                I2C_STOP_SDA_RELEASE: begin
                    if (I2C_delay<I2C_delay_start_stop) begin
                        I2C_SCL <= 1;
                        I2C_SDA_OUT <= 1;
                        I2C_delay <= I2C_delay + 1;
                    end else begin
                        I2C_delay <= 0;
                        I2C_state <= I2C_IDLE;
                    end
                end
                default: I2C_state <= I2C_IDLE;
            endcase
            
            
            
            
             //here we assume a 100MHz clock, i.e. 10ns per cycle
             //reminder: SPI IO: 
                //input SPI_IN;
                //output reg SPI_OUT = 0;
                //output reg SPI_SCK = 0;
                //output reg [1:0] SPI_SEL = 2'b11;     
                //reg [7:0] SPI_delay = 0;
                //reg [7:0] SPI_OUT_bit_nr = 0;
                //reg [6:0] SPI_IN_bit_nr = 0;
                //reg [117:0] SPI_data = 0;
                //reg [6:0] SPI_OUT_length = 0;
                //reg [5:0] SPI_IN_length = 0;
                //reg [1:0] SPI_SEL_next = 1;
                //reg [7:0] SPI_delay_CS_low_start_wait = 4;
                //reg [7:0] SPI_delay_write = 4;
                //reg [11:0] SPI_delay_pause_before_read = 4;
                //reg [7:0] SPI_delay_read = 23;
                //reg [7:0] SPI_delay_CS_low_end_wait = 4;          
             case (SPI_state)
                SPI_IDLE: begin
                        SPI_SCK <= SPI_SCK_idle_phase;
                        SPI_OUT <= 1;
                        SPI_delay <= 0;
                        SPI_SELECT_0 <= 0;
                        SPI_SELECT_1 <= 0;    
                        SPI_OUT_ENDED <= 0;
                        SPI_chip_select[3:3] <= 1;
                        //SPI_chip_select[2:0] are set in CMD_SPI_OUT_IN while SPI_chip_select[3:3] is high.
                        input_buf_mem_data_SPI <= 0;
                    end
                SPI_START: begin
                    if (SPI_delay<SPI_delay_CS_low_start_wait) begin //50ns from CS low to start of normal clock cycle (i.e. 100ns till first clock rise)
                        SPI_OUT_bit_nr <= SPI_OUT_length - 1;
                        SPI_IN_bit_nr <= SPI_IN_length;
                        SPI_SCK <= SPI_SCK_idle_phase;
                        SPI_OUT <= 1;
                        SPI_SELECT_0 <= SPI_SEL_next[0:0];
                        SPI_SELECT_1 <= SPI_SEL_next[1:1];
                        SPI_OUT_ENDED <= 0;
                        SPI_chip_select[3:3] <= 0;
                        input_buf_mem_data_SPI <= 0;
                        SPI_delay <= SPI_delay + 1;
                    end else begin
                        SPI_delay <= 0;
                        if (SPI_WAIT_FOR_READY_ACTIVE == 1) SPI_state <= SPI_STATE_WAIT_FOR_READY_ACTIVE;
                        else if (SPI_WAIT_FOR_READY_EDGE_TO_ACTIVE == 1) SPI_state <= SPI_STATE_WAIT_FOR_READY_INACTIVE;
                        else SPI_state <= SPI_DATA_OUT_DRIVE_PHASE;
                    end
                end
                SPI_STATE_WAIT_FOR_READY_INACTIVE: begin
                    if (SPI_READY_sync == SPI_READY_INACTIVE) SPI_state <= SPI_STATE_WAIT_FOR_READY_ACTIVE;
                end
                SPI_STATE_WAIT_FOR_READY_ACTIVE: begin
                    if (SPI_READY_sync == SPI_READY_ACTIVE) begin
                        SPI_state <= SPI_DATA_OUT_DRIVE_PHASE;
                        if (SPI_RESTART_INPUT_REPEAT_WAIT_ON_READY_ACTIVE ==1) INPUT_REPEAT_set_to_delay_zero_flag <= 1;
                    end
                end
                SPI_DATA_OUT_DRIVE_PHASE: begin
                    if (SPI_delay<SPI_delay_write) begin  //drive MOSI during the SPI change phase and wait
                        SPI_SCK <= SPI_SCK_drive_phase;
                        SPI_OUT <= SPI_data[SPI_OUT_bit_nr];
                        SPI_delay <= SPI_delay + 1;
                        if (SPI_OUT_bit_nr == 0) SPI_OUT_ENDED <= 1;
                    end else begin
                        SPI_delay <= 0;
                        SPI_OUT_bit_nr <= SPI_OUT_bit_nr - 1;
                        SPI_state <= SPI_DATA_OUT_SAMPLE_PHASE;
                    end
                end
                SPI_DATA_OUT_SAMPLE_PHASE: begin
                    if (SPI_delay<SPI_delay_write) begin  //present the SPI sample phase and wait
                        SPI_SCK <= SPI_SCK_sample_phase;
                        SPI_delay <= SPI_delay + 1;                        
                    end else begin
                        SPI_delay <= 0;                
                        if (SPI_OUT_ENDED == 1)
                            if (SPI_IN_length == 0) SPI_state <= SPI_FINAL_PAUSE;
                            else SPI_state <= SPI_PAUSE_BEFORE_READ;
                        else
                            SPI_state <= SPI_DATA_OUT_DRIVE_PHASE;
                    end
                end

                SPI_PAUSE_BEFORE_READ: begin                  
                    if (SPI_delay<SPI_delay_pause_before_read) begin //pause after write before read                     
                        SPI_delay <= SPI_delay + 1;
                    end else begin
                        SPI_delay <= 0;
                        SPI_state <= SPI_DATA_IN_PRE_SAMPLE;
                    end                                      
                end

                SPI_DATA_IN_PRE_SAMPLE: begin
                    SPI_IN_bit_nr <= SPI_IN_bit_nr - 1;                                
                    SPI_state <= SPI_DATA_IN_DRIVE_PHASE_WAIT;
                end
                SPI_DATA_IN_DRIVE_PHASE_WAIT: begin
                    if (SPI_delay<SPI_delay_read) begin  //present the SPI change phase and wait for input data to settle
                        SPI_SCK <= SPI_SCK_drive_phase;
                        SPI_delay <= SPI_delay + 1;                        
                    end else begin
                        SPI_delay <= 0;                               
                        SPI_SCK <= SPI_SCK_sample_phase;
                        SPI_state <= SPI_DATA_IN_SAMPLE_PHASE_WAIT;
                        input_buf_mem_data_SPI[SPI_IN_bit_nr] <= (SPI_IN_0 & SPI_SELECT_0) | (SPI_IN_1 & SPI_SELECT_1);
                        
                    end
                end
                SPI_DATA_IN_SAMPLE_PHASE_WAIT: begin
                    if (SPI_delay<SPI_delay_read) begin  //hold the SPI sample phase before the next bit
                        SPI_delay <= SPI_delay + 1;                        
                    end else begin
                        SPI_delay <= 0;         
                        if (SPI_IN_bit_nr == 0) begin
                            SPI_state <= SPI_WRITE_TO_INPUT_MEMORY;
                        end else begin
                            SPI_state <= SPI_DATA_IN_PRE_SAMPLE;
                        end 
                    end
                end
                SPI_WRITE_TO_INPUT_MEMORY: begin
                    //the input_buf_mem_data_SPI register is used to separate circuitry needed for individial bit setting in SPI from circutry needed to write to input_mem
                    //input_buf_mem_data is accessed by ana_in, dig_in, SPI_in, ... The hope is to not overcomplicate things by adding bit manipulation circuitry to that. 
                    //if circuit would become too complicated we will get issues with timing.
                    input_buf_mem_data[31:0] <= input_buf_mem_data_SPI[31:0];
                    INPUT_MEM_state <= INPUT_MEM_WRITE;
                    SPI_state <= SPI_FINAL_PAUSE;
                end


                SPI_FINAL_PAUSE: begin                  
                    if (SPI_delay<SPI_delay_CS_low_end_wait) begin //pause after write before read                    
                        SPI_delay <= SPI_delay + 1;
                    end else begin
                        SPI_delay <= 0;
                        SPI_state <= SPI_STOP;   
                    end                                      
                end

                SPI_STOP: begin
                    //input_buf_mem_address <= input_buf_mem_address + 1;
                    SPI_state <= SPI_IDLE;                    
                end
                default: SPI_state <= SPI_IDLE;
            endcase
            
            case (INPUT_MEM_state)
                INPUT_MEM_IDLE: begin
                end
                
                INPUT_MEM_WRITE_64BIT: begin
                    input_buf_mem_write <= 1;
                    INPUT_MEM_state <= INPUT_MEM_WRITE_2_64BIT;
                end
                INPUT_MEM_WRITE_2_64BIT: begin
                    INPUT_MEM_state <= INPUT_MEM_WRITE_3_64BIT;
                end
                INPUT_MEM_WRITE_3_64BIT: begin
                    input_buf_mem_write <= 0;
                    input_buf_mem_address <= input_buf_mem_address + 1;
                    INPUT_MEM_state <= INPUT_MEM_WRITE_4_64BIT;
                end
                INPUT_MEM_WRITE_4_64BIT: begin
                    input_buf_mem_data <= input_buf_data_high_32_bit;
                    INPUT_MEM_state <= INPUT_MEM_WRITE;
                end

                INPUT_MEM_WRITE: begin
                    input_buf_mem_write <= 1;
                    INPUT_MEM_state <= INPUT_MEM_WRITE_2;
                end
                INPUT_MEM_WRITE_2: begin
                    INPUT_MEM_state <= INPUT_MEM_STOP;
                end
                INPUT_MEM_STOP: begin
                    input_buf_mem_write <= 0;
                    input_buf_mem_address <= input_buf_mem_address + 1;
                    INPUT_MEM_state <= INPUT_MEM_IDLE;
                end
                default: begin
                    input_buf_mem_write <= 0;
                    INPUT_MEM_state <= INPUT_MEM_IDLE;
                end
            endcase

`ifdef USE_ZYNQ_ADC
            //storing adc result in core registers, just in cae adc outputs go to zero when adc_ready goes to zero
            if (adc_ready) begin
                ana_in_data <= adc_result_in;
                ana_in_channel <= adc_channel;
            end 
            
            case (ANA_IN_state)
                ANA_IN_IDLE: begin
                    ANA_IN_delay <= 0;
                    adc_conversion_start <= 0;
                end
                ANA_IN_START: begin
                    if (ANA_IN_delay<110) begin  //need to wait 22 ADCCLK = 88 DCLK  + 16 DCLK = 104 DLCK cycles. Do a bit more for safety
                        adc_conversion_start <= 1;                        
                        ANA_IN_delay <= ANA_IN_delay + 1;
                    end else begin
                        ANA_IN_delay <= 0;
                        adc_conversion_start <= 0;
                        ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE;
                    end
                end
                ANA_IN_TRIGGER_READ_WRITE: begin
                    if (ANA_IN_delay<2) begin
                        ana_in_data_expected <= !adc_write_enable;
                        adc_enable <= 1;
                        ANA_IN_delay <= ANA_IN_delay + 1;
                     end else begin
                        adc_enable <= 0;
                        ANA_IN_delay <= 0;
                        if (ana_in_data_expected) ANA_IN_state <= ANA_IN_INCREASE_INPUT_MEM_ADDRESS;
                        else ANA_IN_state <= ANA_IN_IDLE;
                     end
                end
                ANA_IN_INCREASE_INPUT_MEM_ADDRESS: begin
                    //it nominally takes 5 to 6 DCLK cycles to present result. 
                    //DRDY = adc_ready has a high transition when data ready, which we use to read in data into ana_in_data and ana_in_channel (see "always" block above). 
                    //After a few safety cycles we increase input buffer address. 
                    //The address has to be increased here to avoid multiple driver nets for address.
                    if (ANA_IN_delay<10) begin   
                        ANA_IN_delay <= ANA_IN_delay + 1;
                     end else begin
                        input_buf_mem_data[15:0] <= ana_in_data;
                        input_buf_mem_data[20:16] <= ana_in_channel;
                        input_buf_mem_data[28:21] <= core_dig_in_sync;
                        input_buf_mem_data[30:29] <= INPUT_REPEAT_nr[1:0];
                        input_buf_mem_data[31:31] <= 1'b1; 
                        //input_buf_mem_address <= input_buf_mem_address + 1;
                        INPUT_MEM_state <= INPUT_MEM_WRITE;
                        ana_in_data_expected <= 0;
                        ANA_IN_state <= ANA_IN_IDLE;
                     end
                end
                                
                default: ANA_IN_state <= ANA_IN_IDLE;
            endcase
`endif

            case (DIG_IN_state)
                DIG_IN_IDLE: begin
                                              
                end
                DIG_IN_START: begin
                    input_buf_mem_data[7:0] <= core_dig_in_sync;
                    input_buf_mem_data[28:8] <= INPUT_REPEAT_nr;
                    input_buf_mem_data[31:29] <= 3'b010; 
                    //input_buf_mem_address <= input_buf_mem_address + 1;
                    INPUT_MEM_state <= INPUT_MEM_WRITE;
                    DIG_IN_state <= DIG_IN_IDLE;                 
                end
                default: DIG_IN_state <= DIG_IN_IDLE;
            endcase
            
            

        //reminder: parameters
        //reg [23:0] INPUT_REPEAT_wait = 0;
        //reg [24:0] INPUT_REPEAT_delay = 0;
        //reg [19:0] INPUT_REPEAT_repeats = 0;
        //reg [20:0] INPUT_REPEAT_nr = 0;

        //enum logic [2:0] {  IN_REP_CMD_IDLE,//0
                    //IN_REP_CMD_SPI,//1
                    //IN_REP_CMD_DIG_IN,//2
                    //IN_REP_CMD_DIG_EVENT,//3
                    //IN_REP_CMD_ANA_IN//4
                    //} INPUT_REPEAT_command = IN_REP_CMD_IDLE;

         case (INPUT_REPEAT_state)
                INPUT_REPEAT_IDLE: begin
                        INPUT_REPEAT_nr <= 0;    
                        INPUT_REPEAT_delay <= 0;  
                        INPUT_REPEAT_set_to_delay_zero_flag <=0;
                    end
`ifdef USE_INPUT_EVENT_TAGGER
                INPUT_REPEAT_DIG_EVENT: begin
                    //this mode is active until user stops it with a new CMD_INPUT_REPEATED_OUT_IN command, e.g. one that sets INPUT_REPEAT_command == IN_REP_CMD_IDLE. 
                    if ((input_event_tagger_last_input != core_dig_in_sync) || ((input_event_tagger_mark_counter_overflow == 1))) begin
                        input_event_tagger_last_input <= core_dig_in_sync;
                        
`ifdef USE_INPUT_EVENT_TAGGER_FIFO_8_ENTRIES                        
                        if (input_event_tagger_fifo_count < 8) begin
`else                         
                        if (input_event_tagger_fifo_count < 4) begin
`endif                                                
                            input_event_tagger_fifo[input_event_tagger_fifo_write_ptr] <= {cycle_count_since_startup[21:0], input_event_tagger_mark_counter_overflow, core_dig_in_sync};
                            input_event_tagger_fifo_write_ptr <= input_event_tagger_fifo_write_ptr + 1;
                            input_event_tagger_fifo_count <= input_event_tagger_fifo_count + 1;
                            input_event_tagger_mark_counter_overflow <= 0;
                        end else begin
                            input_event_tagger_fifo_overflow <= 1;
                        end
                    end else begin
                        if ((input_event_tagger_fifo_count > 0) && (INPUT_MEM_state == INPUT_MEM_IDLE)) begin
                            input_buf_mem_data[7:0] <= input_event_tagger_fifo[input_event_tagger_fifo_read_ptr][7:0];
                            input_buf_mem_data[8:8] <= input_event_tagger_fifo[input_event_tagger_fifo_read_ptr][8:8];
                            input_buf_mem_data[9:9] <= input_event_tagger_fifo_overflow;
                            input_buf_mem_data[31:10] <= input_event_tagger_fifo[input_event_tagger_fifo_read_ptr][30:9];
                            input_event_tagger_fifo_read_ptr <= input_event_tagger_fifo_read_ptr + 1;
                            INPUT_MEM_state <= INPUT_MEM_WRITE;
    
                            if (input_event_tagger_last_input != core_dig_in_sync) begin
                                input_event_tagger_last_input <= core_dig_in_sync;
                                input_event_tagger_fifo[input_event_tagger_fifo_write_ptr] <= {cycle_count_since_startup[21:0], input_event_tagger_mark_counter_overflow, core_dig_in_sync};
                                input_event_tagger_fifo_write_ptr <= input_event_tagger_fifo_write_ptr + 1;
                                input_event_tagger_fifo_count <= input_event_tagger_fifo_count;
                                input_event_tagger_mark_counter_overflow <= 0;
                            end else if (input_event_tagger_mark_counter_overflow == 1) begin
                                input_event_tagger_fifo[input_event_tagger_fifo_write_ptr] <= {cycle_count_since_startup[21:0], 1'b1, core_dig_in_sync};
                                input_event_tagger_fifo_write_ptr <= input_event_tagger_fifo_write_ptr + 1;
                                input_event_tagger_fifo_count <= input_event_tagger_fifo_count;
                                input_event_tagger_mark_counter_overflow <= 0;
                            end else begin
                                input_event_tagger_fifo_count <= input_event_tagger_fifo_count - 1;
                            end
    
                            input_event_tagger_fifo_overflow <= 0;                            
                        end
                    end
                    if (cycle_count_since_startup[21:0] == 0) begin
                        input_event_tagger_mark_counter_overflow <= 1;
                    end
                end
`endif
                INPUT_REPEAT_START: begin
                    INPUT_REPEAT_set_to_delay_zero_flag <=0;
                    case (INPUT_REPEAT_command)
                        IN_REP_CMD_SPI: begin
                            if (SPI_state == SPI_IDLE) begin
                                SPI_state <= SPI_START;
                                SPI_IN_bit_nr <= SPI_IN_length;
                                INPUT_REPEAT_state <= INPUT_REPEAT_STARTED;
                            end
                        end
                        IN_REP_CMD_IDLE: begin 
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                        end
                        IN_REP_CMD_DIG_IN: begin
                            if (DIG_IN_state == DIG_IN_IDLE) begin
                                DIG_IN_state <= DIG_IN_START;
                                INPUT_REPEAT_state <= INPUT_REPEAT_STARTED;
                            end
                        end
`ifdef USE_INPUT_EVENT_TAGGER                    
                        IN_REP_CMD_DIG_EVENT: begin 
                            INPUT_REPEAT_state <= INPUT_REPEAT_DIG_EVENT;
                        end
`endif
`ifdef USE_ZYNQ_ADC
                        IN_REP_CMD_ANA_IN: begin 
                            if (ANA_IN_state == ANA_IN_IDLE) begin
                                ANA_IN_state <= ANA_IN_START;
                                INPUT_REPEAT_state <= INPUT_REPEAT_STARTED;
                            end
                        end
`endif              
                        default: begin
                            INPUT_REPEAT_state <= INPUT_REPEAT_STARTED;
                        end
                    endcase

                end
                INPUT_REPEAT_STARTED: begin
                    INPUT_REPEAT_delay <= 0;
                    INPUT_REPEAT_state <= INPUT_REPEAT_WAIT;
                    if (INPUT_REPEAT_repeats != 'hFFFFF) INPUT_REPEAT_nr <= INPUT_REPEAT_nr + 1;       //INPUT_REPEAT_repeats == 'hFFFFF leads to repeats till INPUT_REPEAT_repeats is set to zero
                end
                INPUT_REPEAT_WAIT: begin
                    if (INPUT_REPEAT_set_to_delay_zero_flag == 1) begin
                        INPUT_REPEAT_delay <= 0;
                        INPUT_REPEAT_set_to_delay_zero_flag <= 0;
                    end else if (INPUT_REPEAT_delay < INPUT_REPEAT_wait) begin
                        INPUT_REPEAT_delay <= INPUT_REPEAT_delay + 1;
                    end else begin
                        SPI_delay <= 0;                         
                        if (INPUT_REPEAT_nr < INPUT_REPEAT_repeats)
                            INPUT_REPEAT_state <= INPUT_REPEAT_START;
                        else begin
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (INPUT_REPEAT_trigger_secondary_interrupt_when_finished) 
                                secondary_PS_PL_interrupt <= 1;  
                        end
                    end
                end
                default: INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
            endcase
             
        end
    end
end


    
endmodule
