// MainWindow.cpp : implementation file
//

#include "stdafx.h"
#include <mshtml.h>
#include <exdispid.h>
#include <comutil.h>
#include "IEControlSite.h"
#include "IEModePlugin.h"
#include "IEHostWindow.h"
#include "plugin.h"



CSimpleMap<HWND, CIEHostWindow *> CIEHostWindow::s_IEWindowMap;
CSimpleMap<DWORD, CIEHostWindow *> CIEHostWindow::s_NewIEWindowMap;
CCriticalSection CIEHostWindow::s_csIEWindowMap; 
CCriticalSection CIEHostWindow::s_csNewIEWindowMap; 

// CIEHostWindow dialog

IMPLEMENT_DYNAMIC(CIEHostWindow, CDialog)

CIEHostWindow::CIEHostWindow(Plugin::CPlugin* pPlugin /*=NULL*/, CWnd* pParent /*=NULL*/)
: CDialog(CIEHostWindow::IDD, pParent)
, m_pPlugin(pPlugin)
, m_bCanBack(FALSE)
, m_bCanForward(FALSE)
, m_iProgress(-1)
, SyncUserAgent(TRUE)
{

}

CIEHostWindow::~CIEHostWindow()
{
}

/** ���� CIEHostWindow �� HWND Ѱ�Ҷ�Ӧ�� CIEHostWindow ���� */
CIEHostWindow * CIEHostWindow::GetInstance(HWND hwnd)
{
	CIEHostWindow *pInstance = NULL;
	s_csIEWindowMap.Lock();
	pInstance = s_IEWindowMap.Lookup(hwnd);
	s_csIEWindowMap.Unlock();
	return pInstance;
}

/** ���� URL Ѱ�Ҷ�Ӧ�� CIEHostWindow ���� */
CIEHostWindow * CIEHostWindow::GetInstance(const CString& URL)
{
	CIEHostWindow *pInstance = NULL;
	s_csIEWindowMap.Lock();
	for (int i=0; i<s_IEWindowMap.GetSize(); i++)
	{
		CIEHostWindow* p = s_IEWindowMap.GetValueAt(i);
		if (FuzzyUrlCompare( p->m_strLoadingUrl, URL))
		{
			pInstance = p;
			break;
		}
	}
	s_csIEWindowMap.Unlock();
	return pInstance;
}

BOOL CIEHostWindow::CreateControlSite(COleControlContainer* pContainer, 
									  COleControlSite** ppSite, UINT nID, REFCLSID clsid)
{
	ASSERT(ppSite != NULL);
	*ppSite = new CIEControlSite(pContainer, this);
	return TRUE;
}

void CIEHostWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IE_CONTROL, m_ie);
}


BEGIN_MESSAGE_MAP(CIEHostWindow, CDialog)
	ON_WM_SIZE()
	ON_MESSAGE(UserMessage::WM_USER_MESSAGE, OnUserMessage)
	ON_WM_PARENTNOTIFY()
END_MESSAGE_MAP()


// CIEHostWindow message handlers

BOOL CIEHostWindow::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	InitIE();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CIEHostWindow::InitIE()
{
  SetProcessDEPPolicy(2);
	s_csIEWindowMap.Lock();
	s_IEWindowMap.Add(GetSafeHwnd(), this);
	s_csIEWindowMap.Unlock();

	// ����IE�ؼ���һЩ����, ��ϸ��Ϣ��MSDN��CoInternetSetFeatureEnabled Function
	INTERNETFEATURELIST features[] = {FEATURE_WEBOC_POPUPMANAGEMENT
		, FEATURE_WEBOC_POPUPMANAGEMENT		// ����IE�ĵ������ڹ���
		, FEATURE_SECURITYBAND				// ���غͰ�װ���ʱ��ʾ
		, FEATURE_LOCALMACHINE_LOCKDOWN		// ʹ��IE�ı��ذ�ȫ����(Apply Local Machine Zone security settings to all local content.)
		, FEATURE_SAFE_BINDTOOBJECT			// ActiveX���Ȩ�޵�����, ���幦�ܲ��꣬Coral IE Tab�������ѡ��
		, FEATURE_TABBED_BROWSING			// ���ö��ǩ���
	};			
	int n = sizeof(features) / sizeof(INTERNETFEATURELIST);
	for (int i=0; i<n; i++)
	{
		CoInternetSetFeatureEnabled(features[i], SET_FEATURE_ON_PROCESS, TRUE);
	}

	// ���νű�������ʾ
	//m_ie.put_Silent(TRUE);
}


