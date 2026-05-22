#include "ControlAPI.h"
#include "network.h"
#include "std.h"
#include <cstring>
#include <iostream>
#include <thread>
#ifdef WIN32
#include <tchar.h>
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/sockios.h>
#endif
#endif

// Try to use CSocketException to see if the compiler sees it:
//void test_exception() {
//	CSocketException e;
//}

//#include <afxcplex.h>      // For MFC exception macros (TRY/CATCH)


#ifdef _WIN32
#ifndef _UNICODE
#include <strstream>
#endif
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CNetwork construction/destruction

CNetwork::CNetwork()
{	
	DebugFile=NULL;
	m_pSocket = NULL;
	LastMessage="";
}

CNetwork::~CNetwork()
{
	DebugStop();	
	DisconnectSocket();
}

void CNetwork::StoreLastMessage(const std::string& Message)
{
	if (DebugFile) (*DebugFile) << Message << endl;
}

void CNetwork::Flush()
{
	FlushInputBuffer();
	if (LastMessage != "") AddErrorMessage("CNetwork::Flush : Message " + CStringToStdString(LastMessage) + " flushed");
	LastMessage = "";
}

void CNetwork::DebugStart(const std::string& Filename) {
	DebugStop();
	DebugFile = new ofstream(Filename, ios::out);
}

void CNetwork::DebugStop() {
	if (DebugFile) {
		DebugFile->close();
		delete DebugFile;
		DebugFile = NULL;
	}
}

void CNetwork::DisconnectSocket()
{
	if (m_pSocket) {
		//AddErrorMessage("CNetwork::DisconnectSocket : Disconnecting socket ");

		StoreLastMessage("Disconnected");
#ifdef WIN32
		if (CAsyncSocket::LookupHandle(m_pSocket->m_hSocket, FALSE) == NULL) {
			// Avoid ASSERT by skipping Close and just invalidating the handle
			m_pSocket->m_hSocket = INVALID_SOCKET;
		}
		else {
			m_pSocket->Close();
		}
#else
		::close(m_socketfd);
#endif
		delete m_pSocket;
		m_pSocket = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetwork Operations

//From ChatGPT: a function to connect a CSocket with a timeout

#ifdef WIN32
bool ConnectWithTimeout(CSocket& sock, const std::string& host, unsigned int port, bool reconnect, bool showError = true, int timeoutSec = 2)
{
	// 1. WSAStartup (only needed once per app, but harmless if called repeatedly)
	static bool wsaInitialized = false;
	if (!wsaInitialized) {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			AddErrorMessage("WSAStartup failed", /*dothrow*/ false);
			return false;
		}
		wsaInitialized = true;
	}

	// 2. Create socket
	if (!sock.Create()) {
		AddErrorMessage("Failed to create socket", /*dothrow*/ false);
		return false;
	}

	// 3. Set to non-blocking mode
	u_long nonBlocking = 1;
	if (ioctlsocket(sock, FIONBIO, &nonBlocking) != 0) {
		sock.Close();
		AddErrorMessage("Failed to set non-blocking mode", /*dothrow*/ false);
		return false;
	}

	// 4. Prepare sockaddr_in
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host.c_str());  // Convert CString to const char*

	// 5. Begin connection
	int result = connect(sock, (SOCKADDR*)&addr, sizeof(addr));
	if (result == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
			sock.Close();
			AddErrorMessage("Immediate connection error", /*dothrow*/ false);
			return false;
		}

		// 6. Wait up to timeoutSec seconds
		fd_set writeSet;
		FD_ZERO(&writeSet);
		FD_SET(sock, &writeSet);

		TIMEVAL timeout = {};
		timeout.tv_sec = timeoutSec;

		int sel = select(0, nullptr, &writeSet, nullptr, &timeout);
		if (sel <= 0 || !FD_ISSET(sock, &writeSet)) {
			sock.Close();
			std::string message = std::format("Connection timed out or failed. IP = {}, port = {}.\n\nIf this is wrong, check the IP address given in ControlHardwareConfigFileCreator.py and run that script again.\n\nIf you don't use ControlHardwareConfig.json to configure control, check the IP given in ControlParam_SystemParamList.txt.", host, port);
			if (showError) AddErrorMessage(message);
			return false;
		}
	}

	// 7. Set back to blocking mode
	u_long blocking = 0;
	ioctlsocket(sock, FIONBIO, &blocking);

	//if (reconnect) {
	//	AddErrorMessage("CNetwork::ConnectWithTimeout : Reconnecting socket " + sock.m_hSocket);
	//	//  Re-attach socket to MFC for message handling
	//	sock.Attach(sock.m_hSocket); // rebinds socket to MFC message system
	//	sock.AsyncSelect(FD_READ | FD_WRITE | FD_CONNECT | FD_CLOSE);
	//}
	return true;
}
#else
#define BACKLOG 50
void sigchld_handler(int /*s*/)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}
#endif

