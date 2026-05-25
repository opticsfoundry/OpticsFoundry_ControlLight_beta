// CNetworkClient.cpp: implementation of the CNetworkClient class.
//
//////////////////////////////////////////////////////////////////////
#include "NetworkClient.h"
#include "ControlAPI.h"
#include "std.h"
#ifdef WIN32
#include <tchar.h>
#endif
#include <thread>
#include <format>

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNetworkClient::CNetworkClient(int amode, bool aFastWrite)
{
	mode=amode;
	Network=NULL;
	FastWrite= aFastWrite;
	DebugOn=false;
	DebugFileName = "";
}

CNetworkClient::~CNetworkClient()
{
	if (Network) {
		if (mode==1) {
			CString help="Goodbye";
			SendCommand(help);
		}
		delete Network;
	}
}

void CNetworkClient::Debug(bool OnOff, const std::string& filename) {
	DebugFileName = filename;
	DebugOn = OnOff;
	if (Network) {
		if (!OnOff) Network->DebugStop();
		if (OnOff) Network->DebugStart(DebugFileName);
	}
}

bool CNetworkClient::ConnectSocket(const std::string& host, unsigned int port, const std::string& SocketName) {
	Network=new CNetwork();
	if (DebugOn) Network->DebugStart(DebugFileName);
	bool Connected=Network->ConnectSocket(host, port, SocketName);
	//we insist that a server is present on program start. If it isn't we ignore that server, gaining speed. The alternative would be to try to reconnect every time, which takes a second or so, if the sever is not present.
	//if you dont want this, comment out the next four lines
	if (!Connected) {
		delete Network;
		Network=NULL;
	}
	return Connected;
}

bool CNetworkClient::SendCommand(const CString& command) {
	if (!Network) {
		AddErrorMessage("CNetworkClient::SendCommand: not connected (command dropped: " + CStringToStdString(command) + ")");
		return false;
	}
	bool ok = true;
	if (mode == 1) {
		CString msg = _T("*") + command + _T("#");
		Network->SendMsg(msg);
	}
	else if (mode == 2) {
#ifdef STD_STRING
		unsigned int StrLength = command.size();
#else
		unsigned int StrLength = command.GetLength();
#endif
		if (StrLength > 255) return false;
		uint8_t length = StrLength;
		ok = Network->SendData(&length, 1);
#ifdef STD_STRING
		ok = Network->SendData((uint8_t*)(command.c_str()), length) && ok;
#else
		ok = Network->SendData((uint8_t*)(LPCTSTR)command, length) && ok;
#endif
	}
	else {  //mode == 3
		CString msg = command + _T("\n");
		Network->SendMsg(msg);
	}
	return ok;
}

bool CNetworkClient::WriteDouble(double d) {
#ifdef STD_STRING
	return SendCommand(std::format("{:8.7e}", d));
#else
	CString buf;
	buf.Format("%8.7e",d);
	return SendCommand(buf);
#endif
}

bool CNetworkClient::SendData(uint8_t* Data, unsigned long Size, bool SendReady) {
	if (!Network) {
		AddErrorMessage("CNetworkClient::SendData: not connected (dropping " + std::to_string(Size) + " bytes)");
		return false;
	}
	Network->FlushOutputBuffer();
	if (SendReady) {
		if (!Ready()) return false;
	}
	return Network->SendData(Data, Size);
}

bool CNetworkClient::WriteInteger(long i) {
#ifdef STD_STRING
	return SendCommand(std::format("{:8d}", i));
#else
	CString buf;
	buf.Format("%8i",i);
	return SendCommand(buf);
#endif
}

bool CNetworkClient::WriteBoolean(bool b) {
	if (mode == 1) {
		if (b) return SendCommand("TRUE");
		else return SendCommand("FALSE");
	} else {  // mode == 2 or 3
		if (b) return SendCommand("1");
		else return SendCommand("0");
	}
}

bool CNetworkClient::WriteString(CString s) {
  return SendCommand(s);
}

bool CNetworkClient::WriteChar(char c) {
	char tmp[2];
	sprintf(tmp, "%c", c);
	CString buf(tmp);
	return SendCommand(buf);
}

bool CNetworkClient::ReadDouble(double& Value)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf);
	//Value=atof(buf);
	// Convert CString ? const char* safely
#ifdef STD_STRING
	const char* str = buf.c_str();
#else
	CT2A narrow(buf);      // Converts to multibyte from Unicode if needed
	const char* str = narrow;
