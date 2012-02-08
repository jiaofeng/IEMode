#include "StdAfx.h"
#include <Wininet.h>
#include <string>
#pragma comment(lib, "Wininet.lib")

#include "plugin.h"
#include "IEHostWindow.h"
#include "MonitorSink.h"
#include "IEModePlugin.h"
#include "ScriptablePluginObject.h"

namespace HttpMonitor
{
	// ���� \0 �ָ��� Raw HTTP Header ����ת������ \r\n �ָ��� Header
	void HttpRawHeader2CrLfHeader(LPCSTR szRawHeader, CString & strCrLfHeader)
	{
		strCrLfHeader.Empty();

		LPCSTR p = szRawHeader;
		while ( p[0] )
		{
			CString strHeaderLine(p);

			p += strHeaderLine.GetLength() + 1;

			strCrLfHeader += strHeaderLine + _T("\r\n");
		}
	}


	// @todo ��ʲô�ã�
	LPWSTR ExtractFieldValue(LPCWSTR szHeader, LPCWSTR szFieldName, LPWSTR * pFieldValue, size_t * pSize )
	{
		LPWSTR r = NULL;

		do 
		{
			// ���� RFC2616 �涨, HTTP field name �����ִ�Сд
			LPWSTR pStart = StrStrIW( szHeader, szFieldName );
			if ( ! pStart ) break;
			pStart += wcslen(szFieldName);
			while ( L' ' == pStart[0] ) pStart++;		// ������ͷ�Ŀո�
			LPWSTR pEnd = StrStrW( pStart, L"\r\n" );
			if ( ( ! pEnd ) || ( pEnd <= pStart ) ) break;

			size_t nSize = pEnd - pStart;
			size_t nBufLen = nSize + 2;		// �����ַ����� 0 ������
			LPWSTR lpBuffer = (LPWSTR)VirtualAlloc( NULL, nBufLen * sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE );
			if ( !lpBuffer ) break;

			if ( wcsncpy_s( lpBuffer, nBufLen, pStart, nSize) )
			{
				VirtualFree( lpBuffer, 0, MEM_RELEASE);
				break;
			}

			* pSize = nBufLen;
			* pFieldValue = lpBuffer;
			r = pEnd;

		} while(false);

		return r;
	}

	CIEHostWindow* MonitorSink::s_pLastIEHostWindow = NULL;

	MonitorSink::MonitorSink()
		: m_pIEHostWindow(NULL)
		, m_bIsSubRequest(TRUE)
	{
	}

	MonitorSink::~MonitorSink()
	{
	}


	void MonitorSink::QueryIEHostWindow()
	{
		m_pIEHostWindow = NULL;
		CComPtr<IWindowForBindingUI> spWindowForBindingUI;
		if ( SUCCEEDED(QueryServiceFromClient(&spWindowForBindingUI)) && spWindowForBindingUI )
		{
			HWND hwndIEServer = NULL;
			if ((spWindowForBindingUI->GetWindow(IID_IHttpSecurity, &hwndIEServer) == S_OK) && IsWindow(hwndIEServer))
			{
				// ����õ��� hwndIEServer ����ܸ���, �� Internet Explorer_Server ���ڻ�û�����ü�������ʱ��(�շ�����������ʱ��),
				// hwndIEServer �� Shell Embedding ���ڵľ��; ֮���������� Internet Explorer_Server ���ڵľ��, ��ʱ��Ҳ����
				// Shell DocObject View ���ڵľ��

				HWND hwndIEHost = ::GetParent(hwndIEServer);

				// ������������, ����ʹ� hwndIEServer һֱ������, ֱ���ҵ��� CIEHostWindow �� ATL Host ����Ϊֹ. Ϊ�˰�ȫ���, ���
				// ������ 5 ��
				CString strClassName;
				for ( int i = 0; i < 5; i++ )
				{
					int cnt = GetClassName(hwndIEHost, strClassName.GetBuffer(MAX_PATH), MAX_PATH);
					strClassName.ReleaseBuffer();
					if (cnt == 0)
					{
						break;
					}

					if (strClassName == STR_WINDOW_CLASS_NAME)
					{
						// �ҵ���
						m_pIEHostWindow = CIEHostWindow::GetInstance(hwndIEHost);
						return;;
					}

					hwndIEHost = ::GetParent(hwndIEHost);
				}
			}
		}
	}

