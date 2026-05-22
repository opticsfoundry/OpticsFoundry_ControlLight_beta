// OpticsFoundryCommandSystemTest.cpp : Defines the entry point for the application.
//


#ifndef _MSC_VER




#else

#include "CPUCommandSequencer.h"

using namespace std;
#include <iostream>


#include <stdio.h>
#include <string.h>
#include "OpticsFoundryFPGA.h"
#include "command_defs.h"
#include "executor.h"

void DisplayResult(CommandStatus status) {
    if (status != CMD_SUCCESS) {
        for (int i = 0; i < CS_GetNrErrorMessages(); ++i) {
            xil_printf("Error message: ", CS_GetErrorMessage(i));
            xil_printf("\n");
        }
        xil_printf("\n");
    }
    else {
        xil_printf("\nExecution complete.\n");
    }

    //    xil_printfsi("Last line number: ", GetLastCommandLineNumber());
    xil_printf("\n");

}

void simple_test() {
    cout << "Simple test function." << endl;
    cout << "Hello CMake." << endl;
    xil_printf("Initializing command system...\n");

    //it is possible to define variables in a first command sequence, then execute another one that uses those variables.
    CS_AssembleSequence();
    CS_AddCommand("Equal(R1, 1.23);");
    CS_ExecuteSequence();

    //new sequence, but using the variable R1 defined in the previous sequence.
    CS_AssembleSequence();
    CS_AddCommand("Print(\"\ncheck R1 is still defined as 1.23 = \");");
    CS_AddCommand("Print(R1);");



    CS_AddCommand("Equal(LoopCounter, 5); //test comment");
    // CS_AddCommand("Print(\"\nloop counter start = \");");
    CS_AddCommand("Print(LoopCounter);");
    CS_AddCommand("LoopCountStart:Sub(LoopCounter, LoopCounter, 1); //test comment");
    CS_AddCommand("Print(\"\nloop 1 counter = \");");
    CS_AddCommand("Print(LoopCounter);");
    CS_AddCommand("JumpIfNotZero(LoopCounter,LoopCountStart:);");


    xil_printf("Adding commands...\n");

    CS_AddCommand("Print(\"\n\n\");");
    // Example: R0 = 5, 0.4, 0xFFF, 0b1010
    CS_AddCommand("Equal(R10, 1.5);");
    CS_AddCommand("Print(R10);");
    CS_AddCommand("Print(\"\ntest\n\");");

    // Example: R1 = R0 * 2
    CS_AddCommand("Mul(R1, R10, 2);");
    CS_AddCommand("Print(R1);Print(\"\n\");");

    // Example: R2 = sin(R1)
    CS_AddCommand("Sin(R2, R1);");
    CS_AddCommand("Print(R2);Print(\"\n\");");

    CS_AddCommand("RecvInt(R4);");
    CS_AddCommand("Print(\"received int = \");Print(R4);Print(\"\n\");");

    CS_AddCommand("Equal(R20, pi);");

    CS_AddCommand("Print(R20);Print(\"\n\");");

    CS_AddCommand("SetRegister(30, R20);");
    CS_AddCommand("Mul(R30, R30, 2);");

    CS_AddCommand("Print(R30);Print(\"\n\");");


    CS_AddCommand("Print(\"Set \");");

    CS_AddCommand("Print(R30);Print(\"\n\");");


    CS_AddCommand("GetRegister(R4, 30);");
    CS_AddCommand("Print(\"Get \");");
    CS_AddCommand("Print(R4);Print(\"\n\");");

    CS_AddCommand("Equal(R1, 0b00);");
    CS_AddCommand("Add(R1, R1, 1);Add(R1, R1, 1); //test comment");
    CS_AddCommand("Print(R1);Print(\"\n\");");

    //CS_AddCommand("JumpIfNotZero(R1,LoooopStart:);"); //test: error must be displayed if this line is there, as label is not defined.

    CS_AddCommand("TestLabel:Equal(R1, 5); //test comment");
    CS_AddCommand("Print(R1);Print(\"\n\");");
    CS_AddCommand("Print(\"\nloop counter start = \");");
    CS_AddCommand("Print(R1);");
    CS_AddCommand("LoopStart:Sub(R1, R1, 1); //test comment");
    CS_AddCommand("Print(\"\nloop 2 counter = \");");
    CS_AddCommand("Print(R1);");
    CS_AddCommand("JumpIfNotZero(R1,LoopStart:);");

    xil_printf("Executing sequence...\n");

    //CS_PrintCommandTable();
    CommandStatus status = CS_ExecuteSequence();
    DisplayResult(status);
}

void comparison_test() {
    CS_AssembleSequence();

    CS_AddCommand("Print(\"\n\nComparisons \");");
    CS_AddCommand("Equal(R1,20);");
    CS_AddCommand("Print(\"\nR1 = \");Print(R1);Print(\"\n\");");
    CS_AddCommand("Print(\"R2 = \");Print(R2);Print(\"\n\");");
    CS_AddCommand("Print(\"R3 = \");Print(R3);Print(\"\n\");");

    CS_AddCommand("Equal(R2,10);");
    CS_AddCommand("Print(\"\nR1 = \");Print(R1);Print(\"\n\");");
    CS_AddCommand("Print(\"R2 = \");Print(R2);Print(\"\n\");");
    CS_AddCommand("Print(\"R3 = \");Print(R3);Print(\"\n\");");

    CS_AddCommand("CmpGe(R3, R1, R2);");
	CS_AddCommand("Print(\"\nR1 = 0x\");PrintHex(R1);Print(\"\n\");");
    CS_AddCommand("Print(\"R2 = 0b\");PrintBinary(R2);Print(\"\n\");");
    CS_AddCommand("Print(\"R3 = \");Print(R3);Print(\"\n\");");
    

    CS_AddCommand("JumpIfNotZero(R3,NotZeroR3:);");
    CS_AddCommand("Print(\"\nR3 is 0, i.e. R1<R2\n\");");
    CS_AddCommand("Print(\"pi = \");Print(pi);Print(\"\n\");");
    CS_AddCommand("Jump(End:);");
    CS_AddCommand("NotZeroR3:");
    CS_AddCommand("Print(\"\nR3 is 1, i.e. R1>=R2\n\");");
    CS_AddCommand("Print(\"e = \");Print(e);Print(\"\n\");");
    CS_AddCommand("End:Print(\"\nEnd\");");


    xil_printf("Executing sequence...\n");

    CS_PrintCommandTable();
    CommandStatus status = CS_ExecuteSequence();
    DisplayResult(status);
}