void CIEHostWindow::UninitIE()
{
	s_csIEWindowMap.Lock();
	s_IEWindowMap.Remove(GetSafeHwnd());
	s_csIEWindowMap.Unlock();
}


void CIEHostWindow::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (m_ie.GetSafeHwnd())
	{
		m_ie.MoveWindow(0, 0, cx, cy);
	}
}

HRESULT CIEHostWindow::OnUserMessage(WPARAM wParam, LPARAM lParam)
{
	using namespace UserMessage;
	switch(wParam)
	{
	case WPARAM_SET_FIREFOX_COOKIE:
		{
			LParamSetFirefoxCookie* pData = reinterpret_cast<LParamSetFirefoxCookie*>(lParam);
			OnSetFirefoxCookie(pData->strURL, pData->strCookie);
		}
		break;
	}
	return 0;
}


BEGIN_EVENTSINK_MAP(CIEHostWindow, CDialog)
	ON_EVENT(CIEHostWindow, IDC_IE_CONTROL, DISPID_COMMANDSTATECHANGE, CIEHostWindow::OnCommandStateChange, VTS_I4 VTS_BOOL)
	ON_EVENT(CIEHostWindow, IDC_IE_CONTROL, DISPID_STATUSTEXTCHANGE  , CIEHostWindow::OnStatusTextChange, VTS_BSTR)
	ON_EVENT(CIEHostWindow, IDC_IE_CONTROL, DISPID_TITLECHANGE       , CIEHostWindow::OnTitleChange, VTS_BSTR)
	ON_EVENT(CIEHostWindow, IDC_IE_CONTROL, DISPID_PROGRESSCHANGE    , CIEHostWindow::OnProgressChange, VTS_I4 VTS_I4)
	ON_EVENT(CIEHostWindow, IDC_IE_CONTROL, DISPID_BEFORENAVIGATE2   , CIEHostWindow::OnBeforeNavigate2, VTS_DISPATCH VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PBOOL)
	ON_EVENT(CIEHostWindow, IDC_IE_CONTROL, DISPID_DOCUMENTCOMPLETE  , CIEHostWindow::OnDocumentComplete, VTS_DISPATCH VTS_PVARIANT)
	ON_EVENT(CIEHostWindow, IDC_IE_CONTROL, DISPID_NEWWINDOW3        , CIEHostWindow::OnNewWindow3Ie, VTS_PDISPATCH VTS_PBOOL VTS_UI4 VTS_BSTR VTS_BSTR)
END_EVENTSINK_MAP()


void CIEHostWindow::OnCommandStateChange(long Command, BOOL Enable)
{
	// TODO: Add your message handler code here
	switch (Command)
	{
	case CSC_NAVIGATEBACK:
		m_bCanBack =  Enable;
		break;
	case CSC_NAVIGATEFORWARD:
		m_bCanForward = Enable;
		break;
	}
}

// Pack some data into a SAFEARRAY of BYTEs
HRESULT FillSafeArray(_variant_t &vDest, LPCSTR szSrc)
{
    HRESULT hr;
    LPSAFEARRAY psa;
    UINT cElems = strlen(szSrc);
    LPSTR pPostData;

    psa = SafeArrayCreateVector(VT_UI1, 0, cElems);
    if (!psa)
    {
      return E_OUTOFMEMORY;
    }

    hr = SafeArrayAccessData(psa, (LPVOID*)&pPostData);
    memcpy(pPostData, szSrc, cElems);
    hr = SafeArrayUnaccessData(psa);

    vDest.vt = VT_ARRAY | VT_UI1;
    vDest.parray = psa;
    return NOERROR;
}