bool CNetwork::ConnectSocket(const std::string& host, unsigned nPort, const std::string& SocketName, bool reconnect, int timeout_s)
{
	m_host = host;
	m_nPort = nPort;
	m_SocketName = SocketName;
#ifdef WIN32
	m_pSocket = new CSocket();
	/*if (!m_pSocket->Create()) { //socket creation is now done in ConnectWithTimeout; it's not done in CSocket::Connect
		int err = m_pSocket->GetLastError();
		AddErrorMessage(sd::format("Socket creation failed with error %d", err));
		delete m_pSocket;
		m_pSocket = nullptr;
		return false;
	}*/
	
	if (!ConnectWithTimeout(*m_pSocket, m_host, m_nPort, reconnect, (reconnect) ? false : true, timeout_s))
	{
	//if (!m_pSocket->Connect(m_host, m_nPort)) { //Standard CSocket::connect, which has a ~20s timeout
		//int err = m_pSocket->GetLastError();
		//AddErrorMessage(std::format("Socket connection to IP {}, port {} failed with error {}", host, nPort, err));
		delete m_pSocket;
		m_pSocket = nullptr;
		return false;
	}
	return true;
#else
	m_pSocket = nullptr;
	struct hostent* hostInfo = gethostbyname(m_host.c_str());
	if (hostInfo == NULL)
	{
		cerr << "Unknown host: " << host << endl;
		return false;
	}

	m_socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socketfd < 0)
	{
			cerr << "cannot create socket" << endl;
			return false;
	}

	struct sockaddr_in address;
	address.sin_family = hostInfo->h_addrtype;
	memcpy((char *) &address.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
	address.sin_port = htons(nPort);

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(m_socketfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	if (::connect(m_socketfd, (struct sockaddr*) &address, sizeof(address)) < 0)
	{
		cerr << "cannot connect to " << host << endl;
		return false;
	}
	cout << "Connected to " << m_host << endl;

	m_pSocket = new int;
	return true;
#endif
}

bool CNetwork::ResetConnection(unsigned long sleep_time) {
#ifdef WIN32
	// Keep socket infrastructure alive
	CAsyncSocket dummy;
	BOOL bDummyCreated = dummy.Create();
	DisconnectSocket();
	if (sleep_time > 0) this_thread::sleep_for(sleep_time*1ms);
	bool ret = Reconnect(/*maxRetries*/ 4,/*timeout_s*/0,/*delay_ms*/100);
	if (bDummyCreated)
		dummy.Close();
	return ret;
#else
	/// @todo
	return true;
#endif
}

void CNetwork::SendMsg(CString& strText)
{
	StoreLastMessage(">> " + CStringToStdString(strText));

	SendString(strText);
}

bool CNetwork::SendData(const uint8_t* Data, unsigned long Size)
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	StoreLastMessage(std::format(">> SendData {}", Size));
	unsigned long totalSent = 0;
	while (totalSent < Size) {
		int sent = 0;
#ifdef WIN32
		TRY
		{
			sent = m_pSocket->Send(Data + totalSent, Size - totalSent);
		}
		CATCH(CFileException, e)
		{
			AddErrorMessage("CNetwork::SendData : error sending data");
			return false;
		}
		END_CATCH
#else
		sent = ::send(m_socketfd, Data + totalSent, Size - totalSent, MSG_NOSIGNAL);
#endif
		if (sent <= 0) {
			AddErrorMessage("CNetwork::SendData : socket did not accept data");
			return false;
		}
		totalSent += sent;
	}
	return true;
}

