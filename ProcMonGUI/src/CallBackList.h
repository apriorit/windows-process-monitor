#pragma once

#include "resource.h"
#include "Common.h"

// CCallBackGreyList dialog

class CCallBackListForm : public CDialog
{
    DECLARE_DYNAMIC(CCallBackListForm)

public:
    CCallBackListForm(CheckedProcesses checkedProcesses, CWnd* parent = nullptr);   // standard constructor
    const CheckedProcesses& GetCheckedProcessesList() const;
    virtual ~CCallBackListForm();

// Dialog Data
    enum { IDD = IDD_DIALOG_GREYLISTED_CALLBACK };

protected:
    virtual BOOL OnInitDialog() override;
    virtual void DoDataExchange(CDataExchange* dataExchange) override;    // DDX/DDV support
    virtual void OnTimer(UINT_PTR nIDEvent);
    virtual void OnOK() override;
    virtual void OnCancel() override;
    DECLARE_MESSAGE_MAP()

private:
    void InitGreyList(CheckedProcesses processes);
    afx_msg void OnBnClickedBtnCheckUncheckAll();

private:
    UINT_PTR m_timer{};
    ULONG m_currentTime{}; // in seconds
    CheckedProcesses m_checkedProcesses{};
    CListCtrl m_uiGreyListedProcessesCtrl{};
    CString m_uiTimeLeft{};

    static constexpr ULONG kBlackListTimeout = 180;
};