	STDMETHODIMP MonitorSink::BeginningTransaction(
		LPCWSTR szURL,
		LPCWSTR szHeaders,
		DWORD dwReserved,
		LPWSTR *pszAdditionalHeaders)
	{
		if (pszAdditionalHeaders)
		{
			*pszAdditionalHeaders = 0;
		}

		// �ȵ���Ĭ�ϵ� IHttpNegotiate ����ӿ�, ��Ϊ����֮�� pszAdditionalHeaders �Ż��� Referer ����Ϣ
		CComPtr<IHttpNegotiate> spHttpNegotiate;
		QueryServiceFromClient(&spHttpNegotiate);
		HRESULT hr = spHttpNegotiate ?
			spHttpNegotiate->BeginningTransaction(szURL, szHeaders,
			dwReserved, pszAdditionalHeaders) :
		E_UNEXPECTED;

		m_strURL = szURL;

		QueryIEHostWindow();

		// ���� URL ��ʶ���Ƿ���ҳ���ڵ�������
		// @todo ��URI���滻FuzzyUrlCompare����
		m_bIsSubRequest = !(m_pIEHostWindow && FuzzyUrlCompare(m_pIEHostWindow->m_strLoadingUrl, m_strURL));


		// ������� User-Agent��Referrer ���ӵ� pszAdditionalHeaders ��
		SetCustomHeaders(pszAdditionalHeaders);

		if (!m_pIEHostWindow)
		{
			m_pIEHostWindow = CIEHostWindow::GetInstance(m_strReferer);
		}

		if (!m_pIEHostWindow)
		{
			if (MonitorSink::s_pLastIEHostWindow->GetSafeHwnd())
			{
				m_pIEHostWindow = MonitorSink::s_pLastIEHostWindow;
			}
			else
			{
				MonitorSink::s_pLastIEHostWindow = NULL;
			}
		}

		if (m_pIEHostWindow)
		{
			MonitorSink::s_pLastIEHostWindow = m_pIEHostWindow;
		}

		return hr;
	}

	void MonitorSink::SetCustomHeaders(LPWSTR *pszAdditionalHeaders)
	{
		if ( pszAdditionalHeaders )
		{
			static const WCHAR REFERER [] = L"Referer:";

			CStringW strHeaders(*pszAdditionalHeaders);

			if ( m_pIEHostWindow)
			{
				if (!m_strURL.IsEmpty())
				{
					ImportCookies();
				}

				if (m_pIEHostWindow->SyncUserAgent)
				{
					// ���� User-Agent
					CString strUserAgent;
					strUserAgent.Format(_T("User-Agent: %s\r\n"), m_pIEHostWindow->m_pPlugin->GetFirefoxUserAgent());

					strHeaders += strUserAgent;
				}

				// ����� m_strUrlContext, ˵�������´���, ��Ҫ�� IE ���� Referer
				if (! m_pIEHostWindow->m_strUrlContext.IsEmpty() )
				{
					if (StrStrIW( *pszAdditionalHeaders, REFERER))
					{
						// �Ѿ��� Referer ��, �ǾͲ�����
						if (!m_bIsSubRequest)
						{
							m_pIEHostWindow->m_strUrlContext.Empty();
						}
					}
					else
					{
						CString strReferer;
						strReferer.Format( _T("%s %s\r\n"), REFERER, m_pIEHostWindow->m_strUrlContext);

						strHeaders += strReferer;
					}
				}


				size_t nLen = strHeaders.GetLength() + 2;
				if ( *pszAdditionalHeaders = (LPWSTR)CoTaskMemRealloc(*pszAdditionalHeaders, nLen*sizeof(WCHAR)) )
				{
					wcscpy_s(*pszAdditionalHeaders, nLen, strHeaders);
				}
			}

			LPWSTR lpReferer = NULL;
			size_t nRefererLen = 0;
			if (ExtractFieldValue(*pszAdditionalHeaders, REFERER, & lpReferer, & nRefererLen ) )
			{
				m_strReferer = lpReferer;

				VirtualFree( lpReferer, 0, MEM_RELEASE);
			}
		}
	}


	inline char * SplitCookies(char * cookies, std::string & cookie_name, std::string & cookie_value)
	{
		char * p = cookies;
		// IE ���Լ����˵��ո���������Ĵ��벻��Ҫ��
		// while ( cookies && (*cookies == ' ') ) cookies++;			// �˵��ո�
		while ( p && (*p != 0) && (*p != '=') ) p++;
		if ( '=' == *p )
		{
			*p = 0;
			cookie_name = cookies;
			cookies = ++p;

			while ( (*p != 0) && (*p != ';') ) p++;
			if ( ';' == *p )
			{
				*p = 0;
			}
			cookie_value = cookies;

			return ++p;
		}

		return NULL;
	}

	void MonitorSink::ImportCookies()
	{
		CString strCookie = m_pIEHostWindow->m_pPlugin->GetURLCookie(m_strURL);
		CT2A url(m_strURL);
		if (!strCookie.IsEmpty())
		{
			CT2A cookies(strCookie);
			LPSTR p = cookies;
			std::string cookie_name;
			std::string cookie_value;
			while ( p = SplitCookies(p, cookie_name, cookie_value) )
			{
				InternetSetCookieA(url, cookie_name.c_str(), cookie_value.c_str());
			}
		}
		TRACE(_T("[MonitorSink::ImportCookies] URL: %s Cookie: %s"), m_strURL, strCookie);
	}