bool CNetwork::SendString(const CString& str) {
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
#ifdef STD_STRING
	const char* psz = str.c_str();
#else
	CT2A conv(str);
	const char* psz = conv;
#endif
	return SendData(reinterpret_cast<const uint8_t*>(psz), (unsigned long)strlen(psz));
}

bool CNetwork::FlushOutputBuffer()
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;

	const unsigned long timeout_ms = 5000;
	Time start = Clock::now();

#ifdef WIN32
	if (!WaitForWrite(timeout_ms)) return false;

	int socketError = 0;
	int socketErrorSize = sizeof(socketError);
	SOCKET s = m_pSocket->m_hSocket;
	if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &socketErrorSize) == SOCKET_ERROR)
		return false;
	return socketError == 0;
#else
	while (milliSeconds(Clock::now() - start) < timeout_ms) {
#ifdef SIOCOUTQ
		int pendingBytes = 0;
		if (ioctl(m_socketfd, SIOCOUTQ, &pendingBytes) == 0 && pendingBytes == 0)
			return true;
#else
		return WaitForWrite(timeout_ms);
#endif
		Duration timeLeft = timeout_ms*1ms - (Clock::now() - start);
		if (!WaitForWrite(milliSeconds(timeLeft))) return false;
	}
	return false;
#endif
}

bool CNetwork::FlushInputBuffer()
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	const int kBufferSize = 4096;
	char tempBuffer[kBufferSize];

#ifdef WIN32
	SOCKET s = m_pSocket->m_hSocket;
	// Set the socket temporarily to non-blocking mode
	u_long nonBlocking = 1;
	ioctlsocket(s, FIONBIO, &nonBlocking);
#else
	int s = m_socketfd;
	int flags = fcntl(m_socketfd, F_GETFL, 0);
	flags = flags | O_NONBLOCK;
	fcntl(m_socketfd, F_SETFL, flags);
#endif


	int bytesRead = 0;
	do {
		bytesRead = ::recv(s, tempBuffer, kBufferSize, 0);
		if (bytesRead > 0) {
			// Data read and discarded
			tempBuffer[bytesRead + 1] = 0;
			StoreLastMessage("Flushed input buffer (" + std::string(tempBuffer) + ")");
			continue;
		}
		else if (bytesRead == 0) {
			// Connection closed
			break;
		}
		else {
#ifdef WIN32
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) {
				// No more data to read
				break;
			}
			else {
				Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
				return true;
				break;
			}
#else
			break;
#endif
		}
	} while (bytesRead > 0);

#ifdef WIN32
	// Restore socket to blocking mode
	nonBlocking = 0;
	ioctlsocket(s, FIONBIO, &nonBlocking);
#else
	flags = fcntl(m_socketfd, F_GETFL, 0);
	flags = flags & ~O_NONBLOCK;
	fcntl(m_socketfd, F_SETFL, flags);
#endif

	return true;
}

bool CNetwork::WaitForWrite(unsigned long timeout_ms) {
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	fd_set writeSet;
	FD_ZERO(&writeSet);
#ifdef WIN32
	SOCKET s = m_pSocket->m_hSocket;
#else
	int s = m_socketfd;
#endif
	FD_SET(s, &writeSet);
	timeval tv;
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;
#ifdef WIN32
	int result = select(0, nullptr, &writeSet, nullptr, &tv);
#else
	int result = select(s + 1, nullptr, &writeSet, nullptr, &tv);
#endif
	return (result > 0) && FD_ISSET(s, &writeSet);
}

