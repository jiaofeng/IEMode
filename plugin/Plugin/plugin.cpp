/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is 
* Netscape Communications Corporation.
* Portions created by the Initial Developer are Copyright (C) 1998
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or 
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK ***** */

//////////////////////////////////////////////////
//
// CPlugin class implementation
//
#include "stdafx.h"
#include <windows.h>
#include <windowsx.h>
#include "IEHostWindow.h"
#include "plugin.h"
#include "npfunctions.h"
#include "ScriptablePluginObject.h"
#include "HttpMonitorAPP.h"

namespace Plugin
{
	/** ���ڼ���HTTP��HTTPS����, ͬ��Cookie */
	CComPtr<IClassFactory> g_spCFHTTP;
	CComPtr<IClassFactory> g_spCFHTTPS;

	typedef PassthroughAPP::CMetaFactory<PassthroughAPP::CComClassFactoryProtocol, HttpMonitor::HttpMonitorAPP> MetaFactory;

	/** WH_GETMESSAGE, �������ؼ�����Ϣ */
	HHOOK s_hhookGetMessage = NULL;
	/** WH_CALLWNDPROCRET, �������� WM_KILLFOCUS ��Ϣ */
	HHOOK s_hhookCallWndProcRet = NULL;

	inline VOID PreTranslateAccelerator(HWND hwnd, MSG * pMsg)
	{
		static const UINT  WM_HTML_GETOBJECT = ::RegisterWindowMessage(_T( "WM_HTML_GETOBJECT" ));

		LRESULT lRes;
		if ( ::SendMessageTimeout( hwnd, WM_HTML_GETOBJECT, 0, 0, SMTO_ABORTIFHUNG, 1000, (DWORD*)&lRes ) && lRes )
		{
			CComPtr<IHTMLDocument2> spDoc;
			if (SUCCEEDED(::ObjectFromLresult(lRes, IID_IHTMLDocument2, 0, (void**)&spDoc)))
			{
				CComQIPtr<IServiceProvider> spSP(spDoc);
				if (spSP)
				{
					CComPtr<IWebBrowser2> spWB2;
					if ( SUCCEEDED(spSP->QueryService(IID_IWebBrowserApp, IID_IWebBrowser2, (void**)&spWB2)) && spWB2 )
					{
						CComPtr< IDispatch > pDisp;
						if ( SUCCEEDED(spWB2->get_Application(&pDisp)) && pDisp )
						{
							CComQIPtr< IOleInPlaceActiveObject > spObj( pDisp );
							if ( spObj )
							{
								if (spObj->TranslateAccelerator(pMsg) == S_OK)
								{
									pMsg->message = /*uMsg = */WM_NULL;
								}
							}
						}
					}
				}
			}
		}
	}

		// ���� IEMode ���ڵľ��
	inline HWND GetWebBrowserControlWindow(HWND hwnd)
	{
		// Internet Explorer_Server ���������� ATL:xxxxx ����
		HWND hwndAtl = ::GetParent(::GetParent(::GetParent(hwnd)));
		TCHAR szClassName[MAX_PATH];
		if ( GetClassName(hwndAtl, szClassName, ARRAYSIZE(szClassName)) > 0 )
		{
			if ( _tcsncmp(szClassName, _T("IEMode"), 6) == 0 )
			{
				return hwndAtl;
			}
		}
	
		return NULL;
	}

	LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) 
	{ 
		if ( (nCode >= 0) && (wParam == PM_REMOVE) && lParam)
		{
			MSG * pMsg = (MSG *)lParam;

			UINT uMsg = pMsg->message;
			if ( (uMsg >= WM_KEYFIRST) && (uMsg <= WM_KEYLAST) )
			{
				HWND hwnd = pMsg->hwnd;
				TCHAR szClassName[MAX_PATH];
				if ( GetClassName( hwnd, szClassName, ARRAYSIZE(szClassName) ) > 0 )
				{
					/*
					CString str;
					str.Format(_T("%s: Msg = %.4X, wParam = %.8X, lParam = %.8X\r\n"), szClassName, uMsg, pMsg->wParam, pMsg->lParam );
					OutputDebugString(str);
					*/
					if ( ( WM_KEYDOWN == uMsg ) && ( VK_TAB == pMsg->wParam ) && (_tcscmp(szClassName, _T("Internet Explorer_TridentCmboBx")) == 0) )
					{
						hwnd = ::GetParent(pMsg->hwnd);
					}
					else
					{
						if ( _tcscmp(szClassName, _T("Internet Explorer_Server")) == 0 )
						{
							// hwnd = pMsg->hwnd;
						}
						else
						{
							hwnd = NULL;
						}
					}

					if (hwnd)
					{
						PreTranslateAccelerator(hwnd, pMsg);

						if ( (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN /*|| uMsg == WM_CHAR*/) )
						{
							// BUG FIX: Characters like @, #, � (and others that require AltGr on European keyboard layouts) cannot be entered in CoralIETab
							// Suggested by Meyer Kuno (Helbling Technik): AltGr is represented in Windows massages as the combination of Alt+Ctrl, and that is used for text input, not for menu naviagation.
							// 
							bool bCtrlPressed = HIBYTE(GetKeyState(VK_CONTROL))!=0;
							bool bAltPressed = HIBYTE(GetKeyState(VK_MENU))!=0;

							if ( ( bCtrlPressed ^ bAltPressed ) || ((pMsg->wParam >= VK_F1) && (pMsg->wParam <= VK_F24)) )
							{
								/* Test Cases by Meyer Kuno (Helbling Technik):
								Ctrl-L (change keyboard focus to navigation bar)
								Ctrl-O (open a file)
								Ctrl-P (print)
								Alt-d (sometimes Alt-s): "Address": the IE-way of Ctrl-L
								Alt-F (open File menu) (NOTE: BUG: keyboard focus is not moved)
								*/
								HWND hwndAtl = GetWebBrowserControlWindow(hwnd);
								if (hwndAtl)
								{
									HWND hwndMessageTarget = GetMozillaContentWindow(hwndAtl);
									if ( ! hwndMessageTarget )
									{
										hwndMessageTarget = GetTopMozillaWindowClassWindow(hwndAtl);
									}

									if ( hwndMessageTarget )
									{
										if ( bAltPressed )
										{
											// BUG FIX: Alt-F (open File menu): keyboard focus is not moved
											switch ( pMsg->wParam )
											{
											case 'F':
											case 'E':
											case 'V':
											case 'S':
											case 'B':
											case 'T':
											case 'H':
												::SetFocus( hwndMessageTarget );
												break;
												// ���¿�ݼ��� IE �ڲ�����, ������� Firefox �Ļ��ᵼ���ظ�
											case VK_LEFT:
											case VK_RIGHT:
												uMsg = WM_NULL;
												break;
											default:
												break;
											}
										}
										else
										{
											if ( bCtrlPressed )
											{
												switch ( pMsg->wParam )
												{
													// ���¿�ݼ��� IE �ڲ�����, ������� Firefox �Ļ��ᵼ���ظ�
												case 'P':
												case 'F':
													uMsg = WM_NULL;
													break;
												default:
													break;
												}
											}
										}

										if ( uMsg != WM_NULL )
										{
											// ����ͬһ�����̵Ļ����� SendMessage() �����
											::PostMessage( hwndMessageTarget, uMsg, pMsg->wParam, pMsg->lParam );
										}
									}
								}
							}
						}
					}
				}
			}
		}

		return CallNextHookEx(s_hhookGetMessage, nCode, wParam, lParam); 
	} 

	LRESULT CALLBACK CallWndRetProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION)
		{
			CWPRETSTRUCT * info = (CWPRETSTRUCT*) lParam;
			UINT uMsg = info->message;
			// info->wParam == NULL ��ʾ�����Ƶ���������ȥ�ˣ�����ֻ�������ʱ���Ҫ����IE�Ľ���
			if ( ( WM_KILLFOCUS == uMsg ) && ( NULL == info->wParam ) )
			{
				HWND hwnd = info->hwnd;
				TCHAR szClassName[MAX_PATH];
				if ( GetClassName( hwnd, szClassName, ARRAYSIZE(szClassName) ) > 0 )
				{
					if ( _tcscmp(szClassName, _T("Internet Explorer_Server")) == 0 )
					{
						// ���°ѽ����Ƶ� ATL:xxxx �����ϣ������ӱ�Ľ��̴����л�������ʱ��IE�����н���
						HWND hwndAtl = GetWebBrowserControlWindow(hwnd);
						if (hwndAtl) ::SetFocus(hwndAtl);
					}
				}
			}
		}

		return CallNextHookEx(s_hhookCallWndProcRet, nCode, wParam, lParam);
	}

	// global plugin initialization
	NPError NS_PluginInitialize()
	{

    /*
// ����http��https����ͬ��cookie
		CComPtr<IInternetSession> spSession;
		HRESULT hret = S_OK;
		if (FAILED(CoInternetGetSession(0, &spSession, 0)) && spSession )
		{
			return NPERR_GENERIC_ERROR;
		}
		if (MetaFactory::CreateInstance(CLSID_HttpProtocol, &g_spCFHTTP) != S_OK)
		{
			return NPERR_GENERIC_ERROR;
		}
		if (spSession->RegisterNameSpace(g_spCFHTTP, CLSID_NULL, L"http", 0, 0, 0) != S_OK)
		{
			return NPERR_GENERIC_ERROR;
		}
		if (MetaFactory::CreateInstance(CLSID_HttpSProtocol, &g_spCFHTTPS) != S_OK)
		{
			return NPERR_GENERIC_ERROR;
		}
		if (spSession->RegisterNameSpace(g_spCFHTTPS, CLSID_NULL, L"https", 0, 0, 0) != S_OK)
		{
			return NPERR_GENERIC_ERROR;
		}
    */

		if (NULL == s_hhookGetMessage)
		{
			s_hhookGetMessage = SetWindowsHookEx( WH_GETMESSAGE, GetMsgProc, NULL, GetCurrentThreadId() );
		}

		if (NULL == s_hhookCallWndProcRet)
		{
			s_hhookCallWndProcRet = SetWindowsHookEx( WH_CALLWNDPROCRET, CallWndRetProc, NULL, GetCurrentThreadId() );
		}

		return NPERR_NO_ERROR;
	}

	// global shutdown
	void NS_PluginShutdown()
	{
		// ȡ������http��https����
		CComPtr<IInternetSession> spSession;
		if (SUCCEEDED(CoInternetGetSession(0, &spSession, 0)) && spSession )
		{
			if (g_spCFHTTP)
			{
				spSession->UnregisterNameSpace(g_spCFHTTP, L"http");
				g_spCFHTTP.Release();
			}
			if (g_spCFHTTPS)
			{
				spSession->UnregisterNameSpace(g_spCFHTTPS, L"https");
				g_spCFHTTPS.Release();
			}
		}
		if (s_hhookGetMessage)
		{
			UnhookWindowsHookEx(s_hhookGetMessage);
			s_hhookGetMessage = NULL;
		}

		if (s_hhookCallWndProcRet)
		{
			UnhookWindowsHookEx(s_hhookCallWndProcRet);
			s_hhookCallWndProcRet= NULL;
		}
	}


	CPlugin::CPlugin(NPP pNPInstance) :
	m_pNPInstance(pNPInstance),
		m_pNPStream(NULL),
		m_bInitialized(false),
		m_pScriptableObject(NULL),
		m_pIEHostWindow(NULL)
	{
	}

	CPlugin::~CPlugin()
	{
		if (m_pScriptableObject)
			NPN_ReleaseObject(m_pScriptableObject);
		if (m_pIEHostWindow)
			delete m_pIEHostWindow;
	}


	NPBool CPlugin::init(NPWindow* pNPWindow)
	{
		AfxEnableControlContainer();

		if(pNPWindow == NULL)
			return FALSE;

		m_hWnd = (HWND)pNPWindow->window;
		if (!IsWindow(m_hWnd))
			return FALSE;

		// ��ȡPlugin����ҳ���URL, URL�ĸ�ʽ��chrome://iemode/content/container.html?url=XXX��
		// ����XXX��ʵ��Ҫ���ʵ�URL
		CString strHostUrl = GetHostURL();
		static const CString PREFIX(_T("chrome://iemode/content/container.html?url="));

		// ��ȫ���, ���Plugin����ҳ���URL�����������������ҳ��������Plugin
		if (!strHostUrl.Mid(0, PREFIX.GetLength()) == PREFIX)
			return FALSE;

		// ��URL�����л�ȡʵ��Ҫ���ʵ�URL��ַ
		CString url = strHostUrl.Mid(PREFIX.GetLength());

		CWnd parent;
		if (!parent.Attach(m_hWnd))
			return FALSE;
		try
		{
			static const CString IEMODE_PREFIX(_T("IEMode.onNewIETab/"));
			if (url.Find(IEMODE_PREFIX) != -1)
			{
				CIEHostWindow::s_csNewIEWindowMap.Lock();
				// CIEhostWindow�Ѵ���
				DWORD id = _ttoi(url.Mid(IEMODE_PREFIX.GetLength()));
				m_pIEHostWindow = CIEHostWindow::s_NewIEWindowMap.Lookup(id);
				if (m_pIEHostWindow)
				{
					m_pIEHostWindow->SetPlugin(this);
					CIEHostWindow::s_NewIEWindowMap.Remove(id);
				}
				CIEHostWindow::s_csNewIEWindowMap.Unlock();
			}
			if (m_pIEHostWindow == NULL)
			{
				m_pIEHostWindow = new CIEHostWindow(this);
				if (m_pIEHostWindow == NULL)
				{
					throw CString(_T("Cannot new CIEHostWindow!"));
				}
				if (!m_pIEHostWindow->Create(CIEHostWindow::IDD))
				{
					throw CString(_T("CIEHostWindow.Create failed!"));
				}
			}
			m_pIEHostWindow->SetParent(&parent);
			CRect rect;
			parent.GetClientRect(rect);
			m_pIEHostWindow->MoveWindow(rect);
			m_pIEHostWindow->ShowWindow(SW_SHOW);
			if (url.Find(IEMODE_PREFIX) != -1)
			{
				if (m_pIEHostWindow->GetProgress() == -1)
				{
					FireEvent(_T("TitleChanged"), m_pIEHostWindow->GetTitle());
				}
			}
			else
			{
				m_pIEHostWindow->Navigate(url, _T(""), _T(""));
			}
		}
		catch (CString strMessage)
		{
			if (m_pIEHostWindow)
				delete m_pIEHostWindow;
			parent.Detach();
			TRACE(_T("[CPlugin::init] Exception: %s\n"), (LPCTSTR)strMessage);
			return FALSE;
		}
		parent.Detach();


		ScriptablePluginObject* sp = static_cast<ScriptablePluginObject*>(GetScriptableObject());
		if (sp == NULL)
			return false;
		sp->SetMainWindow(m_pIEHostWindow);

		// ����������, Firefox ���ڱ仯��ʱ��Ż�֪ͨ IE ����ˢ����ʾ
		SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE)|WS_CLIPCHILDREN);
		SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE)|WS_EX_CONTROLPARENT);

		m_bInitialized = TRUE;
		return TRUE;
	}

	NPError CPlugin::SetWindow(NPWindow* aWindow)
	{
		m_hWnd = (HWND)aWindow->window;
		if (IsWindow(m_hWnd) && m_pIEHostWindow && IsWindow(m_pIEHostWindow->GetSafeHwnd()))
		{
			CRect rect(0, 0, aWindow->width, aWindow->height);
			m_pIEHostWindow->MoveWindow(rect);
		}
		return NPERR_NO_ERROR;
	}

	void CPlugin::shut()
	{
		m_pIEHostWindow->DestroyWindow();
		m_hWnd = NULL;
		m_bInitialized = false;
	}

	void CPlugin::setStatus(const CString& text)
	{
		if (m_pNPInstance)
		{
			char* message = CStringToNPStringCharacters(text);
			NPN_Status(m_pNPInstance, message);
			NPN_MemFree(message);
		}
	}

	NPBool CPlugin::isInitialized()
	{
		return m_bInitialized;
	}

	NPObject *CPlugin::GetScriptableObject()
	{
		if (!m_pScriptableObject) 
		{
			m_pScriptableObject =
				NPN_CreateObject(m_pNPInstance,
				GET_NPOBJECT_CLASS(ScriptablePluginObject));
		}

		if (m_pScriptableObject) 
		{
			NPN_RetainObject(m_pScriptableObject);
		}

		return m_pScriptableObject;
	}

	// ��ȡPlugin����ҳ���URL
	CString CPlugin::GetHostURL() const
	{
		CString url;

		BOOL bOK = FALSE;
		NPObject* pWindow = NULL;
		NPVariant vLocation;
		VOID_TO_NPVARIANT(vLocation);
		NPVariant vHref;
		VOID_TO_NPVARIANT(vHref);

		try 
		{

			if (( NPN_GetValue( m_pNPInstance, NPNVWindowNPObject, &pWindow) != NPERR_NO_ERROR ) || !pWindow )
			{
				throw(CString(_T("Cannot get window")));
			}

			if ((!NPN_GetProperty( m_pNPInstance, pWindow, NPN_GetStringIdentifier ("location"), &vLocation)) || !NPVARIANT_IS_OBJECT (vLocation))
			{
				throw(CString(_T("Cannot get window.location")));
			}

			if ((!NPN_GetProperty( m_pNPInstance, NPVARIANT_TO_OBJECT(vLocation), NPN_GetStringIdentifier ("href"), &vHref)) || !NPVARIANT_IS_STRING(vHref))
			{
				throw(CString(_T("Cannot get window.location.href")));
			}

			// ת��window.location.href�ı���
			int buffer_size = vHref.value.stringValue.UTF8Length + 1;
			char* szUnescaped = new char[buffer_size];
			DWORD dwSize = buffer_size;
			if (SUCCEEDED(UrlUnescapeA(const_cast<LPSTR>(vHref.value.stringValue.UTF8Characters), szUnescaped, &dwSize, 0)))
			{
				WCHAR* szURL = new WCHAR[dwSize + 1];
				if (MultiByteToWideChar(CP_UTF8, 0, szUnescaped, -1, szURL, dwSize + 1) > 0)
				{
					url = CW2T(szURL);
				}
				delete[] szURL;
			}
			delete[] szUnescaped;

		}
		catch (CString strMessage)
		{
			TRACE(_T("[CPlugin::GetHostURL Exception] %s"), strMessage);
		}

		if (!NPVARIANT_IS_VOID(vHref))	NPN_ReleaseVariantValue(&vHref);
		if (!NPVARIANT_IS_VOID(vLocation))	NPN_ReleaseVariantValue(&vLocation);
		if (pWindow != NULL) NPN_ReleaseObject(pWindow);

		return url;
	}

	// This function is equivalent to the following JavaScript function:
	// function FireEvent(strEventType, strDetail) {
	//   var event = document.createEvent("CustomEvent");
	//   event.initCustomEvent(strEventType, true, true, strDetail);
	//   pluginObject.dispatchEvent(event);
	// }
	// 
	// Uses following JavaScript code to listen to the event fired:
	// pluginObject.addEventListener(strEventType, function(event) {
	//    alert(event.detail);
	// }
	BOOL CPlugin::FireEvent(const CString &strEventType, const CString &strDetail)
	{
		BOOL bOK = FALSE;
		NPObject* pWindow = NULL;
		NPVariant vDocument;
		VOID_TO_NPVARIANT(vDocument);
		NPVariant vEvent;
		NPObject* pDocument = NULL;
		VOID_TO_NPVARIANT(vEvent);
		NPObject *pEvent = NULL;
		NPObject* pPlugin = NULL;

		try
		{
			// get window object
			if (NPN_GetValue(m_pNPInstance, NPNVWindowNPObject, &pWindow) != NPERR_NO_ERROR || pWindow == NULL)
			{
				throw CString(_T("Cannot get window"));
			}

			// get window.document
			bOK = NPN_GetProperty(m_pNPInstance, pWindow, NPN_GetStringIdentifier("document"), &vDocument);
			if (!NPVARIANT_IS_OBJECT(vDocument) || !bOK)
			{
				throw CString(_T("Cannot get window.document"));
			}
			pDocument = NPVARIANT_TO_OBJECT(vDocument);

			// var event = document.createEvent("CustomEvent");
			if (pDocument) 
			{
				NPVariant arg;
				STRINGZ_TO_NPVARIANT(CStringToNPStringCharacters(_T("CustomEvent")), arg);
				bOK = NPN_Invoke(m_pNPInstance, pDocument, NPN_GetStringIdentifier("createEvent"), &arg, 1, &vEvent);
				NPN_ReleaseVariantValue(&arg);
				if (!NPVARIANT_IS_OBJECT(vEvent) || !bOK)
				{
					throw CString(_T("Cannot document.createEvent"));
				}
			}
			else 
			{
				throw CString(_T("window.document is null"));
			}
			pEvent = NPVARIANT_TO_OBJECT(vEvent);;

			// event.initCustomEvent(strEventType, true, true, strDetail);
			if (pEvent)
			{
				NPVariant args[4];
				STRINGZ_TO_NPVARIANT(CStringToNPStringCharacters(strEventType), args[0]);
				BOOLEAN_TO_NPVARIANT(true, args[1]);
				BOOLEAN_TO_NPVARIANT(true, args[2]);
				STRINGZ_TO_NPVARIANT(CStringToNPStringCharacters(strDetail), args[3]);
				NPVariant vResult;
				bOK = NPN_Invoke(m_pNPInstance, pEvent, NPN_GetStringIdentifier("initCustomEvent"), args, 4, &vResult);
				for (int i=0; i<4; i++)
				{
					NPN_ReleaseVariantValue(&args[i]);
				}
				NPN_ReleaseVariantValue(&vResult);
				if (!bOK)
				{
					throw CString(_T("Cannot event.initCustomEvent"));
				}
			}
			else
			{
				throw CString(_T("event is null"));
			}

			// get plugin object
			if (NPN_GetValue(m_pNPInstance, NPNVPluginElementNPObject, &pPlugin) != NPERR_NO_ERROR || pPlugin == NULL)
			{
				throw CString(_T("Cannot get window"));
			}


			// pluginObject.dispatchEvent(event);
			NPVariant vNotCanceled; 
			bOK = NPN_Invoke(m_pNPInstance, pPlugin, NPN_GetStringIdentifier("dispatchEvent"), &vEvent, 1, &vNotCanceled);
			NPN_ReleaseVariantValue(&vEvent);
			if (!bOK || !NPVARIANT_IS_BOOLEAN(vNotCanceled)) 
			{
				throw CString(_T("Cannot dispatchEvent"));
			}
			if (NPVARIANT_TO_BOOLEAN(vNotCanceled) != true)
			{
				throw CString(_T("Event is canceled"));
			}
		}
		catch (CString strMessage)
		{
			TRACE(_T("[CPlugin::FireEvent Exception] %s"), strMessage);
			bOK = FALSE;
		}
		if (pPlugin != NULL) NPN_ReleaseObject(pPlugin);
		if (!NPVARIANT_IS_VOID(vEvent))	NPN_ReleaseVariantValue(&vEvent);
		if (!NPVARIANT_IS_VOID(vDocument))	NPN_ReleaseVariantValue(&vDocument);
		if (pWindow != NULL) NPN_ReleaseObject(pWindow);
		return bOK;
	}

	double CPlugin::GetZoomLevel()
	{
		double level = 1;

		NPObject* pWindow = NULL;
		NPVariant vIEMode;
		VOID_TO_NPVARIANT(vIEMode);
		NPVariant vLevel;
		VOID_TO_NPVARIANT(vLevel);

		try
		{
			if ((NPN_GetValue( m_pNPInstance, NPNVWindowNPObject, &pWindow) != NPERR_NO_ERROR ) || !pWindow )
			{
				throw(CString(_T("Cannot get window")));
			}

			if ((!NPN_GetProperty( m_pNPInstance, pWindow, NPN_GetStringIdentifier ("IEMode"), &vIEMode)) || !NPVARIANT_IS_OBJECT (vIEMode))
			{
				throw(CString(_T("Cannot get window.IEMode")));
			}

			if (!NPN_Invoke(m_pNPInstance, NPVARIANT_TO_OBJECT(vIEMode), NPN_GetStringIdentifier("getZoomLevel"), NULL, 0, &vLevel))
			{
				throw(CString(_T("Cannot execute window.IEMode.getZoomLevel()")));
			}
			if (NPVARIANT_IS_DOUBLE(vLevel)) 
				level = NPVARIANT_TO_DOUBLE(vLevel);
			else if ( NPVARIANT_IS_INT32(vLevel) ) 
				level = NPVARIANT_TO_INT32(vLevel);
		}
		catch (CString strMessage)
		{
			level = 1;
			TRACE(_T("[CPlugin::FireEvent Exception] %s"), strMessage);
		}

		if (!NPVARIANT_IS_VOID(vLevel))	NPN_ReleaseVariantValue(&vLevel);
		if (!NPVARIANT_IS_VOID(vIEMode))	NPN_ReleaseVariantValue(&vIEMode);
		if (pWindow != NULL) NPN_ReleaseObject(pWindow);

		return level;
	}

	CString CPlugin::GetURLCookie(const CString& strURL)
	{
		CString strCookie;
		char* url = CStringToNPStringCharacters(strURL);
		char * cookies = NULL;
		uint32_t len = 0;
		if (NPN_GetValueForURL(m_pNPInstance, NPNURLVCookie, url, &cookies, &len) == NPERR_NO_ERROR && cookies)
		{
			strCookie = UTF8ToCString(cookies);
			NPN_MemFree(cookies);
		}
		NPN_MemFree(url);
		return strCookie;
	}

	void CPlugin::SetURLCookie(const CString& strURL, const CString& strCookie)
	{
		char* url = CStringToNPStringCharacters(strURL);
		char* cookie = CStringToNPStringCharacters(strCookie);
		if (NPN_SetValueForURL(m_pNPInstance, NPNURLVCookie, url, cookie, strlen(cookie)) != NPERR_NO_ERROR)
		{
			TRACE(_T("[CPlugin::SetURLCookie] NPN_SetValueForURL failed! URL: %s"), strURL);
		}
		NPN_MemFree(cookie);
		NPN_MemFree(url);
	}

	CString CPlugin::GetFirefoxUserAgent()
	{
		return UTF8ToCString(NPN_UserAgent(m_pNPInstance));
	}

	void CPlugin::NewIETab(DWORD id)
	{
		CString strEventType = _T("NewIETab");
		CString strDetail;
		strDetail.Format(_T("%d"), id);
		FireEvent(strEventType, strDetail);
	}

	void CPlugin::CloseIETab()
	{
		CString strEventType = _T("CloseIETab");
		CString strDetail = _T("");
		FireEvent(strEventType, strDetail);
	}
}