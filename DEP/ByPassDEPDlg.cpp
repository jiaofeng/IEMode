
// ByPassDEPDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ByPassDEP.h"
#include "ByPassDEPDlg.h"
#include "afxdialogex.h"
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CByPassDEPDlg 对话框

CByPassDEPDlg::CByPassDEPDlg(CWnd* pParent /*=NULL*/)
  : CDialogEx(CByPassDEPDlg::IDD, pParent)
  , m_strPath(_T(""))
  , m_strFirefoxVersion(_T(""))
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CByPassDEPDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
  DDX_Control(pDX, IDC_EDIT_LOG, m_edtLog);
}

BEGIN_MESSAGE_MAP(CByPassDEPDlg, CDialogEx)
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_BN_CLICKED(IDC_BUTTON_HANDLE, &CByPassDEPDlg::OnBnClickedButtonHandle)
  ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CByPassDEPDlg::OnBnClickedButtonBrowse)
  ON_BN_CLICKED(IDC_BUTTON_AUTO_HANLE, &CByPassDEPDlg::OnBnClickedButtonAutoHanle)
END_MESSAGE_MAP()


// CByPassDEPDlg 消息处理程序

BOOL CByPassDEPDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
  //  执行此操作
  SetIcon(m_hIcon, TRUE);			// 设置大图标
  SetIcon(m_hIcon, FALSE);		// 设置小图标

  m_strPath = AfxGetApp()->m_lpCmdLine;
  if(m_strPath.Left(1) == "\"" ){
    m_strPath.Delete(0,1);
    m_strPath.Delete(m_strPath.GetLength()-1,1);
  }
  UpdateData(FALSE);
/*
  if (FindFirefox(false))
  {
    MessageBox(_T("处理成功"));
    EndDialog(0);
  }
  else
  {
    MessageBox(_T("处理失败"));
  }
*/
  return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CByPassDEPDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this); // 用于绘制的设备上下文

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // 使图标在工作区矩形中居中
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // 绘制图标
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialogEx::OnPaint();
  }
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CByPassDEPDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

void CByPassDEPDlg::OnBnClickedButtonHandle()
{
  UpdateData(TRUE);
  if (m_strPath.IsEmpty()) 
  {
    MessageBox(_T("请输入Firefox路径"));
    return;
  }
  HandleFile(m_strPath);
}

bool CByPassDEPDlg::FindFirefox(bool autoFind)
{
  const CString strPluginContainerName(_T("plugin-container.exe"));
  CString strFirefoxPath;
  CString strFirefoxDir;

  if(autoFind){
    m_edtLog.SetWindowText(NULL);

    static const int KEY_NAME_LEN = 255;
    HKEY  hKey;
    if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Mozilla\\Mozilla Firefox"), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
      return false;
    LONG lRet;
    DWORD dwIndex = 0;
    DWORD cbName = KEY_NAME_LEN;
    TCHAR szSubKeyName[KEY_NAME_LEN];
    while ((lRet = ::RegEnumKeyEx(hKey, dwIndex, szSubKeyName, &cbName, NULL,
      NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS)
    {
      // Do we have a key to open?
      if (lRet == ERROR_SUCCESS)
      {
        // Open the key and get the value

        HKEY hItem;
        CString strMainKeyName;
        strMainKeyName.Format(_T("%s\\Main"), szSubKeyName);
        if (::RegOpenKeyEx(hKey, strMainKeyName, 0, KEY_READ, &hItem) != ERROR_SUCCESS)
          continue;
        // Opened - look for "PathToExe"
        TCHAR szPath[KEY_NAME_LEN];
        DWORD dwSize = sizeof(szPath);
        DWORD dwType;


        if (::RegQueryValueEx(hItem, _T("PathToExe"), NULL, &dwType,
          (LPBYTE)&szPath, &dwSize) == ERROR_SUCCESS)
        {
          strFirefoxPath.SetString(szPath);
        }
        ::RegCloseKey(hItem);
      }
      dwIndex++;
      cbName = KEY_NAME_LEN;
    }
    ::RegCloseKey(hKey);
    if (dwIndex == 0) 
    {
      MessageBox(_T("注册表中没有Firefox安装信息, 请手动选择要处理的文件"));
      return false;
    }
  }else{
    strFirefoxPath = AfxGetApp()->m_lpCmdLine;
  }
  int nPos;
  nPos = strFirefoxPath.ReverseFind('\\');   
  strFirefoxDir = strFirefoxPath.Left(nPos + 1);

  if (!HandleFile(strFirefoxPath))
  {
    return false;
  }
  if (!HandleFile(strFirefoxDir + strPluginContainerName))
  {
    return false;
  }

  return true;
}

DWORD  WinExecAndWait32(LPCTSTR lpszAppPath,   // 执行程序的路径
  LPCTSTR lpParameters,  // 参数
  LPCTSTR lpszDirectory, // 执行环境目录
  DWORD dwMilliseconds)  // 最大等待时间, 超过这个时间强行终止
{
  SHELLEXECUTEINFO ShExecInfo = {0};
  ShExecInfo.cbSize    = sizeof(SHELLEXECUTEINFO);
  ShExecInfo.fMask    = SEE_MASK_NOCLOSEPROCESS;
  ShExecInfo.hwnd        = NULL;
  ShExecInfo.lpVerb    = NULL;
  ShExecInfo.lpFile    = lpszAppPath;        
  ShExecInfo.lpParameters = lpParameters;    
  ShExecInfo.lpDirectory    = lpszDirectory;
  ShExecInfo.nShow    = SW_HIDE;
  ShExecInfo.hInstApp = NULL;    
  ShellExecuteEx(&ShExecInfo);

  // 指定时间没结束
  if (WaitForSingleObject(ShExecInfo.hProcess, dwMilliseconds) == WAIT_TIMEOUT)
  {    // 强行杀死进程
    TerminateProcess(ShExecInfo.hProcess, 0);
    return 0;    //强行终止
  }

  DWORD dwExitCode;
  BOOL bOK = GetExitCodeProcess(ShExecInfo.hProcess, &dwExitCode);
  ASSERT(bOK);

  return dwExitCode;
}

HANDLE FindProcess(const CString& strProcessName)
{
  DWORD aProcesses[1024], cbNeeded, cbMNeeded;
  HMODULE hMods[1024];
  HANDLE hProcess;
  TCHAR szProcessName[MAX_PATH + 1];

  if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))  
    return NULL;
  for(int i=0; i< (int) (cbNeeded / sizeof(DWORD)); i++)
  {
    hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
    if (hProcess == NULL)
      continue;
    DWORD err = GetLastError();
    EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbMNeeded);
    GetModuleFileNameEx(hProcess, hMods[0], szProcessName, MAX_PATH);

    if (strProcessName.CompareNoCase(szProcessName) == 0) 
    {
      return hProcess;
    }
  }
  return NULL;
}