bool CNetwork::WaitForRead(unsigned long timeout_ms) {
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	fd_set readSet;
	FD_ZERO(&readSet);
#ifdef WIN32
	SOCKET s = m_pSocket->m_hSocket;
#else
	int s = m_socketfd;
#endif
	FD_SET(s, &readSet);
	timeval tv;
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;
#ifdef WIN32
	int result = select(0, &readSet, nullptr, nullptr, &tv);
#else
	int result = select(s + 1, &readSet, nullptr, nullptr, &tv);
#endif
	return (result > 0) && FD_ISSET(s, &readSet);
}

bool CNetwork::ReceiveMsg(char end_character, bool WaitForStartCharacter, char start_character, double timeout_in_seconds)
{
	LastMessage = "";
	char in;
	Duration timeout = int(1000.*timeout_in_seconds)*1ms;
	if (WaitForStartCharacter) {
		in = '@';
		Time start = Clock::now();
		while ((WaitForStartCharacter) && ((Clock::now() - start) < timeout)) {
			unsigned long timeLeft = milliSeconds(timeout - (Clock::now() - start));
#ifdef WIN32
			if (!WaitForRead(timeLeft)) break;
			int nRead = m_pSocket->Receive(&in, 1);
#else
			int nRead = recv(m_socketfd, &in, 1, 0);
#endif
			if (nRead != 1) {
				AddErrorMessage("CNetwork::ReceiveMsg : error receiving data");
				return false;
			}
			if (in != start_character) {
				AddErrorMessage(std::format("CNetwork::ReceiveMsg :: start_character ({}) expected, but {} received.", start_character, in));
				return false;
			}
			else WaitForStartCharacter = false;
		}
	}
	in='@';
	Time StartTime=Clock::now();
	ReceiveString(LastMessage, timeout_in_seconds, end_character);
	StoreLastMessage("<< " + CStringToStdString(LastMessage));
	return true;
	//AddErrorMessage("ReceiveMsg message received\n( " + LastMessage + ")");
}

bool CNetwork::ReceiveString(CString& outStr, double timeout_in_seconds, char endChar)
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/100);
	if (!m_pSocket) return false;
#ifdef STD_STRING
	outStr.clear();
#else
	outStr.Empty();
#endif
	char ch = 0;
	Duration timeout = int(1000.*timeout_in_seconds)*1ms;
	Time start = Clock::now();
	while ((Clock::now() - start) < timeout) {
		unsigned long timeLeft = milliSeconds(timeout - (Clock::now() - start));
#ifdef WIN32
		if (!WaitForRead(timeLeft)) break;
		int nRead = m_pSocket->Receive(&ch, 1);
#else
		int nRead = recv(m_socketfd, &ch, 1, 0);
#endif
		if (nRead != 1) {
			AddErrorMessage("CNetwork::ReceiveString : error receiving data");
			return false;
		}
		if (ch == endChar) break;
		outStr += ch;
	}
	return true;
}

bool CNetwork::GetMessage(CString& Message, double timeout_in_seconds, int mode)
{
	if (LastMessage == "") {
		Time StartTime = Clock::now();
		Duration timeout = int(timeout_in_seconds*1000.)*1ms;
		while (((Clock::now() - StartTime) < timeout) && (LastMessage == "")) {
			if (mode == 1) ReceiveMsg(/*char end_character = */ '#', /*bool WaitForStartCharacter =*/ true, /*char start_character =*/ '*', timeout_in_seconds);
			else ReceiveMsg(/*char end_character =*/ '\n', /*bool WaitForStartCharacter = */false, /*char start_character =*/ '*', timeout_in_seconds);
		}
	}
	Message = LastMessage;
	LastMessage = "";
	return Message != "";
}