void CIEHostWindow::Navigate(const CString& strURL, const CString& strPost, const CString& strHeaders)
{
	m_strLoadingUrl = strURL;
	if (m_ie.GetSafeHwnd())
	{
		try
		{
      _variant_t vFlags(0l);
      _variant_t vTarget(_T(""));
      _variant_t vPost;
      _variant_t vHeader(strHeaders);
      
      if (!strPost.IsEmpty()) 
      {
        int pos = strPost.Find(_T("\r\n\r\n"));
        CString strTrimed = strPost.Right(strPost.GetLength() - pos - 4);
        int size = WideCharToMultiByte(CP_ACP, 0, strTrimed, -1, 0, 0, 0, 0);
        char* szPostData = new char[size + 1];
	      WideCharToMultiByte(CP_ACP, 0, strTrimed, -1, szPostData, size, 0, 0);
	      FillSafeArray(vPost, szPostData);
      }
      m_ie.Navigate(strURL, &vFlags, &vTarget, &vPost, &vHeader);
		}
		catch(...)
		{
			TRACE(_T("CIEHostWindow::Navigate URL=%s failed!\n"), strURL);
		}
	}
}

void CIEHostWindow::Refresh()
{
	if (m_ie.GetSafeHwnd())
	{
		try
		{
			m_ie.Refresh();
		}
		catch(...)
		{
			TRACE(_T("CIEHostWindow::Refresh failed!\n"));
		}
	}
}

void CIEHostWindow::Stop()
{
	if (m_ie.GetSafeHwnd())
	{
		try
		{
			m_ie.Stop();
		}
		catch(...)
		{
			TRACE(_T("CIEHostWindow::Stop failed!\n"));
		}
	}
}

void CIEHostWindow::Back()
{
	if (m_ie.GetSafeHwnd() && m_bCanBack)
	{
		try
		{
			m_ie.GoBack();
		}
		catch(...)
		{
			TRACE(_T("CIEHostWindow::Back failed!\n"));
		}
	}
}

void CIEHostWindow::Forward()
{
	if (m_ie.GetSafeHwnd() && m_bCanForward)
	{
		try
		{
			m_ie.GoForward();
		}
		catch(...)
		{
			TRACE(_T("CIEHostWindow::Forward failed!\n"));
		}
	}
}

void CIEHostWindow::Focus()
{
	if (m_ie.GetSafeHwnd())
	{
		m_ie.SetFocus();
	}
}

void CIEHostWindow::Copy()
{
	ExecOleCmd(OLECMDID_COPY);
}

void CIEHostWindow::Cut()
{
	ExecOleCmd(OLECMDID_CUT);
}

void CIEHostWindow::Paste()
{
	ExecOleCmd(OLECMDID_PASTE);
}

void CIEHostWindow::SelectAll()
{
	ExecOleCmd(OLECMDID_SELECTALL);
}

void CIEHostWindow::Find()
{
	ExecOleCmd(OLECMDID_FIND);
}

// ����Ҫ����Ϣ���͵� MozillaContentWindow ���Ӵ��ڣ�����������ڽṹ�Ƚϸ��ӣ�Firefox/SeaMonkey������ͬ��
// Firefox ��������� OOPP Ҳ������һ������������ר��дһ�����ҵĺ���
HWND GetMozillaContentWindow(HWND hwndAtl)
{
	//�����������취����һ��ѭ�������ң�ֱ���ҵ� MozillaContentWindow Ϊֹ
	HWND hwnd = ::GetParent(hwndAtl);
	for ( int i = 0; i < 5; i++ )
	{
		hwnd = ::GetParent( hwnd );
		TCHAR szClassName[MAX_PATH];
		if ( GetClassName(::GetParent(hwnd), szClassName, ARRAYSIZE(szClassName)) > 0 )
		{
			if ( _tcscmp(szClassName, _T("MozillaContentWindowClass")) == 0 )
			{
				return hwnd;
			}
		}
	}

	return NULL;
}

