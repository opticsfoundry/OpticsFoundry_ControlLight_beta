#ifndef TESTSEQUENCE_H
#define TESTSEQUENCE_H

#include "QTCPIP.h"

bool InitializeSequencer(QTelnet *telnet);
bool ResetSystem();
bool ExecuteTestSequence();
bool CycleSequence();
bool CycleSequenceWithIndividualCommandUpdate();
bool StopCyclingButtonPressed();

#endif // TESTSEQUENCE_H
