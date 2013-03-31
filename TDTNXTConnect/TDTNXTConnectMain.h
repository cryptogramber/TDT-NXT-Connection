
// BrainConnectMain.h : main header file for the Brain Connect application
//

#pragma once
#pragma warning(disable: 4996)

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

// CConveyor2Leverv3App:
// See Conveyor 2Lever v3.cpp for the implementation of this class
//

class CBrainConnectApp : public CWinApp
{
public:
	CBrainConnectApp();
// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	DECLARE_MESSAGE_MAP()
};

extern CBrainConnectApp theApp;