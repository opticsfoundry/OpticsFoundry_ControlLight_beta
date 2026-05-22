#include "ControlAPI.h"
#include "CDevice.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CDevice::CDevice(CDeviceSequencer* _MySequencer, unsigned long _MyAddress, const std::string _MyType) {
	// Constructor implementation
	MySequencer = _MySequencer;
	MyAddress = _MyAddress;
	MyType = _MyType;
	_ErrorOccured = false;
}

void CDevice::NotifyError(const std::string& ErrorMessage, bool dothrow) {
	_ErrorOccured = true;
	AddErrorMessage(ErrorMessage, dothrow);
}

void CDevice::ClearError() {
	// Clear the error state
	_ErrorOccured = false;
}
