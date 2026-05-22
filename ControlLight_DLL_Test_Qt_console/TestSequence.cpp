#include "TestSequence.h"
#include "ControlAPI.h"
#include "QTCPIP.h"
#include "main.h"

#include <QMessageBox>
#include <QApplication>
#include <QFile>
//#include <QThread>
#include <unistd.h>
#include <QTime>
#include <QThread>

CControlAPI CA;

bool BlockButtons = false;

void MessageBox(QString aMessage) {
    QMessageBox msgBox;
    msgBox.setText(aMessage);
    msgBox.exec();
    TelnetTester->setStatusText(aMessage, true);
}

bool InitializeSequencer(QTelnet *atelnet) {
    if (!CA.ConnectToLowLevelSoftware(atelnet, /*IP*/ "192.168.0.103")) { //Irene's place: 192.168.0.103 Odido: 192.168.1.155
        MessageBox("TestSequence.cpp : Initialize() : couldn't connect to low level software.");
        return false;
    }
    //CA.ConnectToSequencer(/*IP*/ "192.168.0.115"); //done automatically for now
    //CA.ConfigureControlAPI(/*DisplayCommandErrors*/ false);
    return true;
}

bool CheckSequencer() {
    TelnetTester->setStatusText(CA.UsingDLL() ? "Using Control.dll to connect to sequencer" : "Using ethernet connection to Control.exe to connect to sequencer", true);
    CA.ConfigureControlAPI(/*DisplayCommandErrors*/ false);
    CA.SwitchDebugMode(/*On*/ false, /*DebugTimingOn*/ false); //put debug mode to true if you want to debug sequence (see all the files created by the Visual Studio code, and the USB-serial port output of the ZYNQ FPGA). Usually leave this "false".
    CA.StopCycling();//just in case cycling wasn't correctly terminated before
    CA.StoreSequenceInMemory(false); //just in case StoreSequenceInMemory mode is active.
    if (!CA.CheckIfLowLevelSoftwareReady(/*timeout_in_seconds*/ 10)) {
        MessageBox("Low level software not ready");
        return false;
    }
    if (!CA.CheckIfSequencerReady(/*timeout_in_seconds*/ 10)) {
        MessageBox("FPGA sequencer not ready");
        return false;
    }
    return true;
}

//function called by GUI button
bool ResetSystem() {
    if (BlockButtons) return false;
    if (!CheckSequencer()) return false;
    //enter sequence programming mode
    CA.ProgramSequence();
    //put or ramp outputs to state needed for MOT
    CA.Command("SetFrequencyBlueMOTDPAOM(205);");
    CA.Command("SetFrequencyBlueMOTDPAOM(210);");
    long lineNrError;
    QString badCodeLine;
    if (CA.DidCommandErrorOccur(lineNrError, badCodeLine)) {
        //there was an error. Discard programmed sequence by going back to direct output mode.
        CA.SwitchToDirectOutputMode();
        //QString str = QString("Error at line number: %d in command line: %s").arg(lineNrError, badCodeLine);
        MessageBox("Error at line number: "+ QString::number(lineNrError) + " in command line: " + badCodeLine);
        return false;
    } else {
        CA.StartSequence(/*diplay_progress_dialog*/ true);
        CA.WaitTillSequenceEnds(/*timeout_in_seconds*/ 10);
        //now we are automatically back in direct output mode
        MessageBox("Reset system done");
    }
    return true;
}

void SaveInputDataToFile(QString filename, unsigned int* buffer, unsigned long buffer_length) {
    //save input data as ASCII table
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        for (unsigned long i=0;i<buffer_length;i++) {
            /*
            //for 32-bit data format (mostly for debugging)
            unsigned long buf_high = buffer[i] >> 16;
            unsigned long buf_low = buffer[i] & 0xFFFF;
            char out_buf[100];
            unsigned long data = buf_low & 0xFFF;
            sprintf(out_buf, "%u %u %u %u    %x %x\n", i, data, buf_high, buf_low, buf_high, buf_low);
            stream << out_buf;
            */
            //for usual, 16-bit data format
            char out_buf[100];
            sprintf(out_buf, "%u %u\n", i, buffer[i]);
            //sprintf(out_buf, "%u %u 0x%x\n", i, buffer[i], buffer[i]);
            stream << out_buf;
        }
        file.close();
    } else {
        MessageBox("Couldn't open file for writing");
    }
}