void bit_manipulation_test() {
    CS_AssembleSequence();

    CS_AddCommand("Print(\"\n\nBit manipulation test \");");
    CS_AddCommand("Equal(R1,0b1100);");
    CS_AddCommand("Print(\"\nR1 = 0b\");PrintBinary(R1);Print(\"\n\");");
    CS_AddCommand("Equal(R2,0b0110);");
    CS_AddCommand("Print(\"\nR2 = 0b\");PrintBinary(R2);Print(\"\n\");");

    
    CS_AddCommand("Or(R0,R1,R2);");
    CS_AddCommand("Print(\"\nR1 | R2 = 0b\");PrintBinary(R0);Print(\"\n\");");

    CS_AddCommand("And(R0,R1,R2);");
    CS_AddCommand("Print(\"\nR1 & R2 = 0b\");PrintBinary(R0);Print(\"\n\");");

	CS_AddCommand("Xor(R0,R1,R2);");
	CS_AddCommand("Print(\"\nR1 ^ R2 = 0b\");PrintBinary(R0);Print(\"\n\");");

	CS_AddCommand("ShiftLeft(R0,R1,2);");
	CS_AddCommand("Print(\"\nR1 << 2 = 0b\");PrintBinary(R0);Print(\"\n\");");
	CS_AddCommand("ShiftRight(R0,R1,2);");
	CS_AddCommand("Print(\"\nR1 >> 2 = 0b\");PrintBinary(R0);Print(\"\n\");");
	CS_AddCommand("Not(R0,R1);");
	CS_AddCommand("Print(\"\n~R1 = 0b\");PrintBinary(R0);Print(\"\n\");");



    xil_printf("Executing sequence...\n");

    CS_PrintCommandTable();
    CommandStatus status = CS_ExecuteSequence();
    DisplayResult(status);
}





void test_AD9854VCO() {
    cout << "test_AD9854VCO" << endl;
    
    CS_AssembleSequence();

    CS_AddCommand("ExecuteFPGASequence(0);WaitTillSequenceFinished();");
    CS_AddCommand("LoopStart:");
    CS_AddCommand("ExecuteFPGASequence(111);WaitTillSequenceFinished();");
    CS_AddCommand("GetInputBufferValue(Input, 0);");
    CS_AddCommand("Mul(ScaledInput, Input, 1.23);");
    CS_AddCommand("Add(Output, ScaledInput, 123);");
    CS_AddCommand("SetAD9858Frequency(Output);");
	//CS_AddCommand("JumpIfNotZero(ContinueExecution, LoopStart:);"); 
    //if this command is there, we can stop execution at this well defined point by calling CS_StopExecution. 
    // We can always terminate execution with CS_InterruptExecution(), but this is not a clean way to stop execution, as it does not allow the command system to finish at a point of its choosing.

    for (int i = 0; i < 3; i++) {
        CommandStatus status = CS_ExecuteSequence();
        if (status != CMD_SUCCESS) {
            xil_printf("Execution error: ", CS_GetErrorMessage(0));
            xil_printf("\n");
        }
        else {
            xil_printf("\nExecution complete.\n");
        }

      //  xil_printfsi("Last line number: ", GetLastCommandLineNumber());
        xil_printf("\n");
    }
}


void jump_test() {
    cout << "test if non-existence of label is detected. This should throw an error:" << endl;
   
    CS_AssembleSequence();

    CS_AddCommand("ExecuteFPGASequence(0);WaitTillSequenceFinished();");
    CS_AddCommand("ExecuteFPGASequence(111);WaitTillSequenceFinished();");
    CS_AddCommand("GetInputBufferValue(Input, 0);");
    CS_AddCommand("Mul(ScaledInput, Input, 1.23);");
    CS_AddCommand("Add(Output, ScaledInput, 123);");
    CS_AddCommand("SetAD9854Frequency(Output);");
    CS_AddCommand("JumpIfNotZero(ContinueExecution, LoopStart:);"); 
    //if this command is there, we can stop execution at this well defined point by calling CS_StopExecution. 
    // We can always terminate execution with CS_InterruptExecution(), but this is not a clean way to stop execution, as it does not allow the command system to finish at a point of its choosing.

        CommandStatus status = CS_ExecuteSequence();
        if (status != CMD_SUCCESS) {
            xil_printf("Execution error: ", CS_GetErrorMessage(0));
            xil_printf("\n");
        }
        else {
            xil_printf("\nExecution complete.\n");
        }

        //  xil_printfsi("Last line number: ", GetLastCommandLineNumber());
        xil_printf("\n");
    
}

int main()
{
	SwitchDebugMode(true);
    simple_test();
    test_AD9854VCO();
    comparison_test();
    bit_manipulation_test();
    jump_test();
	return 0;
}


#endif
