#pragma once

#include <urlmon.h>

class CIEHostWindow;

namespace HttpMonitor 
{
	class MonitorSink:
		public PassthroughAPP::CInternetProtocolSinkWithSP<MonitorSink>,
		public IHttpNegotiate,
		public IAuthenticate
	{
		typedef PassthroughAPP::CInternetProtocolSinkWithSP<MonitorSink> BaseClass;
	public:
		MonitorSink();
		~MonitorSink();

		BEGIN_COM_MAP(MonitorSink)
			COM_INTERFACE_ENTRY(IHttpNegotiate)
			COM_INTERFACE_ENTRY_FUNC(IID_IAuthenticate, 0, QueryIAuthenticate)
			COM_INTERFACE_ENTRY_CHAIN(BaseClass)
		END_COM_MAP()

		BEGIN_SERVICE_MAP(MonitorSink)
			SERVICE_ENTRY(IID_IHttpNegotiate)
			SERVICE_ENTRY(IID_IAuthenticate)
		END_SERVICE_MAP()

		STDMETHODIMP BeginningTransaction(
		LPCWSTR szURL,
		LPCWSTR szHeaders,
		DWORD dwReserved,
		LPWSTR *pszAdditionalHeaders);

		STDMETHODIMP OnResponse(
			DWORD dwResponseCode,
			LPCWSTR szResponseHeaders,
			LPCWSTR szRequestHeaders,
			LPWSTR *pszAdditionalRequestHeaders);

		STDMETHODIMP ReportProgress(
			ULONG ulStatusCode,
			LPCWSTR szStatusText);

		// IAuthenticate
		STDMETHODIMP Authenticate( 
			/* [out] */ HWND *phwnd,
			/* [out] */ LPWSTR *pszUsername,
			/* [out] */ LPWSTR *pszPassword);

		/**
		* Ϊ�˱�֤�����ԣ��ڲ�ȷ���ܷ���ȷ��¼��ʱ������ IAuthenticate �ӿڣ�������IE �ᵯ���Լ��� login �Ի���
		*/
		static HRESULT WINAPI QueryIAuthenticate(void* pv, REFIID riid, LPVOID* ppv, DWORD dw);

	private:
		static CIEHostWindow* s_pLastIEHostWindow;
		// ��ѯ����������Ӧ�� CIEHostWindow ����
		void QueryIEHostWindow();

		// ������Ҫ���Ƶ� Headers ����ȥ
		void SetCustomHeaders(LPWSTR *pszAdditionalHeaders);

		// �� Firefox �е��� Cookie
		void ImportCookies();

		/** �� HTTP Response Headers ��ɨ��� Cookies �����õ� Firefox �� */
		void ExportCookies(LPCWSTR szResponseHeaders);

		/** ��������� URL */
		CString m_strURL;

		/** ��������� Referer */
		CString m_strReferer;

		/** Login �û���*/
		CString m_strUsername;
		/** Login ���� */
		CString m_strPassword;

		/** ���𱾴������ CIEHostWindow ���� */
		CIEHostWindow * m_pIEHostWindow;

		/** �Ƿ���ҳ�������������, ��HTMLҳ�����������ͼƬ���ű��ȵ�������������� */
		bool m_bIsSubRequest;
	};
}