// Firefox 4.0 ��ʼ�������µĴ��ڽṹ
// ���ڲ�����Ƿ��� GeckoPluginWindow �����������һ�� MozillaWindowClass���������Ƕ����
// MozillaWindowClass�����ǵ���ϢҪ�������㣬������дһ�����ҵĺ���
HWND GetTopMozillaWindowClassWindow(HWND hwndAtl)
{
	HWND hwnd = ::GetParent(hwndAtl);
	for ( int i = 0; i < 5; i++ )
	{
		HWND hwndParent = ::GetParent( hwnd );
		if ( NULL == hwndParent ) break;
		hwnd = hwndParent;
	}

	TCHAR szClassName[MAX_PATH];
	if ( GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName)) > 0 )
	{
		if ( _tcscmp(szClassName, _T("MozillaWindowClass")) == 0 )
		{
			return hwnd;
		}
	}

	return NULL;
}

void CIEHostWindow::HandOverFocus()
{
	HWND hwndMessageTarget = GetMozillaContentWindow(m_hWnd);
	if (!hwndMessageTarget)
	{
		hwndMessageTarget = GetTopMozillaWindowClassWindow(m_hWnd);
	}

	if ( hwndMessageTarget != NULL )
	{
		::SetFocus(hwndMessageTarget);
	}
}

void CIEHostWindow::Zoom(double level)
{
	if (level <= 0.01)
		return;

	int nZoomLevel = (int)(level * 100 + 0.5);

	CComVariant vZoomLevel(nZoomLevel);

	// >= IE7
	try
	{
		m_ie.ExecWB(OLECMDID_OPTICAL_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, &vZoomLevel, NULL);
		return;
	}
	catch (...)
	{
		TRACE(_T("CIEHostWindow::Zoom failed!\n"));
	}

	// IE6
	try
	{
		// IE6 ֻ֧����������, ��СΪ0, ���Ϊ4, Ĭ��Ϊ2
		int nLegecyZoomLevel = (int)((level - 0.8) * 10 + 0.5);
		nLegecyZoomLevel = max(nLegecyZoomLevel, 0);
		nLegecyZoomLevel = min(nLegecyZoomLevel, 4);

		vZoomLevel.intVal = nLegecyZoomLevel;
		m_ie.ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, &vZoomLevel, NULL );
	}
	catch(...)
	{
		TRACE(_T("CIEHostWindow::Zoom failed!\n"));
	}
}

void CIEHostWindow::DisplaySecurityInfo()
{

}

void CIEHostWindow::SaveAs()
{
	ExecOleCmd(OLECMDID_SAVEAS);
}

void CIEHostWindow::Print()
{
	ExecOleCmd(OLECMDID_PRINT);
}

void CIEHostWindow::PrintPreview()
{
	ExecOleCmd(OLECMDID_PRINTPREVIEW);
}

void CIEHostWindow::PrintSetup()
{
	ExecOleCmd(OLECMDID_PAGESETUP);
}

CString CIEHostWindow::GetURL()
{
	CString url;
	try
	{
		if (m_ie.GetSafeHwnd())
		{
			url = m_ie.get_LocationURL();
		}
	}
	catch(...)
	{
		TRACE(_T("CIEHostWindow::GetURL failed!\n"));
	}
	return url;
}

CString CIEHostWindow::GetTitle()
{
	CString title;
	try
	{
		if (m_ie.GetSafeHwnd())
		{
			title = m_ie.get_LocationName();
		}
	}
	catch(...)
	{
		TRACE(_T("CIEHostWindow::GetTitle failed!\n"));
	}
	return title;
}

BOOL CIEHostWindow::IsOleCmdEnabled(OLECMDID cmdID)
{
	try
	{
		if (m_ie.GetSafeHwnd())
		{
			long result = m_ie.QueryStatusWB(cmdID);
			return  (result & OLECMDF_ENABLED) != 0; 
		}
	}
	catch(...)
	{
		TRACE(_T("CIEHostWindow::IsOleCmdEnabled id=%d failed!\n"), cmdID);
	}
	return false;
}

