
// ProcMonGUIDlg.h : header file
//

#pragma once

#include "ErrorDispatcher.h"
#include "processdll.h"
#include "Callbacks.h"


// CProcMonGUIDlg dialog
class CProcMonGUIDlg : public CDialog
{
// Construction
public:
	CProcMonGUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PROCMONGUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	void RefreshDriverStatus();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedInstallDriver();
	afx_msg void OnBnClickedStartDriver();
	afx_msg void OnBnClickedStopDriver();
	afx_msg void OnBnClickedUninstallDriver();
	afx_msg void OnBnClickedStartMonitor();
	afx_msg void OnBnClickedStopMonitor();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
