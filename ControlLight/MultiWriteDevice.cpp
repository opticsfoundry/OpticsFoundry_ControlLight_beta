// MultiWriteDevice.cpp: implementation of the CMultiWriteDevice class.
//
//////////////////////////////////////////////////////////////////////

#include "MultiWriteDevice.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMultiWriteDevice::CMultiWriteDevice()
{
	ForceWriting=false;	
	Enabled=true;
	MultiIOAddress=0;
}

CMultiWriteDevice::~CMultiWriteDevice()
{

}

void CMultiWriteDevice::SwitchForceWritingMode(bool OnOff) {
	ForceWriting=OnOff;	
}

bool CMultiWriteDevice::WriteToBus()
{	
	return true;
}

bool CMultiWriteDevice::HasSomethingToWriteToBus() {
	return false;
}

void CMultiWriteDevice::Enable(bool OnOff)
{
	Enabled=OnOff;
}