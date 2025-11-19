// ProcMonGUIDlg.h : header file
//

#pragma once

#include "ErrorDispatcher.h"
#include "Processdll.h"
#include "Callbacks.h"

// CProcMonGUIDlg dialog
class CProcMonGUIDlg : public CDialog
{
// Construction
public:
    CProcMonGUIDlg(CWnd* parent = nullptr);    // standard constructor

// Dialog Data
    enum { IDD = IDD_PROCMONGUI_DIALOG };

public:
    afx_msg void OnBnClickedInstallDriver();
    afx_msg void OnBnClickedStartDriver();
    afx_msg void OnBnClickedStopDriver();
    afx_msg void OnBnClickedUninstallDriver();
    afx_msg void OnBnClickedStartMonitor();
    afx_msg void OnBnClickedStopMonitor();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();

protected:
    virtual void DoDataExchange(CDataExchange* dataExchange) override;    // DDX/DDV support

// Implementation
protected:
    void RefreshDriverStatus();

    // Generated message map functions
    virtual BOOL OnInitDialog() override;
    afx_msg void OnSysCommand(UINT id, LPARAM param);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();

protected:
    HICON m_icon = nullptr;

    DECLARE_MESSAGE_MAP()
};
