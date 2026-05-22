#pragma once

#include <string>
#include <chrono>

typedef std::chrono::high_resolution_clock Clock;
typedef Clock::duration Duration;
typedef std::chrono::time_point<Clock> Time;
double milliSeconds(const Duration& d);

#ifndef WIN32
	#define _T
	#define STD_STRING

	#ifndef STD_STRING
	#include "CStdString.h"
	typedef CStdString CString;
	#endif
#endif

#ifdef STD_STRING
	typedef std::string CString;
	#define CStringToStdString(str) str
#else
	#define CStringToStdString(str) std::string(str.GetString())
#endif

extern void ControlMessageBox(const std::string& text);
