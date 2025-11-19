#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

// CProcMonGUIApp:
// See ProcMonGUI.cpp for the implementation of this class
//

class CProcMonGUIApp : public CWinAppEx
{
public:
    CProcMonGUIApp();

// Overrides
    virtual BOOL InitInstance() override;

// Implementation

    DECLARE_MESSAGE_MAP()
};

// The one and only CProcMonGUIApp object
inline CProcMonGUIApp theApp;