long ModifyCodeLineNr1 = 0;
long ModifyCodeLineNr2 = 0;
void TestSequence(bool RestartMOTLoading, bool take_photodiode_data) {
    //Starting point of sequence: end of MOT loading
    //end point of sequence: just after start of MOT loading
    // in this way we can do photodiod data treatment and update of next parameters during MOT loading time
    // Fixed duration of MOT loading time is guaranteed by FPGA when used in cycle mode

    if (RestartMOTLoading) {
        //drop all atoms, restart MOT, wait MOT loading time
        //ToDo: implement more commands than just SetFrequencyBlueMOTDPAOM(), write complete clock sequence.
        CA.Command("SetFrequencyBlueMOTDPAOM(200);Wait(100);SetFrequencyBlueMOTDPAOM(210);Wait(100);");
    }

    for (int i=0;i<2;i++) {
        CA.Command("SetFrequencyBlueMOTDPAOM(205);Wait(50);");
        CA.Command("SetFrequencyBlueMOTDPAOM(200);Wait(50);");
    }
    CA.Command("DoNothing();"); 
    //storing the line number of the last command enables you to replace it using ReplaceCommand(), see CycleSequenceWithIndividualCommandUpdate()
    //The command that can be replaced consists of as manny ;-separated sub-commands as you like.
    //In the example here, we inserted a placeholder command, doing nothing, and replace it later by a real command
    //The main application of this technique is to update the frequency of the clock laser probe beam.
    ModifyCodeLineNr1 = CA.GetLastCommandLineNumber();
    
    CA.Command("Wait(10);");
    //Message("Modify code line number: "+ QString::number(ModifyCodeLineNr));
    if (take_photodiode_data)  {
        //The FPGA input BRAM (2048 32-bit words) has to periodically copied into DDR by the ZYNQ's CPU (DMA is used for sequence output; too complicated to mesh two DMA transfers).
        //To avoid this CPU being stuck too long treating TCP/IP data and thereby overlooking the need to transfer the input buffer, we can ignore the TCP/IP communication (e.g. a request if sequence have finished by PC), we halt all TCP/IP communication for a while.
        //The TCP/IP data is not lost, it's just staying a bit longer in an input buffer, which is copied to DDR once we enabel TCP/IP communication again.
        //This is only a safety measure and maybe not needed.
        //GoBackInTime is used to start ignoring TCPIP a bit earlier than needed, just in case the last TCP/IP command takes a little bit of time to finish.
        CA.Command("GoBackInTime(100);IgnoreTCPIP(1);Wait(100);");
        CA.Command("WriteSystemTimeToInputMemory();"); //SystemTime is a 56-bit counter that counts the time since the FPGA was switched on in units of the FPGA clock period, i.e. usually 10ns.
        //Placing the System time (as zero padded 8*8-bit = 64-bit value) right into the data set that contains also the photodiode readout makes it possible to determine if the AQuRA clock runs have been executed in the desired time sequence or not.
        //Clock runs that don't meet the timing requirements should be ignored.
        //optionally we can leave some data in the input buffer, e.g. as markers to that can be used to check if the memory got corrupted, or to leave data about a specific sequence, see the use of ReplaceCommand() below to store cycle number in input memory.
        //this is useful for debugging, but not needed for normal operation
        CA.Command("WriteInputMemory(1, 1, 0);"); //WriteInputMemory(unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0)
        //The next command will be replaced in CycleSequenceWithIndividualCommandUpdate() with a command that writes the cycle number into the input buffer.
        //This is only done for debugging and demonstration.
        CA.Command("WriteInputMemory(2);");
        ModifyCodeLineNr2 = CA.GetLastCommandLineNumber(); 
        
        CA.Command("SwitchDebugLED(1);");
        CA.Command("AddMarker(1);"); //for debug: displays marker (here "1") on ZYNQ USB port output (use Termite or similar to see it)
        CA.Command("StartAnalogInAcquisition(0,1000,0.1);");// StartAnalogInAcquisition(/*channel_number*/ 2, /* number_of_datapoints */ 100, /* delay_between_datapoints_in_ms*/ 1)
        for (int i=0;i<50;i++) {
            CA.Command("SwitchSpareDigitalOut0(On);Wait(1);");
            CA.Command("SwitchSpareDigitalOut0(Off);Wait(1);");
        }
        /*
        CA.Wait(1);
        for (int i=0;i<8;i++) {
            char command[100];
            sprintf(command,  "StartAnalogInAcquisition(%u,100,0.1);Wait(11);", 3 + i);
            CA.Command(command);
        }
        */
        CA.Command("Wait(10);");
        CA.Command("WriteInputMemory(3);");
        CA.Command("WriteInputMemory(4);");

        CA.Command("SetFrequencyBlueMOTDPAOM(201);Wait(10);");// repump atoms (here just a dummy command)
        CA.Command("SetFrequencyBlueMOTDPAOM(202);Wait(10);");// repump atoms (here just a dummy command)
        CA.Command("SetFrequencyBlueMOTDPAOM(201);Wait(10);");// repump atoms (here just a dummy command)
        CA.Command("StartAnalogInAcquisition(3,1000, 0.1);Wait(200);");// StartAnalogInAcquisition(/*channel_number*/ 2, /* number_of_datapoints */ 100, /* delay_between_datapoints_in_ms*/ 1)
        //reaable TCP/IP communication. (Optional, it's reanabled automatically after the sequence ends)
        CA.Command("AddMarker(2);IgnoreTCPIP(0);");
        //CA.Command("Wait(5000);");
        CA.Command("WriteInputMemory(5);");
        CA.Command("WriteInputMemory(6);");
        CA.Command("SwitchDebugLED(0);");
    }
     CA.Command("SetFrequencyBlueMOTDPAOM(205);Wait(20);");
    CA.Command("SetFrequencyBlueMOTDPAOM(201);Wait(20);");
    
    //example of a linear ramp
    CA.Command("Ramp(\"SetFrequencyBlueMOTDPAOM\", LastValue, 210, 100, 1);"); //Ramp(unsigned char* output_name, double start_value /* use LAST_VALUE for last value */, double end_value, double ramp_time_in_ms, double timestep_in_ms = 0.1)
    CA.Command("WaitTillRampsEnd();");
    CA.Command("SetFrequencyBlueMOTDPAOM(210);");
    CA.Command("Wait(10);");
    CA.Command("SetFrequencyBlueMOTDPAOM(201);");
    //ToDo: add commands needed to start MOT loading
    //end point of sequence: just after start of MOT loading
}

