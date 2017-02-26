// CallBackGreyList.cpp : implementation file
//

#include "stdafx.h"
#include "CallBackList.h"

const UINT ID_TIMER_SECONDS = 0x1001;

// CCallBackGreyList dialog

IMPLEMENT_DYNAMIC(CCallBackListForm, CDialog)

CCallBackListForm::CCallBackListForm(CheckedProcesses checkedProcesses, CWnd* pParent /*=NULL*/)
	: CDialog(CCallBackListForm::IDD, pParent)
    , m_Timer(0)
    , m_CurrentTime(180)
    , m_uiTimeLeft(_T(""))
    ,m_checkedProcesses(checkedProcesses)
{
    m_uiTimeLeft.Format(_T("%u"),m_CurrentTime);
}

CCallBackListForm::~CCallBackListForm()
{
}

void CCallBackListForm::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_GREY_PROCESSES_CALLBACK, m_uiGreyListedProcessesCtrl);
    DDX_Text(pDX, IDC_STATIC_GL_TIME, m_uiTimeLeft);
}

BOOL CCallBackListForm::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Start timer
    this->m_Timer = SetTimer(ID_TIMER_SECONDS, 1000, 0);

    InitGreyList(m_checkedProcesses);

    return TRUE;// return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CCallBackListForm, CDialog)   
    ON_BN_CLICKED(IDC_BTN_CHECK_UNCHECK_ALL, &CCallBackListForm::OnBnClickedBtnCheckUncheckAll)
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CCallBackGreyList message handlers

// Timer Handler.
void CCallBackListForm::OnTimer( UINT_PTR nIDEvent )
{
    // Per second timer ticked.
    if( nIDEvent == ID_TIMER_SECONDS )
    {
        if (m_CurrentTime > 0)
        {
            --m_CurrentTime;
            m_uiTimeLeft.Format(_T("%u"), m_CurrentTime);
            this->UpdateData(FALSE);
        }
        else
        {
            OnOK();
        }
    }
    CDialog::OnTimer(nIDEvent);
}


void CCallBackListForm::InitGreyList(CheckedProcesses processes )
{
    CRect rect;
    m_uiGreyListedProcessesCtrl.GetClientRect(&rect);
    int nColInterval = rect.Width()/10;

    m_uiGreyListedProcessesCtrl.SetExtendedStyle(m_uiGreyListedProcessesCtrl.GetStyle()|LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
    m_uiGreyListedProcessesCtrl.InsertColumn(0, _T("Deny"), LVCFMT_LEFT, nColInterval);
    m_uiGreyListedProcessesCtrl.InsertColumn(1, _T("PID"), LVCFMT_LEFT, nColInterval);
    m_uiGreyListedProcessesCtrl.InsertColumn(2, _T("Image name"), LVCFMT_LEFT, 5*nColInterval);

    unsigned int i = 0;
    for (CheckedProcesses::const_iterator iter = processes.begin(); iter != processes.end(); ++iter, ++i)
    {
        CString strPID;
        strPID.Format(_T("%u"),iter->process.pid);

        //item 
        LVITEM li;
        ZeroMemory(&li, sizeof(li));
        li.mask = LVIF_TEXT | LVIF_COLUMNS | LVIF_PARAM;
        li.pszText =  _T("");
        li.iItem = i;
        li.iSubItem = 0;

        m_uiGreyListedProcessesCtrl.InsertItem(&li);

        m_uiGreyListedProcessesCtrl.SetItemText(i, 1, strPID);
        m_uiGreyListedProcessesCtrl.SetItemText(i, 2, iter->process.imageName);

        m_uiGreyListedProcessesCtrl.SetCheck(i, iter->isDeny);
    }  

}

void CCallBackListForm::OnBnClickedBtnCheckUncheckAll()
{
    int nCount = m_uiGreyListedProcessesCtrl.GetItemCount();
    BOOL fCheck = FALSE;

    for (int i = 0; i < nCount; ++i)
    {
        m_uiGreyListedProcessesCtrl.SetCheck(i, fCheck);
    }

}

const CheckedProcesses& CCallBackListForm::GetCheckedProcessesList() const
{
    return m_checkedProcesses;
}

void CCallBackListForm::OnOK()
{
    this->KillTimer(m_Timer);
    this->UpdateData(TRUE);

    int nItemsCount = m_uiGreyListedProcessesCtrl.GetItemCount();

    for (int i = 0; i < nItemsCount; ++i)
    {
        m_checkedProcesses.at(i).isDeny = m_uiGreyListedProcessesCtrl.GetCheck(i) != FALSE;
    }

    CDialog::OnOK();
}

void CCallBackListForm::OnCancel()
{
    OnOK();
}