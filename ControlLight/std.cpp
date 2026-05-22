#include "std.h"

#ifdef WIN32
#define _AFXDLL
#include <afxwin.h>
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#include "ControlAPI.h"

#include <string>
#include <iostream>

using namespace std;

double milliSeconds(const Duration& d)
{
	return chrono::duration_cast<std::chrono::duration<double>>(d).count() * 1000.;
}

void ControlMessageBox(const std::string& text) {
#ifdef WIN32
#ifdef _AFXDLL
	if (DisplayErrors) AfxMessageBox(text.c_str(), MB_OK, 0);
#else
	if (DisplayErrors) MessageBox(NULL, text.c_str(), L"Control DLL", MB_OK);
#endif
#else
	cerr << "MESSAGE BOX: " << text << endl;
#endif
}
