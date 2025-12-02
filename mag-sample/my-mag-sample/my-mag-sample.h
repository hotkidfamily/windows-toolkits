
// my-mag-sample.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CmymagsampleApp:
// See my-mag-sample.cpp for the implementation of this class
//

class CmymagsampleApp : public CWinApp
{
public:
	CmymagsampleApp();

// Overrides
public:
	virtual BOOL InitInstance();

	int ExitInstance() override;

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CmymagsampleApp theApp;