bool DidCommandErrorOccur() {
    long lineNrError;
    QString badCodeLine;
    if (CA.DidCommandErrorOccur(lineNrError, badCodeLine)) {
        //there was an error. Discard programmed sequence by going back to direct output mode
        CA.SwitchToDirectOutputMode();
        //QString str = QString("Error at line number: %d in command line: %s").arg(lineNrError, badCodeLine);
        MessageBox("Error at line number: "+ QString::number(lineNrError) + " in command line: " + badCodeLine);
        return true;
    } else return false;
}

unsigned int* buffer = NULL;
bool TreatPhotodiodeData(bool take_photodiode_data, bool message) {
    bool success;
    unsigned long buffer_length = 0;
    if (take_photodiode_data) {
        success = CA.WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, /*timeout_in_seconds*/ 20);
        if (success && (buffer != NULL)) {
            QString myQString = "Data received: "+ QString::number(buffer_length) + " 16-bit values";
            if (message) MessageBox(myQString);
            else TelnetTester->setStatusText(myQString, true);
            //process input data
            SaveInputDataToFile("D:\\Florian\\Firefly\\FireflyControl\\input.dat", buffer, buffer_length);
            //freeing buffer is done in CA and shouldn't be done here if DLL is used.
            //delete[] Buffer;
        } else {
            if (message) MessageBox("No input data received");
            TelnetTester->setStatusText("No input data received", true);
        }
    } else CA.WaitTillSequenceEnds(/*timeout_in_seconds*/ 10);
    //now we are automatically back in direct output mode
}


