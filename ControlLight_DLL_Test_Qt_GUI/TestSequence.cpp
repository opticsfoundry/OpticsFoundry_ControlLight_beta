#include "TestSequence.h"
#include  "ControlLightAPI.h"
#include "QTCPIP.h"
#include "main.h"

#include <QMessageBox>
#include <QApplication>
#include <QFile>
//#include <QThread>
#include <unistd.h>
#include <QTime>
#include <QThread>

CControlLightAPI CA;

//static bool BlockButtons = false;


void MessageBox(QString aMessage) {
    QMessageBox msgBox;
    msgBox.setText(aMessage);
    msgBox.exec();
    TelnetTester->setStatusText(aMessage, true);
}



bool LoadControlHardwareInterface() {
    bool ControlHardwareInterfaceLoadedSuccessfully = false;

    bool success = CA.LoadFromJSONFile("D:\\Florian\\Firefly\\FireflyControl\\ControlLight_DLL_Test_Qt_GUI\\ControlHardwareConfig.json");
    if (!success) {
        MessageBox("Warning: Hardware configuration file did not load.");
        return false;
    }
    CA.Initialize();
    if (!CA.IsReady()) {
        MessageBox("Warning: Loading of hardware configuration file worked only partially.");
        return false;
    }
    ControlHardwareInterfaceLoadedSuccessfully = true;
    return true;
}

bool InitializeSequencer() {
    if (!CA.LoadDLL()) {
        MessageBox("TestSequence.cpp : InitializeSequencer() : couldn't connect to DLL.");
        return false;
    }
    if (!CA.Create()) {
        MessageBox("TestSequence.cpp : InitializeSequencer() : couldn't initialize MFC.");
        return false;
    }
    CA.Configure(/*DisplayErrors*/false);
    if (!LoadControlHardwareInterface()) {
        //MessageBox("Error loading hardware configuration file");
        bool success = CA.AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.0.109", 7, true, 0, 100000000, 2000000, false, true, true, true);
        if (!success) {
            MessageBox("Adding sequencer didn't work.");
            return false;
        }
        CA.AddDeviceAnalogOut16bit(0, 24, 4, true, -10, 10);
        CA.AddDeviceAnalogOut16bit(0, 552, 4, true, -10, 10);
        CA.AddDeviceDigitalOut(0, 1, 16);
        CA.AddDeviceDigitalOut(0, 2, 16);
        CA.AddDeviceAD9854(0, 232, 2, 300000000, 1, 1);
        CA.AddDeviceAD9858(0, 652, 1200000000, 1);
        CA.Initialize();
        if (!CA.IsReady()) {
            MessageBox("Warning: defining hardware configuration didn't work.");
            return false;
        }
    }
    return true;
}

bool CheckSequencer() {
    //TelnetTester->setStatusText(CA.UsingDLL() ? "Using Control.dll to connect to sequencer" : "Using ethernet connection to Control.exe to connect to sequencer", true);
    CA.Configure(/*DisplayCommandErrors*/ false);
    CA.SwitchDebugMode(/*On*/ true, "DebugSequencer"); //put debug mode to true if you want to debug sequence (see all the files created by the Visual Studio code, and the USB-serial port output of the ZYNQ FPGA). Usually leave this "false".
    if (!CA.IsReady()) {
        MessageBox("Control system not ready");
        return false;
    }
    return true;
}

void SaveInputDataToFile(QString filename, unsigned char* cbuffer, unsigned long buffer_length) {
    //save input data as ASCII table
    unsigned int* buffer = (unsigned int*)cbuffer;
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


static unsigned char* buffer = NULL;
bool TreatPhotodiodeData(bool take_photodiode_data, bool message) {
    bool success;
    unsigned long buffer_length = 0;
    unsigned long endTimeOfCycle =0;
    success = CA.WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length,endTimeOfCycle ,/*timeout_in_seconds*/ 20);
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
}





//
//Example of executing experimental sequence a single time.
//

//function called by GUI button
bool ExecuteTestSequence() {
    TelnetTester->setStatusText("Executing test sequence", true);
    //if (BlockButtons) return false;
    if (!CA.IsReady()) return false;
    //enter sequence programming mode
    CA.StartAssemblingSequence();

    CA.SwitchDebugMode(true, "DebugSequencer");
    if (!CA.IsReady()) {
        MessageBox("Not all sequencers connected");
        CA.Cleanup();
        return -1;
    }
    //test
    for (int i = 0; i < 10; i++) {
        //QString myQString = "Iteration: "+ QString::number(i);
        //TelnetTester->setStatusText(myQString, true);
        CA.StartAssemblingSequence();

        //start data acquisition. This is an example for a command for which we didn't yet provide a convenience function in the DLL.
        //In this somewhat convoluted manner one can achieve anything without API interface changes
        /*
    uint8_t ChannelNumber = 0;
    uint32_t NumberOfDataPoints = 1000;
    double DelayBetweenDataPoints_in_ms_in_ms = 0.02;
    CA.SetValueSerialDevice(0, 0, 0, (uint8_t*)&ChannelNumber, 8);
    CA.SetValueSerialDevice(0, 0, 1, (uint8_t*)&NumberOfDataPoints, 32);
    CA.SetValueSerialDevice(0, 0, 2, (uint8_t*)&DelayBetweenDataPoints_in_ms_in_ms, 64);
    CA.SetValueSerialDevice(0, 0, 3, (uint8_t*)&ChannelNumber, 8); //starts the acquisition
    */

        //this is the same command using a convenience function
        CA.SequencerStartAnalogInAcquisition(0, 0, 0, 0, 1000, 0.02);

        for (int j = 1; j < 100; j++) {
            CA.SetVoltage(0, 24, 10.0*j/100.0);
            uint16_t data = 0xffff;
            CA.SetValue(0, 1, 0, (uint8_t*)&data, 16);
            CA.Wait_ms(0.1);
            data = 0;
            CA.SetValue(0, 1, 0, (uint8_t*)&data, 16);
            CA.Wait_ms(0.1);
            double Frequency = 1000.0 * j / 100.0;
            CA.SetValue(0, 232, 0, (uint8_t*)&Frequency, 64);
        }
        CA.Wait_ms(10);
        CA.ExecuteSequence();
        bool running = false;
        unsigned long long DataPointsWritten = 0;
        CA.GetSequenceExecutionStatus(running, DataPointsWritten);

        unsigned long buffer_length = 0;
        unsigned long EndTimeOfCycle = 0;
        CA.WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, 10);

        QString myQString = "Iteration: "+ QString::number(i) + " Data received: "+ QString::number(buffer_length) + " 16-bit values";
        TelnetTester->setStatusText(myQString, true);

    }

    return true;
}


//function called by GUI button
bool ResetSystem() {
    TelnetTester->setStatusText("ResetSystem button clicked", true);
    ExecuteTestSequence();
    return true;
}

//function called by GUI button
bool CycleSequence() {
    TelnetTester->setStatusText("CycleSequence button clicked", true);
    ExecuteTestSequence();
    return true;
}


//function called by GUI button
bool StopCyclingButtonPressed() {
    TelnetTester->setStatusText("StopCycling button clicked", true);
    ExecuteTestSequence();
    return true;
}

bool CycleSequenceWithIndividualCommandUpdate(){
    TelnetTester->setStatusText("Cycle Sequence With Individual Command Update button clicked", true);
    ExecuteTestSequence();
    return true;
}
