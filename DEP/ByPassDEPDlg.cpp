
// ByPassDEPDlg.cpp : ʵ���ļ�
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


// CByPassDEPDlg �Ի���

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


// CByPassDEPDlg ��Ϣ�������

BOOL CByPassDEPDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  // ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
  //  ִ�д˲���
  SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
  SetIcon(m_hIcon, FALSE);		// ����Сͼ��

  m_strPath = AfxGetApp()->m_lpCmdLine;
  if(m_strPath.Left(1) == "\"" ){
    m_strPath.Delete(0,1);
    m_strPath.Delete(m_strPath.GetLength()-1,1);
  }
  UpdateData(FALSE);
/*
  if (FindFirefox(false))
  {
    MessageBox(_T("����ɹ�"));
    EndDialog(0);
  }
  else
  {
    MessageBox(_T("����ʧ��"));
  }
*/
  return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CByPassDEPDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this); // ���ڻ��Ƶ��豸������

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // ʹͼ���ڹ����������о���
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // ����ͼ��
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialogEx::OnPaint();
  }
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CByPassDEPDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

void CByPassDEPDlg::OnBnClickedButtonHandle()
{
  UpdateData(TRUE);
  if (m_strPath.IsEmpty()) 
  {
    MessageBox(_T("������Firefox·��"));
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
      MessageBox(_T("ע�����û��Firefox��װ��Ϣ, ���ֶ�ѡ��Ҫ������ļ�"));
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

DWORD  WinExecAndWait32(LPCTSTR lpszAppPath,   // ִ�г����·��
  LPCTSTR lpParameters,  // ����
  LPCTSTR lpszDirectory, // ִ�л���Ŀ¼
  DWORD dwMilliseconds)  // ���ȴ�ʱ��, �������ʱ��ǿ����ֹ
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

  // ָ��ʱ��û����
  if (WaitForSingleObject(ShExecInfo.hProcess, dwMilliseconds) == WAIT_TIMEOUT)
  {    // ǿ��ɱ������
    TerminateProcess(ShExecInfo.hProcess, 0);
    return 0;    //ǿ����ֹ
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
	  if (MessageBox(_T("��Ҫ�رս��̣��Ƿ����?"), _T("�ر���ʾ"), MB_YESNO) != IDYES)
	  {
		  return false;
	  }
	  while (!TerminateAllProcess(strPath)) 
	  {
		  if (MessageBox(_T("�޷��رս���"), _T("��ʾ"), MB_RETRYCANCEL) != IDRETRY)
		  {
		    Log(_T("�޷��رջ��"));
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
	  strLine = _T("����ɹ�!");
	}
	else
	{
	  strLine.Format(_T("����ʧ��!������%d"), nResult);  
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
    MessageBox(_T("����ɹ�"));
  }
  else
  {
    MessageBox(_T("����ʧ��"));
  }
}