//
//Example of executing experimental sequence a single time.
//

//function called by GUI button
bool ExecuteTestSequence() {
    if (BlockButtons) return false;
    if (!CheckSequencer()) return false;
    bool take_photodiode_data = true;
    //enter sequence programming mode
    CA.ProgramSequence();
    TestSequence(/*restart_MOT_loading*/ true, take_photodiode_data);
    if (DidCommandErrorOccur()) return false;
    CA.StartSequence(/*diplay_progress_dialog*/ true, /*timeout_in_seconds*/ 5);
    TreatPhotodiodeData(take_photodiode_data, /*display_message*/ true);
    MessageBox("Sequence ended");
    return true;
}


//
//Example of using a cyclic sequence with your own code in between (probably not appropriate for AQuRA, but good for debugging).
//

void InitializeCycleSequence() {
    //We provide the FPGA with the desired cycle time.
    //When we launch a sequence the FPGA will wait till at least that time has elapsed since the last time we did run
    if (!CheckSequencer()) return;
    //Here we define the cycle time. This resets the cycle.
    CA.SetPeriodicTrigger_ms(/*PeriodicTriggerPeriod_in_ms*/10000, /*PeriodicTriggerAllowedWaitTime_in_ms*/ 10000);
 }

bool StartCycleSequence(bool take_photodiode_data) {
    //enter sequence programming mode
    CA.ProgramSequence();
    //send sequence over
    TestSequence(/*restart_MOT_loading*/ true, take_photodiode_data);
    //check if sequence ok
    if (DidCommandErrorOccur()) return false;
    //if yes, execute sequence
    if (!CA.StartSequence(/*diplay_progress_dialog*/ true)) {
        MessageBox("Cycle Sequence couldn't start");
        return false;
    }
    return true;
}

void YourOwnCode() {
    //here you can run your own code, as long as you give control back soon enough to process photodiode data and tell system to start next run, before the cycle has expired.
    //If your code takes too long, we will get a longer time between two runs of a sequence than desired. The FPGA will notify us with a "missed cycle" message.
}

void EndCycleSequence(bool first_cycle, bool take_photodiode_data) {
    //The first cycle used an undefined MOT loading time. Make sure to ignore that data.
    TreatPhotodiodeData(take_photodiode_data, /*display_message*/ false);
    if (CA.GetPeriodicTriggerError()) {
        MessageBox("Cycle was triggered too late.");
    }
}

//function called by GUI button
bool CycleSequence() {
    if (BlockButtons) return false;
    bool take_photodiode_data = true;
    InitializeCycleSequence();
    for (int n =0; n<3;n++) { //here you could also use a while loop, that's stopped whenever you want
        StartCycleSequence(take_photodiode_data);
        YourOwnCode();
        EndCycleSequence(/*first_cycle*/ n == 0, take_photodiode_data);
    }
    CA.StopCycling();
    MessageBox("CycleSequence ended");
}



//
//Example of using a cyclic sequence with individual command updates. This is likely what we will use for AQuRA.
//

