#pragma once

#include "resource.h"
#include "IECtrl.h"

namespace Plugin
{
	class CPlugin;
}

namespace HttpMonitor
{
	class MonitorSink;
}

namespace UserMessage
{
	// �Զ��崰����Ϣ
	static const UINT WM_USER_MESSAGE =  WM_USER + 200;

	// Sub-types of the user defined window message
	static const WPARAM WPARAM_SET_FIREFOX_COOKIE = 0;
	struct LParamSetFirefoxCookie
	{
		CString strURL;
		CString strCookie;
	};
}

// ����Ҫ����Ϣ���͵� MozillaContentWindow ���Ӵ��ڣ�����������ڽṹ�Ƚϸ��ӣ�Firefox/SeaMonkey������ͬ��
// Firefox ��������� OOPP Ҳ������һ������������ר��дһ�����ҵĺ���
HWND GetMozillaContentWindow(HWND hwndAtl);

// Firefox 4.0 ��ʼ�������µĴ��ڽṹ
// ���ڲ�����Ƿ��� GeckoPluginWindow �����������һ�� MozillaWindowClass���������Ƕ����
// MozillaWindowClass�����ǵ���ϢҪ�������㣬������дһ�����ҵĺ���
HWND GetTopMozillaWindowClassWindow(HWND hwndAtl);


// CIEHostWindow dialog

class CIEHostWindow : public CDialog
{
	DECLARE_DYNAMIC(CIEHostWindow)
	DECLARE_EVENTSINK_MAP()
	DECLARE_MESSAGE_MAP()

	friend class HttpMonitor::MonitorSink;

public:
	/** ���� CIEHostWindow �� HWND Ѱ�Ҷ�Ӧ�� CIEHostWindow ���� */
	static CIEHostWindow * GetInstance(HWND hwnd);

	/** ���� URL Ѱ�Ҷ�Ӧ�� CIEHostWindow ���� */
	static CIEHostWindow * GetInstance(const CString& URL);

public:
	CIEHostWindow(Plugin::CPlugin* pPlugin = NULL, CWnd* pParent = NULL);   // standard constructor
	virtual ~CIEHostWindow();

	virtual BOOL CreateControlSite(COleControlContainer* pContainer, 
		COleControlSite** ppSite, UINT nID, REFCLSID clsid);

// Dialog Data
	enum { IDD = IDD_MAIN_WINDOW };

// Overrides
	virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();

  /** ���ô��ڹ�����Plugin���� */
  void SetPlugin(Plugin::CPlugin* pPlugin) {m_pPlugin = pPlugin;}
protected:
  /** HWND�� CIEWindow �����ӳ��, ����ͨ�� HWND �����ҵ��Ѵ򿪵� CIEWindow ���� */
  static CSimpleMap<HWND, CIEHostWindow *> s_IEWindowMap;
  /** �� s_IEWindowMap ���ʹ�õ�, ��֤�̰߳�ȫ */
  static CCriticalSection s_csIEWindowMap;

	void InitIE();
	void UninitIE();

	// �������������Ƿ����
	BOOL IsOleCmdEnabled(OLECMDID cmdID);

	// ִ�����������
	void ExecOleCmd(OLECMDID cmdID);

	// �Զ��崰����Ϣ��Ӧ����
	void OnSetFirefoxCookie(const CString& strURL, const CString& strCookie);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HRESULT OnUserMessage(WPARAM wParam, LPARAM lParam);
	void OnCommandStateChange(long Command, BOOL Enable);
	void OnStatusTextChange(LPCTSTR Text);
	void OnTitleChange(LPCTSTR Text);
	void OnProgressChange(long Progress, long ProgressMax);
	void OnBeforeNavigate2(LPDISPATCH pDisp, VARIANT* URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers, BOOL* Cancel);
	void OnDocumentComplete(LPDISPATCH pDisp, VARIANT* URL);
	void OnNewWindow3Ie(LPDISPATCH* ppDisp, BOOL* Cancel, unsigned long dwFlags, LPCTSTR bstrUrlContext, LPCTSTR bstrUrl);

public:
	CIECtrl m_ie;

  /** ID�� CIEWindow �����ӳ��, ����ͨ�� ID �����ҵ�������δʹ�õ� CIEWindow ���� */
  static CSimpleMap<DWORD, CIEHostWindow *> s_NewIEWindowMap;
  /** �� s_csNewIEWindowMap ���ʹ�õ�, ��֤�̰߳�ȫ */
  static CCriticalSection s_csNewIEWindowMap;

  /** ���ڼ��ص� URL. */
  CString m_strLoadingUrl;

	// plugin methods
	void Navigate(const CString& strURL, const CString& strPost, const CString& strHeaders);
	void Refresh();
	void Stop();
	void Back();
	void Forward();
	void Focus();
	void Copy();
	void Cut();
	void Paste();
	void SelectAll();
	void Find();
	void HandOverFocus();
	void Zoom(double level);
	void DisplaySecurityInfo();
	void SaveAs();
	void Print();
	void PrintPreview();
	void PrintSetup();

	// read only plugin properties
	CString GetURL();
	CString GetTitle();
	BOOL GetCanBack() {return m_bCanBack;}
	BOOL GetCanForward() {return m_bCanForward;}
	BOOL GetCanStop() {return IsOleCmdEnabled(OLECMDID_STOP);}
	BOOL GetCanRefresh() {return IsOleCmdEnabled(OLECMDID_REFRESH);}
	BOOL GetCanCopy(){return IsOleCmdEnabled(OLECMDID_COPY);}
	BOOL GetCanCut(){return IsOleCmdEnabled(OLECMDID_CUT);}
	BOOL GetCanPaste(){return IsOleCmdEnabled(OLECMDID_PASTE);}
	BOOL GetCanSelectAll(){return IsOleCmdEnabled(OLECMDID_SELECTALL);}
	INT32 GetProgress() {return m_iProgress;}

	// plugin events
	void OnTitleChanged(const CString& title);
	void OnProgressChanged(INT32 iProgress);
	void OnStatusChanged(const CString& message);
	void OnCloseIETab();

protected:
	BOOL m_bCanBack;
	BOOL m_bCanForward;
	INT32 m_iProgress;

	/** DIRTY FIX: NewWindow3 ���洴���� IE ���ڲ������� Referrer */
	CString m_strUrlContext;

	BOOL SyncUserAgent;

	Plugin::CPlugin* m_pPlugin;
};
