#pragma once

class CMultiIO;

class CMultiWriteDevice 
{
public:
	bool ForceWriting;
	unsigned short MultiIOAddress;
	bool Enabled;
public:		
	void SwitchForceWritingMode(bool OnOff);
	virtual bool WriteToBus();
	void Enable(bool OnOff);
	virtual bool HasSomethingToWriteToBus();
	CMultiWriteDevice();
	~CMultiWriteDevice();
};