unsigned long PeriodicTriggerPeriod_in_ms = 0;
long PreviousLastCycleEndTime = 0;
long PreviousLastCycleStartPreTriggerTime = 0;
unsigned long PreviousFPGASystemTime = 0;
unsigned int NumberOfTimesOvertime = 0;
unsigned int* Buffer = NULL; //32-bit data
void GetCycleData(bool take_photodiode_data) {//}, long &LastCycleEndTime) {
    bool success;
    unsigned long BufferLength = 0;
    long CycleNumber;
    long LastCycleEndTime = 0; //ToDo: should be DWORD, i.e. unsigned long
    long LastCycleStartPreTriggerTime; //ToDo: should be DWORD, i.e. unsigned long
    bool CycleError;  //if true start of this sequence happend with too much delay, i.e. we jump over at least one cycle
    QString ErrorMessages;
    success = CA.GetCycleData(Buffer, BufferLength, CycleNumber, LastCycleEndTime, LastCycleStartPreTriggerTime, CycleError, ErrorMessages);
    if (!success) {
        MessageBox("GetCycleData: no data");
        return;
    }

    QFile file("D:\\Florian\\Firefly\\FireflyControl\\Data\\log.dat");
    if (file.open(QIODevice::Append)) {
        //FPGA SystemTime is in first 8 bytes thanks to CA.Command("WriteSystemTimeToInputMemory();"); command
        unsigned long FPGASystemTimeLow = Buffer[0];
        unsigned long FPGASystemTimeHigh = Buffer[1];
        unsigned long FPGASystemTime = ((unsigned long*)Buffer)[0]; // in units of the clock period, i.e. usually 10ns
        unsigned long ElapsedFPGASystemTime = FPGASystemTime - PreviousFPGASystemTime;
        if (ElapsedFPGASystemTime>PeriodicTriggerPeriod_in_ms*100000+10) NumberOfTimesOvertime++;
        PreviousFPGASystemTime = FPGASystemTime;
        QTextStream stream(&file);
        char out_buf[100];
        sprintf(out_buf, "%u %u %u %u %u %u %u %X %X %u %u", CycleNumber, BufferLength, LastCycleStartPreTriggerTime - PreviousLastCycleStartPreTriggerTime, LastCycleEndTime - PreviousLastCycleEndTime, LastCycleEndTime - LastCycleStartPreTriggerTime, (CycleError) ? 1 : 0, ElapsedFPGASystemTime, FPGASystemTimeHigh,FPGASystemTimeLow, NumberOfTimesOvertime, CA.GetNumberReconnects() );
        stream << out_buf << ErrorMessages << "\n";
        file.close();
        QString myQString = QString::fromUtf8(out_buf);
        TelnetTester->setStatusText(myQString, true);
        PreviousLastCycleEndTime = LastCycleEndTime;
        PreviousLastCycleStartPreTriggerTime = LastCycleStartPreTriggerTime;
    } else {
        MessageBox("Couldn't open log file for writing");
    }
    if (take_photodiode_data) {
        //ToDo: use 16-bit data format instead of 32-bit format in order to speed up data handling?
        if (success && (Buffer != NULL)) {
            //process input data
            char filename[100];
            sprintf(filename, "D:\\Florian\\Firefly\\FireflyControl\\Data\\input%04u.dat", CycleNumber);
            SaveInputDataToFile(filename, Buffer, BufferLength);
            //freeing buffer is done in CA and shouldn't be done here if DLL is used.
            //delete[] Buffer;
        } else {
            MessageBox("no input data received 2");
        }
    }
}

bool Cycling = false;



bool TerminateCycling(bool ok) {
    CA.StopCycling();
    CA.StoreSequenceInMemory(false);
    BlockButtons = false;
    TelnetTester->setStatusText("Finished cycling", true);
    Cycling = false;
    return ok;
}

//function called by GUI button
bool StopCyclingButtonPressed() {
    bool ok = true;
    if (Cycling) Cycling = false;
    else TerminateCycling(ok); //needed if QtControl newly started, but Visual Studio Control cycling
    if (!ok) MessageBox("Could not stop cycling");
}