void CIEHostWindow::ExecOleCmd(OLECMDID cmdID)
{
	try
	{
		if(m_ie.GetSafeHwnd() && 
			(m_ie.QueryStatusWB(cmdID) & OLECMDF_ENABLED))
		{
			m_ie.ExecWB(cmdID, 0, NULL, NULL);
		}
	}
	catch(...)
	{
		TRACE(_T("CIEHostWindow::ExecOleCmd id=%d failed!\n"), cmdID);
	}
}

void CIEHostWindow::OnSetFirefoxCookie(const CString& strURL, const CString& strCookie)
{
  if (m_pPlugin)
  {
    m_pPlugin->SetURLCookie(strURL, strCookie);
  }
}

void CIEHostWindow::OnTitleChanged(const CString& title)
{
  if (m_pPlugin)
  {
	  m_pPlugin->FireEvent(_T("TitleChanged"), title);
  }
}

void CIEHostWindow::OnProgressChanged(INT32 iProgress)
{
  if (m_pPlugin)
  {
	  CString strDetail;
	  strDetail.Format(_T("%d"), iProgress);
	  m_pPlugin->FireEvent(_T("ProgressChanged"), strDetail);
  }
}

void CIEHostWindow::OnStatusChanged(const CString& message)
{
  if (m_pPlugin)
  {
	  m_pPlugin->setStatus(message);
  }
}

void CIEHostWindow::OnCloseIETab()
{
	if (m_pPlugin)
	{
		m_pPlugin->CloseIETab();
	}
}
void CIEHostWindow::OnStatusTextChange(LPCTSTR Text)
{
	// TODO: Add your message handler code here
	OnStatusChanged(Text);
}


void CIEHostWindow::OnTitleChange(LPCTSTR Text)
{
	// TODO: Add your message handler code here
	OnTitleChanged(Text);
}


void CIEHostWindow::OnProgressChange(long Progress, long ProgressMax)
{
	// TODO: Add your message handler code here
	if (Progress == -1) 
		Progress = ProgressMax;
	if (ProgressMax > 0) 
		m_iProgress = (100 * Progress) / ProgressMax; 
	else 
		m_iProgress = -1;
	OnProgressChanged(m_iProgress);
}


void CIEHostWindow::OnBeforeNavigate2(LPDISPATCH pDisp, VARIANT* URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers, BOOL* Cancel)
{
	COLE2T szURL(URL->bstrVal);
	m_strLoadingUrl = szURL;
	// TODO: Add your message handler code here
}


void CIEHostWindow::OnDocumentComplete(LPDISPATCH pDisp, VARIANT* URL)
{
	// TODO: Add your message handler code here
	m_iProgress = -1;
	OnProgressChanged(m_iProgress);

	// ��Firefox����������ҳ��
  if (m_pPlugin)
  {
	  double level = m_pPlugin->GetZoomLevel();
	  if (fabs(level - 1.0) > 0.01) 
	  {
		  Zoom(level);
	  }
  }
}

BOOL CIEHostWindow::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	UninitIE();

	return CDialog::DestroyWindow();
}


/** ����֮����Ҫʹ��NewWindow3����ʹ��NewWindow2������ΪNewWindow3�ṩ��bstrUrlContext������
* �ò������������´����ӵ�referrer,һЩ��վͨ�����referrer����ֹ����
*/
void CIEHostWindow::OnNewWindow3Ie(LPDISPATCH* ppDisp, BOOL* Cancel, unsigned long dwFlags, LPCTSTR bstrUrlContext, LPCTSTR bstrUrl)
{
  if (m_pPlugin)
  {
    s_csNewIEWindowMap.Lock();

    CIEHostWindow* pIEHostWindow = new CIEHostWindow();
    if (pIEHostWindow->Create(CIEHostWindow::IDD))
    {
      DWORD id = GetTickCount();
      s_NewIEWindowMap.Add(id, pIEHostWindow);
      *ppDisp = pIEHostWindow->m_ie.get_Application();
      m_pPlugin->NewIETab(id);
    }
    else
    {
      delete pIEHostWindow;
      *Cancel = TRUE;
    }
    s_csNewIEWindowMap.Unlock();
  }
}