	STDMETHODIMP MonitorSink::OnResponse(
		DWORD dwResponseCode,
		LPCWSTR szResponseHeaders,
		LPCWSTR szRequestHeaders,
		LPWSTR *pszAdditionalRequestHeaders)
	{
		if (pszAdditionalRequestHeaders)
		{
			*pszAdditionalRequestHeaders = 0;
		}

		CComPtr<IHttpNegotiate> spHttpNegotiate;
		QueryServiceFromClient(&spHttpNegotiate);
		HRESULT hr = spHttpNegotiate ?
			spHttpNegotiate->OnResponse(dwResponseCode, szResponseHeaders,
			szRequestHeaders, pszAdditionalRequestHeaders) :
		E_UNEXPECTED;

		if ((dwResponseCode >= 200 ) && (dwResponseCode < 300))
		{
			// �����ﵼ�� Cookies, ���ܻ��а�ȫ������, ��һЩ������ Cookie Policy �� Cookie Ҳ�Ź�ȥ
			// ReportProgress() ���濴�ĵ��и� BINDSTATUS_SESSION_COOKIES_ALLOWED, �о�Ҫ����ȫһЩ, ����ʵ������ʱһֱû�е������״̬
			// Ҳ�� Firefox �Լ��ᴦ��
			ExportCookies(szResponseHeaders);
		}
		return hr;
	}

	void MonitorSink::ExportCookies(LPCWSTR szResponseHeaders)
	{
		using namespace UserMessage;

		static const WCHAR SET_COOKIE_HEAD [] = L"\r\nSet-Cookie:";

		if (!m_pIEHostWindow) return;

		LPWSTR p = (LPWSTR)szResponseHeaders;
		LPWSTR lpCookies = NULL;
		size_t nCookieLen = 0;
		while (p = ExtractFieldValue(p, SET_COOKIE_HEAD, &lpCookies, & nCookieLen))
		{
			if (lpCookies)
			{
				CString strCookie((LPCTSTR)CW2T(lpCookies));
				LParamSetFirefoxCookie param = {m_strURL, strCookie};
				m_pIEHostWindow->SendMessage(WM_USER_MESSAGE, WPARAM_SET_FIREFOX_COOKIE, reinterpret_cast<LPARAM>(&param));
				VirtualFree( lpCookies, 0, MEM_RELEASE);
				lpCookies = NULL;
				nCookieLen = 0;
			}

		}
	}

	STDMETHODIMP MonitorSink::ReportProgress(
		ULONG ulStatusCode,
		LPCWSTR szStatusText)
	{
		HRESULT hr = m_spInternetProtocolSink ?
			m_spInternetProtocolSink->ReportProgress(ulStatusCode, szStatusText) :
		E_UNEXPECTED;
		if(m_pIEHostWindow)
		{
			switch ( ulStatusCode )
			{
				// �ض�����, ���¼�¼�� URL
			case BINDSTATUS_REDIRECTING:
				{

					if (!m_bIsSubRequest)
					{
						m_pIEHostWindow->m_strLoadingUrl = szStatusText;
						m_strURL = szStatusText;
					}

					// �ܶ���վ��¼��ʱ�����302��תʱ����Cookie, ����Gmail, ��������������ҲҪ���� Cookie
					CComPtr<IWinInetHttpInfo> spWinInetHttpInfo;
					if ( SUCCEEDED(m_spTargetProtocol->QueryInterface(&spWinInetHttpInfo)) && spWinInetHttpInfo )
					{
						CHAR szRawHeader[8192];		// IWinInetHttpInfo::QueryInfo() ���ص� Raw Header ���� Unicode ��
						DWORD dwBuffSize = ARRAYSIZE(szRawHeader);

						if ( SUCCEEDED(spWinInetHttpInfo->QueryInfo(HTTP_QUERY_RAW_HEADERS, szRawHeader, &dwBuffSize, 0, NULL)) )
						{
							// ע�� HTTP_QUERY_RAW_HEADERS ���ص� Raw Header �� \0 �ָ���, �� \0\0 ��Ϊ����, ��������Ҫ��ת��
							CString strHeader;
							HttpRawHeader2CrLfHeader(szRawHeader, strHeader);

							ExportCookies(strHeader);
						}
					}
				}
				break;
			}
		}
		return hr;
	}