#endif
	char* endptr = nullptr;
	Value = std::strtod(str, &endptr);
	if (endptr == str) {
		//ControlMessageBox("CNetworkClient::ReadDouble : Conversion error: no digits found in ("+buf+").");
		return false;
	}
	else if (*endptr != '\0') {
		//ControlMessageBox("CNetworkClient::ReadDouble : Conversion error: leftover characters after number in (" + buf + ").");
		return false;
	}
	return ok;
}

bool CNetworkClient::ReadBool(bool& Value)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf);
	Value = buf == "1";
	return ok;
}

bool CNetworkClient::ReadInt(int& Value, double timeout_in_seconds)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf, timeout_in_seconds);
	//Value = atoi(buf);
	// Convert CString ? const char* safely
#ifdef STD_STRING
	const char* str = buf.c_str();
#else
	CT2A narrow(buf);      // Converts to multibyte from Unicode if needed
	const char* str = narrow;
#endif
	char* endptr = nullptr;
	Value = std::strtol(str, &endptr, 10);
	if (endptr == str) {
		//ControlMessageBox("CNetworkClient::ReadInt : Conversion error: no digits found in (" + buf + ").");
		return false;
	}
	else if (*endptr != '\0') {
		//ControlMessageBox("CNetworkClient::ReadInt : Conversion error: leftover characters after number in (" + buf + ").");
		return false;
	}
	return ok;
}

bool CNetworkClient::ReadLong(long& Value)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf);
	//Value = atoi(buf);
	// Convert CString ? const char* safely
#ifdef STD_STRING
	const char* str = buf.c_str();
#else
	CT2A narrow(buf);      // Converts to multibyte from Unicode if needed
	const char* str = narrow;
#endif
	char* endptr = nullptr;
	Value = std::strtol(str, &endptr, 10);
	if (endptr == str) {
		//ControlMessageBox("CNetworkClient::ReadLong : Conversion error: no digits found.");
		return false;
	}
	else if (*endptr != '\0') {
		//ControlMessageBox("CNetworkClient::ReadLong : Conversion error: leftover characters after number.");
		return false;
	}
	return ok;
}

bool CNetworkClient::ReadInt64(unsigned long long& Value)
{
	if (!Network) return false;
	CString buf;
	bool ok = GetCommand(buf);
	//Value = atoi(buf);

	// Convert CString ? const char* safely
#ifdef STD_STRING
	const char* str = buf.c_str();
#else
	CT2A narrow(buf);      // Converts to multibyte from Unicode if needed
	const char* str = narrow;
#endif
	char* endptr = nullptr;
	Value = std::strtoull(str, &endptr, 10);
	if (endptr == str) {
		//ControlMessageBox("CNetworkClient::ReadInt64 : Conversion error: no digits found.");
		return false;
	}
	else if (*endptr != '\0') {
		//ControlMessageBox("CNetworkClient::ReadInt64 : Conversion error: leftover characters after number.");
		return false;
	}
	return ok;
}

constexpr unsigned int MaxReconnectAttempts = 100;
bool CNetworkClient::Command(CString CommandString, bool DontWaitForReady) {
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptCommand(CommandString, DontWaitForReady))) {
		if (!Network) break;
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	if (attempts == MaxReconnectAttempts) AddErrorMessage("CNetworkClient::Command : Maximum reconnect attempts reached. Command failed: " + CStringToStdString(CommandString));
	return (attempts < MaxReconnectAttempts);
}

bool CNetworkClient::AttemptCommand(CString command, bool DontWaitForReady) {
	if (Network) Network->Flush();
	if (!SendCommand(command)) return false;
	if ((FastWrite) || (DontWaitForReady)) return true;
	if ((!Ready()) && (Network)) {
		//AddErrorMessage("CNetworkClient not Ready!\n(Command: "+command+")");
		return false;
	} else return true;
}

bool CNetworkClient::GetCommand(CString &Command, double timeout_in_seconds)
{
	if (!Network) return false;
	if (mode == 2) {
		return Network->ReceiveString(Command, timeout_in_seconds);
	} else return Network->GetMessage(Command, timeout_in_seconds, mode);
}

bool CNetworkClient::Ready() {
	//if (mode == 3) return true;
	if (Network) {
		CString buf;
		if (GetCommand(buf)) return buf=="Ready";
		else return false;
	} else return true;
}

void CNetworkClient::Flush()
{
	if (Network) Network->Flush();
}

void CNetworkClient::StartFastWrite()
{
	FastWrite=true;
}

void CNetworkClient::StopFastWrite()
{
	FastWrite=false;
	Flush();
}

void CNetworkClient::DebugStop() {
	if (Network) Network->DebugStop();
}
void CNetworkClient::DebugStart(const std::string& Filename) {
	if (Network) Network->DebugStart(Filename);
}
