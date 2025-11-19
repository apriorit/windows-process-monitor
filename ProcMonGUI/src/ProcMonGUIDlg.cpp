// ProcMonGUIDlg.cpp : implementation file
//

#include "ProcMonGUI.h"
#include "ProcMonGUIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    enum { IDD = IDD_ABOUTBOX };

    protected:
    virtual void DoDataExchange(CDataExchange* dataExchange) override;    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* dataExchange)
{
    CDialog::DoDataExchange(dataExchange);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// CProcMonGUIDlg dialog

CProcMonGUIDlg::CProcMonGUIDlg(CWnd* parent)
    : CDialog(CProcMonGUIDlg::IDD, parent)
{
    m_icon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProcMonGUIDlg::DoDataExchange(CDataExchange* dataExchange)
{
    CDialog::DoDataExchange(dataExchange);
}

BEGIN_MESSAGE_MAP(CProcMonGUIDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_INSTALL_DRIVER, &CProcMonGUIDlg::OnBnClickedInstallDriver)
    ON_BN_CLICKED(IDC_START_DRIVER, &CProcMonGUIDlg::OnBnClickedStartDriver)
    ON_BN_CLICKED(IDC_STOP_DRIVER, &CProcMonGUIDlg::OnBnClickedStopDriver)
    ON_BN_CLICKED(IDC_UNINSTALL_DRIVER, &CProcMonGUIDlg::OnBnClickedUninstallDriver)
    ON_BN_CLICKED(IDC_START_MONITOR, &CProcMonGUIDlg::OnBnClickedStartMonitor)
    ON_BN_CLICKED(IDC_STOP_MONITOR, &CProcMonGUIDlg::OnBnClickedStopMonitor)
    ON_BN_CLICKED(IDOK, &CProcMonGUIDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CProcMonGUIDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

// CProcMonGUIDlg message handlers

BOOL CProcMonGUIDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* sysMenu = GetSystemMenu(false);
    if (sysMenu)
    {
        BOOL nameIsValid = false;
        CString strAboutMenu{};
        nameIsValid = strAboutMenu.LoadString(IDS_ABOUTBOX);

        ASSERT(nameIsValid);

        if (!strAboutMenu.IsEmpty())
        {
            sysMenu->AppendMenu(MF_SEPARATOR);
            sysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_icon, true);     // Set big icon
    SetIcon(m_icon, false);    // Set small icon

    // TODO: Add extra initialization here
    RefreshDriverStatus();

    return true;  // return TRUE  unless you set the focus to a control
}

void CProcMonGUIDlg::OnSysCommand(UINT id, LPARAM param)
{
    if ((id & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(id, param);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CProcMonGUIDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);

        CRect rect{};
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_icon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CProcMonGUIDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_icon);
}

void CProcMonGUIDlg::OnBnClickedInstallDriver()
{
    // TODO: Add your control notification handler code here
    ErrorDispatcher::Dispatch(Install());
    RefreshDriverStatus();
}

void CProcMonGUIDlg::OnBnClickedStartDriver()
{
    // TODO: Add your control notification handler code here
    ErrorDispatcher::Dispatch(Start());
    RefreshDriverStatus();
}

void CProcMonGUIDlg::OnBnClickedStopDriver()
{
    // TODO: Add your control notification handler code here
    ErrorDispatcher::Dispatch(Stop());
    RefreshDriverStatus();
}

void CProcMonGUIDlg::OnBnClickedUninstallDriver()
{
    // TODO: Add your control notification handler code here
    ErrorDispatcher::Dispatch(Uninstall());
    RefreshDriverStatus();
}

void CProcMonGUIDlg::OnBnClickedStartMonitor()
{
    // TODO: Add your control notification handler code here
    ErrorDispatcher::Dispatch(Init(Callbacks::ListCallback));
}

void CProcMonGUIDlg::OnBnClickedStopMonitor()
{
    // TODO: Add your control notification handler code here
    ErrorDispatcher::Dispatch(Deinit());
}

void CProcMonGUIDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    OnOK();
}

void CProcMonGUIDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    OnCancel();
}

void CProcMonGUIDlg::RefreshDriverStatus()
{
    int status = 0;
    ErrorDispatcher::Dispatch(GetStatus(&status));

    DriverStatus::Type driverStatus = static_cast<DriverStatus::Type>(status);
    if (driverStatus == DriverStatus::Type::Running)
    {
        SetDlgItemText(IDC_LABEL_INFO, _T("Status: Driver is started"));
    }
    else if (driverStatus == DriverStatus::Type::NotInstalled)
    {
        SetDlgItemText(IDC_LABEL_INFO, _T("Status: Drver is not installed"));
    }
    else if (driverStatus == DriverStatus::Type::Installed)
    {
        SetDlgItemText(IDC_LABEL_INFO, _T("Status: Driver is installed"));
    }
}