//function called by GUI button
bool CycleSequenceWithIndividualCommandUpdate() {
    //CycleSequenceWithIndividualCommandUpdate() is similar to CycleSequence, but sequence only programmed once. 
    //From then on only desire command changes are scheduled for certain run numbers.
    //In AQuRA, the DDS frequency that controls the clock laser probe frequency is updated.
    if (BlockButtons) return false;
    //Below in this procedure, we use Sleep_ms(), which calls  QCoreApplication::processEvents().
    //If GUI buttons are pressed, processEvents() could create calls to routines that execute other experimental sequences, 
    //which would create a mess with the sequence executed here. 
    //To avoid this, we block these procedures, which is eseentially the same as blocking the buttons on the GUI.
    BlockButtons = true;
    if (!CheckSequencer()) {
        BlockButtons = false;
        return false;
    }
    //We tell the low level software to store the sequence, so that it can be used over and over again.
    CA.StoreSequenceInMemory(true); 
    bool take_photodiode_data = true;
    //Next we program the sequence.
    TestSequence(/*restart_MOT_loading*/ true, take_photodiode_data);
    //We determine the duration of the sequence, such that we can adapt the periodic trigger accordingly.
    double SequenceDuration_in_ms = 0;
    if (!CA.GetSequenceDuration(SequenceDuration_in_ms)) {
        MessageBox("Could not get sequence duration");
        return TerminateCycling(false);
    }
    double WaitTimeBetweenSequences_in_ms = 100; //This is the MOT loading time. If the DLL is used it can be down to about 100ms (for 8000 photodiode data points). In TCP/IP mode, it can be down to 300ms.
    PeriodicTriggerPeriod_in_ms = SequenceDuration_in_ms + WaitTimeBetweenSequences_in_ms;
    TelnetTester->setStatusText("Cycling with " + QString::number(PeriodicTriggerPeriod_in_ms) + " ms period of which " + QString::number(SequenceDuration_in_ms) + " ms sequence duration.", true);

    //WaitTimeBetweenSequences_in_ms is essentially the MOT loading time.
    //During this time the data of the last run is read out, analyzed and the frequency command for the next run(s) are sent.
    double MaxSequenceDuration_in_s = 20 + PeriodicTriggerPeriod_in_ms/1000; //for the detection of timeouts
    //Next we define the cycle time. This resets the cycle.
    //When we launch a sequence the FPGA will wait till at least PeriodicTriggerPeriod_in_ms has elapsed since the last time we did run.
    //If FPGA needs to wait longer than PeriodicTriggerAllowedWaitTime_in_ms, the CycleError flag is set high. That flag is retrieved with GetCycleData().
    CA.SetPeriodicTrigger_ms(PeriodicTriggerPeriod_in_ms, /*PeriodicTriggerAllowedWaitTime_in_ms*/ SequenceDuration_in_ms + WaitTimeBetweenSequences_in_ms);

    long NextCycleNumber = 0;
    long TimeTillNextCycleStart_in_ms = 0;
    long ReadoutPreTriggerTime_in_ms = 100; //time
    long QtReadoutPreTriggerTime_in_ms = 200;
    CA.ResetCycleNumber(); //this sets the next cycle number to 0
    //the display of a progress bar dialog is probably only useful for debugging and can be set to "false" in normal use
    //Note that if sequence is not ok, GetCycleData() will return error messages about which line of the sequence was faulty. //ToDo: check that this works.
    //The Visual Studio code will stop listening to TCPIP commands readout_pre_trigger_in_ms before periodic cycle ends, in order to be ready to immediatley read out data when cycle ends.
    //The Visual Studio code will send the next command sequence to the FPGA, soft_pre_trigger_in_ms before its estimation of the next FPGA periodic trigger event
    //In order for the Visual Studio's cycle start time and the FPGA's cycle start time to remain in sync:
    //at the end of each cycle, Visual Studio measures the end time and calculates the time at which the last periodic trigger must have happened.
    if (!CA.StartCycling(ReadoutPreTriggerTime_in_ms, /*soft_pre_trigger_in_ms*/ 200, /*TransmitOnlyDifferenceBetweenCommandSequenceIfPossible*/ true, /*diplay_progress_dialog*/ false)) { //this starts cycle 0
        if (DidCommandErrorOccur()) return TerminateCycling(false);
        MessageBox("CycleSequence couldn't start");
        return TerminateCycling(false);
    }
    Cycling = true;
    while (Cycling) { //here you could also use a while loop that's stopped whenever you want
        //here you can run your own code, as long as you give control back soon enough to process photodiode data and tell system to start next run, before the cycle has expired.
        //If your code takes too long, we will get a longer time between two runs of a sequence than desired. The FPGA will notify us with a "missed cycle" message.
        YourOwnCode();
        //ToDo (highly optional): get expected time till next cycle end using GetNextCycleTimeAndNumber() instead of GetNextCycleNumber(). Then sleep or do other things till ReadoutPreTriggerTime before that time. Only then start determining exact time of sequence end using TCP/IP communication.
        //wait for current cycle to finish
        long CycleNumber = 0;

        //The following while statement is there to reduce TCP/IP communication. If the amount of communication is no concern it can be skipped.
        bool UseOptionalWait = true;
        if (UseOptionalWait) {
            long ExpectedWaitTimeTillDataAvailableCommandToBeSent = SequenceDuration_in_ms - ReadoutPreTriggerTime_in_ms - QtReadoutPreTriggerTime_in_ms;
            while (ExpectedWaitTimeTillDataAvailableCommandToBeSent>20) { //(CycleNumber <= NextCycleNumber) {
                //check if cycling was aborted because of incorrect command
                if (!CA.IsCycling(/* timeout_in_seconds*/ MaxSequenceDuration_in_s)) {
                    if (DidCommandErrorOccur()) return TerminateCycling(false);
                    MessageBox("Error while cycling");
                    return TerminateCycling(false);
                }
                if (!CA.GetNextCycleStartTimeAndNumber(TimeTillNextCycleStart_in_ms, CycleNumber, /* timeout_in_seconds*/ MaxSequenceDuration_in_s)) {
                    MessageBox("Couldn't get next cycle number (1)");
                    return TerminateCycling(false);
                }
                ExpectedWaitTimeTillDataAvailableCommandToBeSent = TimeTillNextCycleStart_in_ms - ReadoutPreTriggerTime_in_ms - QtReadoutPreTriggerTime_in_ms;
                //instead of calling Sleep_ms() you can also execute your own code for up to ExpectedWaitTimeTillDataAvailableCommandToBeSent;
                if (ExpectedWaitTimeTillDataAvailableCommandToBeSent>20) Sleep_ms(ExpectedWaitTimeTillDataAvailableCommandToBeSent);
                //Sleep_ms(20);
            }
            NextCycleNumber = CycleNumber;
        }
        unsigned int Attempts = 0;
        const unsigned int MaxAttempts = 10;
        while ((!CA.DataAvailable(/* timeout_in_seconds*/ MaxSequenceDuration_in_s)) && (Attempts<MaxAttempts)) {
            Attempts++;
            Sleep_ms(10);
        }
        if (!(Attempts<MaxAttempts)) {
            if (!CA.IsCycling(/* timeout_in_seconds*/ MaxSequenceDuration_in_s)) {
                if (DidCommandErrorOccur()) return TerminateCycling(false);
                MessageBox("Error while cycling");
                return TerminateCycling(false);
            }
            MessageBox("Couldn't get data");
            return TerminateCycling(false);
        }
        GetCycleData(take_photodiode_data);
        while (CA.DataAvailable(/* timeout_in_seconds*/ MaxSequenceDuration_in_s)) {
            //The first cycle used an undefined MOT loading time. Ignore that run's data.
            GetCycleData(take_photodiode_data);//, LastCycleEndTime);//, /*timeout_in_seconds*/ MaxSequenceDuration_in_s);
            //NextCycleReadoutStartTime = LastCycleEndTime + SequenceDuration_in_ms - ReadoutPreTriggerTime;
        }
        YourOwnCode();
        //get number of next cycle (just in case too much time elapsed)
        if (!CA.GetNextCycleStartTimeAndNumber(TimeTillNextCycleStart_in_ms, CycleNumber, /* timeout_in_seconds*/ MaxSequenceDuration_in_s + 1000)) {
            MessageBox("Couldn't get next cycle number (2)");
            return TerminateCycling(false);
        }
        NextCycleNumber = CycleNumber;
        //Next you can specify updated commands for the next several cycles
        //You can schedule the replacement of several code lines within one cycle
        //A code line can consist of several ;-separated commands
        //You can store up to 32 replacement commands.
        //This limit can easily be increased by increasing const unsigned int ReplaceCommandListLength = 32; in ControlAPI.h in the Visual C++ Control code
        CA.ReplaceCommand(NextCycleNumber+1, ModifyCodeLineNr1, "SetFrequencyBlueMOTDPAOM(201);");
        char command[100];
        sprintf(command, "WriteInputMemory(%u);", NextCycleNumber+1);
        CA.ReplaceCommand(NextCycleNumber+1, ModifyCodeLineNr2, command);
        CA.ReplaceCommand(NextCycleNumber+3, ModifyCodeLineNr1, "SetFrequencyBlueMOTDPAOM(202);");
    }
    return TerminateCycling(true);
}