bool TerminateAllProcess(const CString& strProcessName)
{
  HANDLE hProcess = FindProcess(strProcessName);
  while(hProcess)
  {
    TerminateProcess(hProcess, 0);
    hProcess = FindProcess(strProcessName);
  }
  return true;
}

bool CByPassDEPDlg::HandleFile(const CString& strPath)
{
  CString strLine;
	HANDLE hProcess = FindProcess(strPath);
	if (hProcess)
	{
	  if (MessageBox(_T("需要关闭进程，是否继续?"), _T("关闭提示"), MB_YESNO) != IDYES)
	  {
		  return false;
	  }
	  while (!TerminateAllProcess(strPath)) 
	  {
		  if (MessageBox(_T("无法关闭进程"), _T("提示"), MB_RETRYCANCEL) != IDRETRY)
		  {
		    Log(_T("无法关闭火狐"));
		    return false;
		  }
	  }
	}
	CString strParam;
	strParam.Format(_T("/NXCOMPAT:NO \"%s\""), strPath);
	CString strCurrentPath = theApp.GetAppPath();
	DWORD nResult = WinExecAndWait32(strCurrentPath + _T("\\editbin.exe"), strParam, strCurrentPath, 10000);
	if (nResult == 0) 
	{
	  strLine = _T("处理成功!");
	}
	else
	{
	  strLine.Format(_T("处理失败!错误码%d"), nResult);  
	}
	Log(strLine + _T("\r\n"));
	return nResult == 0;
}


void CByPassDEPDlg::Log(const CString& strMsg)
{
  int len   =   m_edtLog.GetWindowTextLength();
  m_edtLog.SetSel(len,   len,   FALSE);
  m_edtLog.ReplaceSel(strMsg + _T("\r\n"));
}


void CByPassDEPDlg::OnBnClickedButtonBrowse()
{
  CFileDialog    dlg(TRUE, NULL,  _T("plugin-container.exe"), OFN_FILEMUSTEXIST, _T("plugin-container.exe|plugin-container.exe||"), NULL);
  //CFileDialog dlg(true, _T("firefox.exe|*.exe"), _T("firefox.exe"), OFN_FILEMUSTEXIST);
  if (dlg.DoModal() == IDOK) 
  {
    m_strPath = dlg.GetPathName();
  }
  UpdateData(FALSE);
}


void CByPassDEPDlg::OnBnClickedButtonAutoHanle()
{
  if (FindFirefox(true))
  {
    MessageBox(_T("处理成功"));
  }
  else
  {
    MessageBox(_T("处理失败"));
  }
}