	STDMETHODIMP MonitorSink::Authenticate( 
		/* [out] */ HWND *phwnd,
		/* [out] */ LPWSTR *pszUsername,
		/* [out] */ LPWSTR *pszPassword)
	{
		if ( (! m_strUsername.IsEmpty()) && (! m_strPassword.IsEmpty()) )
		{
			size_t len = m_strUsername.GetLength()+1;
			* pszUsername = (LPWSTR)CoTaskMemAlloc(len*sizeof(WCHAR));
			wcscpy_s( * pszUsername, len, m_strUsername);
			len = m_strPassword.GetLength()+1;
			* pszPassword = (LPWSTR)CoTaskMemAlloc(len*sizeof(WCHAR));
			wcscpy_s( * pszPassword, len, m_strPassword);
		}

		return S_OK;
	}


	HRESULT WINAPI MonitorSink::QueryIAuthenticate(void* pv, REFIID riid, LPVOID* ppv, DWORD dw)
	{
		* ppv = NULL;

		if ( pv && InlineIsEqualGUID(riid, IID_IAuthenticate) )
		{
			MonitorSink * pThis = (MonitorSink *)pv;

			if ( pThis->m_pIEHostWindow && ! pThis->m_strURL.IsEmpty() && pThis->m_spTargetProtocol )
			{
				do 
				{
					CComPtr<IWinInetHttpInfo> spWinInetHttpInfo;
					if ( FAILED(pThis->m_spTargetProtocol->QueryInterface(&spWinInetHttpInfo)) ) break;
					if ( ! spWinInetHttpInfo ) break;

					CHAR szRawHeader[8192];		// IWinInetHttpInfo::QueryInfo() ���ص� Raw Header ���� Unicode ��
					DWORD dwBuffSize = ARRAYSIZE(szRawHeader);

					if ( FAILED(spWinInetHttpInfo->QueryInfo(HTTP_QUERY_RAW_HEADERS, szRawHeader, &dwBuffSize, 0, NULL)) ) break;

					CString strHeader;
					HttpRawHeader2CrLfHeader(szRawHeader, strHeader);

					static const WCHAR AUTH_HEAD [] = L"\r\nWWW-Authenticate:";

					LPWSTR lpAuth = NULL;
					size_t nAuthLen = 0;
					if ( ! ExtractFieldValue( strHeader, AUTH_HEAD, & lpAuth, & nAuthLen ) ) break;
					if ( ! lpAuth ) break;

					CString strAuthScheme;
					CString strAuthRealm;

					// ���������¼��������
					// WWW-Authenticate: Basic realm="Secure Area"
					// WWW-Authenticate: Digest realm="testrealm@host.com", qop="auth,auth-int", nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093", opaque="5ccc069c403ebaf9f0171e9517f40e41"
					// WWW-Authenticate: NTLM
					// WWW-Authenticate: NTLM <auth token>
					LPWSTR pPos = StrStrW(lpAuth, L" ");
					if ( pPos )
					{
						* pPos = L'\0';
						strAuthScheme = lpAuth;

						do 
						{
							pPos = StrStrIW( pPos + 1, L"realm");
							if ( ! pPos ) break;
							pPos = StrChrW( pPos + 5, L'=');
							if ( ! pPos ) break;
							pPos = StrChrW( pPos + 1, L'"');
							if ( ! pPos ) break;
							LPWSTR lpRealm = pPos + 1;
							pPos = StrChrW( lpRealm, L'"');
							if ( ! pPos ) break;
							* pPos = L'\0';

							strAuthRealm = lpRealm;

						} while (false);

					}
					else
					{
						strAuthScheme = lpAuth;
					}

					VirtualFree( lpAuth, 0, MEM_RELEASE);

					// ���� NPN_GetAuthenticationInfo �ò��� NTLM �� domain��û�취����¼��ֻ�ò�֧����
					if (strAuthRealm == _T("NTLM")) return E_NOINTERFACE;

					CUrl url;
					if ( url.CrackUrl(pThis->m_strURL) )
					{
						CW2A aScheme(url.GetSchemeName());
						CW2A aHost(url.GetHostName());
						int aPort = url.GetPortNumber();

						char* username = NULL;
						char* password = NULL;
						uint32_t ulen = 0, plen = 0;

						char* szAuthScheme = CStringToUTF8String(strAuthScheme);
						char* szAuthRealm = CStringToUTF8String(strAuthRealm);
						NPError result = NPN_GetAuthenticationInfo(pThis->m_pIEHostWindow->m_pPlugin->m_pNPInstance, aScheme, aHost, aPort, szAuthScheme, szAuthRealm, &username, &ulen, &password, &plen );
						delete[] szAuthScheme;
						delete[] szAuthRealm;
						if (result != NPERR_NO_ERROR) break;

						pThis->m_strUsername = username;
						pThis->m_strPassword = password;

						NPN_MemFree(username);
						NPN_MemFree(password);
					}

					* ppv = dynamic_cast<IAuthenticate *>(pThis);

					((IUnknown*)*ppv)->AddRef();

					return S_OK;

				} while (false);
			}
		}

		return E_NOINTERFACE;
	}

}