//
//debug tools and todo list
//

//If you have problems with TCPIP use this routine to intensely test the connection
bool TestTCPIP() {
  //  CA.SwitchDebugMode(/*On*/ true);

    long CycleNumber;
    double MaxSequenceDuration_in_s = 10;
    while (true) {
        TelnetTester->setStatusText("/", true, false);
        Sleep_ms(10); //wait for 100ms
        if (!CA.IsCycling(/* timeout_in_seconds*/ MaxSequenceDuration_in_s)) {
        }
        Sleep_ms(10); //wait for 100ms
        TelnetTester->setStatusText("|", true, false);
        if (!CA.CheckIfLowLevelSoftwareReady()) {
        }
        Sleep_ms(10); //wait for 100ms
        if (!CA.CheckIfSequencerReady()) {
        }
        TelnetTester->setStatusText("\\", true, false);
    }
}


//bool TestDirectOutputMode() {
    //ToDo (highly optional): find out why FPGA doesn't execute the short sequences used in direct output mode correctly
    //if (!CheckSequencer()) return false;
    //test a few commands in direct output mode
    //CA.SwitchToDirectOutputMode();
    //CA.Command("SetFrequencyBlueMOTDPAOM(210);");
    //if (!CA.WaitForDirectOutputModeCommandToFinish(/*timeout_in_seconds*/ 2)) return false;
    //CA.Command("SetFrequencyBlueMOTDPAOM(205);Wait(100);");
    //if (!CA.WaitForDirectOutputModeCommandToFinish(/*timeout_in_seconds*/ 2)) return false;
    //CA.Command("SetFrequencyBlueMOTDPAOM(200);Wait(1000);");
    //if (!CA.WaitForDirectOutputModeCommandToFinish(/*timeout_in_seconds*/ 2)) return false;
//}


//ToDo
//verify that cycling indeed cycles at specified period
//(optional) in Visual Studio code: progress bar
//(optional) first run in cycle doesn't deliver all data (first run is useless anyways, as MOT loaded too long)
//(highly optional) use existing trigger period (//If you have set the periodic trigger time already to the same value and you want the new cycle to start in sync with the old, i.e. not reset the cycle, then you can skip this step.)
//in Visual Studio code: if CAsyncSocket error reapears, replace CAsyncSocket with something that doesn't have bugs
//https://tangentsoft.com/wskfaq/articles/csocket.html
//https://medium.com/@tharunappu2004/creating-a-simple-tcp-server-in-c-using-winsock-b75dde86dd39
//https://learn.microsoft.com/en-us/windows/win32/winsock/complete-server-code
//https://tangentsoft.com/wskfaq/resources/libraries.html
//in Vivado (optionally):
//- pipelined design for memory prefetch instead of waiting for BRAM
//- increase input buffer capacity (16 bit; half output buffer capacity?)
