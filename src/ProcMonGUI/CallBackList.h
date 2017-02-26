#pragma once

#include "resource.h"
#include "afxcmn.h"
#include "common.h"

// CCallBackGreyList dialog

class CCallBackListForm : public CDialog
{
	DECLARE_DYNAMIC(CCallBackListForm)

public:
    CCallBackListForm(CheckedProcesses checkedProcesses, CWnd* pParent = NULL);   // standard constructor
	virtual ~CCallBackListForm();

// Dialog Data
	enum { IDD = IDD_DIALOG_GREYLISTED_CALLBACK };

protected:
    virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnTimer( UINT_PTR nIDEvent );
    virtual void OnOK();
    virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
private:
    UINT_PTR m_Timer;
    ULONG m_CurrentTime; // in seconds
    CheckedProcesses m_checkedProcesses;
    CListCtrl m_uiGreyListedProcessesCtrl;
    CString m_uiTimeLeft;

    void InitGreyList(CheckedProcesses processes );
    afx_msg void OnBnClickedBtnCheckUncheckAll();

public:
    const CheckedProcesses& GetCheckedProcessesList() const;
};
