#pragma once

//#define _AFXDLL

#ifdef WIN32
#include <afxwin.h>
#endif
#include "std.h"
#include "network.h"



class CNetworkClient
#ifdef WIN32
	: public CObject
#endif
{
private:
	int mode;
	bool DebugOn;
	std::string DebugFileName;
public:
	CNetwork* Network;
	void DebugStop();
	void DebugStart(const std::string& Filename);
	void StopFastWrite();
	void StartFastWrite();
	bool FastWrite;
	void Flush();
	bool Ready();
	bool ConnectSocket(const std::string& host, unsigned int port, const std::string& SocketName);
	bool ReadDouble(double &Value);
	bool ReadBool(bool& Value);
	bool ReadInt(int& Value, double timeout_in_seconds = 5);
	bool ReadLong(long& Value);
	bool ReadInt64(unsigned long long& Value);
	bool SendCommand(const CString &comand);
	bool SendData(uint8_t* Data, unsigned long Size, bool SendReady = true);
	bool WriteDouble(double d);
	bool WriteInteger(long i);
	bool WriteBoolean(bool b);
	bool WriteString(CString s);
	bool WriteChar(char c);
	bool Command(CString comand, bool DontWaitForReady = false);
	bool AttemptCommand(CString CommandString, bool DontWaitForReady = false);
	bool GetCommand(CString &Command, double timeout_in_seconds = 5);
	CNetworkClient(int amode, bool aFastWrite = false);
	virtual ~CNetworkClient();
	void Debug(bool OnOff, const std::string& filename);
};
