// CallBackGreyList.cpp : implementation file
//

#include "CallBackList.h"

const UINT ID_TIMER_SECONDS = 0x1001;

// CCallBackGreyList dialog

IMPLEMENT_DYNAMIC(CCallBackListForm, CDialog)

CCallBackListForm::CCallBackListForm(CheckedProcesses checkedProcesses, CWnd* parent)
    : CDialog(CCallBackListForm::IDD, parent)
    , m_timer(0)
    , m_currentTime(kBlackListTimeout)
    , m_uiTimeLeft(_T(""))
    , m_checkedProcesses(checkedProcesses)
{
    m_uiTimeLeft.Format(_T("%u"), m_currentTime);
}

CCallBackListForm::~CCallBackListForm()
{
}

void CCallBackListForm::DoDataExchange(CDataExchange* dataExchange)
{
    CDialog::DoDataExchange(dataExchange);
    DDX_Control(dataExchange, IDC_LIST_GREY_PROCESSES_CALLBACK, m_uiGreyListedProcessesCtrl);
    DDX_Text(dataExchange, IDC_STATIC_GL_TIME, m_uiTimeLeft);
}

BOOL CCallBackListForm::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Start timer
    m_timer = SetTimer(ID_TIMER_SECONDS, 1000, nullptr);

    InitGreyList(m_checkedProcesses);

    return true;// return true unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CCallBackListForm, CDialog)
    ON_BN_CLICKED(IDC_BTN_CHECK_UNCHECK_ALL, &CCallBackListForm::OnBnClickedBtnCheckUncheckAll)
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CCallBackGreyList message handlers

// Timer Handler.
void CCallBackListForm::OnTimer(UINT_PTR nIDEvent)
{
    // Per second timer ticked.
    if (nIDEvent == ID_TIMER_SECONDS)
    {
        if (m_currentTime > 0)
        {
            --m_currentTime;
            m_uiTimeLeft.Format(_T("%u"), m_currentTime);
            UpdateData(false);
        }
        else
        {
            OnOK();
        }
    }

    CDialog::OnTimer(nIDEvent);
}

void CCallBackListForm::InitGreyList(CheckedProcesses processes)
{
    CRect rect{};
    m_uiGreyListedProcessesCtrl.GetClientRect(&rect);
    int colInterval = rect.Width() / 10;

    m_uiGreyListedProcessesCtrl.SetExtendedStyle(m_uiGreyListedProcessesCtrl.GetStyle() | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_uiGreyListedProcessesCtrl.InsertColumn(0, _T("Deny"), LVCFMT_LEFT, colInterval);
    m_uiGreyListedProcessesCtrl.InsertColumn(1, _T("PID"), LVCFMT_LEFT, colInterval);
    m_uiGreyListedProcessesCtrl.InsertColumn(2, _T("Image name"), LVCFMT_LEFT, 5 * colInterval);

    unsigned int i = 0;
    for (CheckedProcesses::const_iterator iter = processes.begin(); iter != processes.end(); ++iter, ++i)
    {
        CString strPID;
        strPID.Format(_T("%u"), iter->process.pid);

        //item 
        LVITEM li{};
        ZeroMemory(&li, sizeof(li));
        li.mask = LVIF_TEXT | LVIF_COLUMNS | LVIF_PARAM;
        li.pszText =  const_cast<decltype(li.pszText)>(_T(""));
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
    int count = m_uiGreyListedProcessesCtrl.GetItemCount();
    bool check = false;

    for (int i = 0; i < count; ++i)
    {
        m_uiGreyListedProcessesCtrl.SetCheck(i, check);
    }
}

const CheckedProcesses& CCallBackListForm::GetCheckedProcessesList() const
{
    return m_checkedProcesses;
}

void CCallBackListForm::OnOK()
{
    KillTimer(m_timer);
    UpdateData(true);

    int itemsCount = m_uiGreyListedProcessesCtrl.GetItemCount();

    for (int i = 0; i < itemsCount; ++i)
    {
        m_checkedProcesses.at(i).isDeny = m_uiGreyListedProcessesCtrl.GetCheck(i);
    }

    CDialog::OnOK();
}

void CCallBackListForm::OnCancel()
{
    OnOK();
}