bool CNetwork::ReceiveData(uint8_t* buffer, unsigned long size, unsigned long timeout_ms)
{
	if (!m_pSocket)	Reconnect(/*maxRetries*/ 0,/*timeout_s*/1,/*delay_ms*/0);
	if (!m_pSocket) return false;
	unsigned long totalRead = 0;
	Time start = Clock::now();
	while (totalRead < size && (milliSeconds(Clock::now() - start) < timeout_ms)) {
		Duration timeLeft = timeout_ms*1ms - (Clock::now() - start);
		int nRead = 0;
#ifdef WIN32
		if (!WaitForRead(milliSeconds(timeLeft))) break;
		TRY
		{
			nRead = m_pSocket->Receive(buffer + totalRead, size - totalRead);
		}
			CATCH(CFileException, e)
		{
			AddErrorMessage("CNetwork::ReceiveData : error receiving data 1");
			return false;
		}
		END_CATCH
#else
		nRead = recv(m_socketfd, buffer + totalRead, size - totalRead, 0);
#endif
		if (nRead <= 0) {
			AddErrorMessage("CNetwork::ReceiveData : error receiving data 2");
			return false; // Disconnected or error
		}
		totalRead += nRead;
	}
	return (totalRead == size);
}

bool CNetwork::IsConnected() const {
#ifdef WIN32
	return (m_pSocket && m_pSocket->m_hSocket != INVALID_SOCKET);
#else
	/// @todo
	return true;
#endif
}

bool CNetwork::Reconnect(int maxRetries, int timeout_s, unsigned long delay_ms) {
#ifdef WIN32
	// Keep socket infrastructure alive
	CAsyncSocket dummy;
	BOOL bDummyCreated = dummy.Create();
	DisconnectSocket();
	int tries = 0;
	while (tries < (maxRetries + 1)) {
		if (ConnectSocket(m_host, m_nPort, m_SocketName, /*reconnect*/true,/*timeout_s*/timeout_s)) {
			StoreLastMessage("Reconnected");
			if (bDummyCreated)
				dummy.Close(); // Close dummy socket if it was created
			return true;
		}
		tries++;
		this_thread::sleep_for(delay_ms*1ms);
	}
	if (bDummyCreated)
		dummy.Close(); // Close dummy socket if it was created
#else
	/// @todo
#endif
	return false;
}


/*
bool CNetwork::Reconnect(int maxRetries, unsigned long delay_ms) {
	DisconnectSocket();
	int tries = 0;
	while (tries < maxRetries) {
		if (ConnectSocket(m_host, m_nPort, m_SocketName))
			return true;
		tries++;
		Sleep(delay_ms);
	}
	return false;
}
*/

/*
bool CNetwork::SendDataWithRetry(const uint8_t* data, unsigned long size, int maxRetries, unsigned long delay_ms) {
	int tries = 0;
	while (tries < maxRetries) {
		TRY{
			if (SendData(data, size))
				return true;
		}
			CATCH(CSocketException, e) {
			// Optionally: log error or print here
			e->Delete();
		}
		END_CATCH
			tries++;
		if (!Reconnect(maxRetries, delay_ms))
			break; // Could not reconnect
		Sleep(delay_ms);
	}
	return false;
}

bool CNetwork::ReceiveDataWithRetry(uint8_t* buffer, unsigned long size, unsigned long timeout_ms, int maxRetries, unsigned long delay_ms) {
	int tries = 0;
	while (tries < maxRetries) {
		TRY{
			if (ReceiveData(buffer, size, timeout_ms))
				return true;
		}
			CATCH(CSocketException, e) {
			e->Delete();
		}
		END_CATCH
			tries++;
		if (!Reconnect(maxRetries, delay_ms))
			break;
		Sleep(delay_ms);
	}
	return false;
}
*/
