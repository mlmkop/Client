

#include "pch.h"
#include "framework.h"
#include "CSHSensor.h"
#include "CSHSensorDlg.h"
#include "CSHSensorOracleDataBase.h"
#include "CSHSensorCryptography.h"
#include "userenv.h"
#include "sddl.h"
#include "tlhelp32.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma warning(disable:26495) 


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define CSHSENSOR_SESSIONTIMER 900


#define CSHSENSOR_SESSIONTIMEOUT (30 * 60)


#define CSHSENSOR_TRAYNOTIFICATION 9000


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


UINT WM_CSHSENSOR_TRAYNOTIFICATIONCALLBACK = RegisterWindowMessage(_T("TRAYNOTIFICATIONCALLBACK"));


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCSHSensorOracleDataBase *pCSHSensorOracleDataBase = NULL;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCSHSensorCryptography *pCCSHSensorCryptography = NULL;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


UINT ShieldWatchThread(LPVOID lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CCSHSensorDlg *pCCSHSensorDlg = (CCSHSensorDlg *)lpVoid;


	while (TRUE)
	{
		Sleep(1000);


		if (!(pCCSHSensorDlg->currentActiveUserSignInStatus))
		{
			if (!(pCCSHSensorDlg->InvestigateTargetProcess(_T("CSHShield.exe"), &pCCSHSensorDlg->currentActiveUserShieldProcessHandle)))
			{
				pCCSHSensorDlg->CreateTargetProcess(pCCSHSensorDlg->shieldModulePath);
			}
		}
	}


	return UINT();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


UINT SpecificSignOutEventWatchThread(LPVOID lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CCSHSensorDlg *pCCSHSensorDlg = (CCSHSensorDlg *)lpVoid;


	TCHAR currentActiveUserSessionTokenPlain[512];

	memset(currentActiveUserSessionTokenPlain, NULL, sizeof(currentActiveUserSessionTokenPlain));


	_stprintf_s(currentActiveUserSessionTokenPlain, pCCSHSensorDlg->currentActiveUserSessionToken);


	pCCSHSensorCryptography->ARIADecrypt(currentActiveUserSessionTokenPlain);


	CString currentActiveUserProcessSessionEvent;


	currentActiveUserProcessSessionEvent.Format(_T("Global\\CSHSensor|%s"), currentActiveUserSessionTokenPlain);


	while (TRUE)
	{
		HANDLE specificSignOutEventHandle = CreateEvent(NULL, FALSE, FALSE, currentActiveUserProcessSessionEvent.GetBuffer());


		currentActiveUserProcessSessionEvent.ReleaseBuffer();


		if (specificSignOutEventHandle == NULL)
		{
			CString errorMessage;


			errorMessage.Format(_T("CreateEvent Failed, GetLastError Code = %ld"), GetLastError());


			MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


			break;
		}


		ResetEvent(specificSignOutEventHandle);

		
		WaitForSingleObject(specificSignOutEventHandle, INFINITE);


		CloseHandle(specificSignOutEventHandle);


		specificSignOutEventHandle = NULL;


		if (pCCSHSensorDlg->currentActiveUserSignInStatus)
		{
			pCCSHSensorDlg->OnCSHSensorUserSignOut();
		}
	}


	return UINT();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


CCSHSensorDlg::CCSHSensorDlg(CWnd *pParent /*=nullptr*/) : CDialogEx(IDD_CSHSENSOR_DIALOG, pParent), currentActiveUserProcessSessionID(NULL), currentActiveUserSensorMutex(NULL), visibleGUIFlag(FALSE), currentActiveUserNotifyIconData({NULL, }), currentActiveUserSignInStatus(FALSE), currentActiveUserShieldProcessHandle(NULL), preventDuplicateMessageFlag(FALSE), currentActiveUserSessionTimeout(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_CSHSENSOR_ICON);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (!(ProcessIdToSessionId(GetCurrentProcessId(), &currentActiveUserProcessSessionID)))
	{
		CString errorMessage;


		errorMessage.Format(_T("ProcessIdToSessionId Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		exit(NULL);


		return;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CString currentActiveUserSensorMutexName(AfxGetAppName());


	currentActiveUserSensorMutexName.AppendFormat(_T("|%ld"), currentActiveUserProcessSessionID);


	currentActiveUserSensorMutex = CreateMutex(NULL, FALSE, currentActiveUserSensorMutexName.GetBuffer());


	currentActiveUserSensorMutexName.ReleaseBuffer();


	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		exit(NULL);


		return;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	sensorBackgroundImagePath.Empty();


	sensorConfigurationPath.Empty();


	shieldModulePath.Empty();


	currentActiveUserOracleTableName.Empty();


	currentActiveUserRegistryPath.Empty();


	memset(currentActiveUserIPAddress, NULL, sizeof(currentActiveUserIPAddress));


	memset(currentActiveUserMACAddress, NULL, sizeof(currentActiveUserMACAddress));


	memset(currentActiveUserSessionToken, NULL, sizeof(currentActiveUserSessionToken));


	memset(currentActiveUserID, NULL, sizeof(currentActiveUserID));


	memset(currentActiveUserPassword, NULL, sizeof(currentActiveUserPassword));


	currentActiveUserNotificationDisplayID.Empty();


	memset(currentActiveUserSignUpID, NULL, sizeof(currentActiveUserSignUpID));


	memset(currentActiveUserSignUpPassword, NULL, sizeof(currentActiveUserSignUpPassword));


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


CCSHSensorDlg::~CCSHSensorDlg()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (currentActiveUserSignInStatus)
	{
		OnCSHSensorUserSignOut();
	}


	if (pCCSHSensorCryptography != NULL) 
	{
		delete pCCSHSensorCryptography;


		pCCSHSensorCryptography = NULL;
	}


	if (pCSHSensorOracleDataBase != NULL)
	{
		delete pCSHSensorOracleDataBase;


		pCSHSensorOracleDataBase = NULL;
	}


	Shell_NotifyIcon(NIM_DELETE, &currentActiveUserNotifyIconData);


	CloseHandle(currentActiveUserSensorMutex);


	currentActiveUserSensorMutex = NULL;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BEGIN_MESSAGE_MAP(CCSHSensorDlg, CDialogEx)


	ON_WM_PAINT()


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_WM_CTLCOLOR()


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_WM_WINDOWPOSCHANGING()


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_REGISTERED_MESSAGE(WM_CSHSENSOR_TRAYNOTIFICATIONCALLBACK, &CCSHSensorDlg::OnCSHSensorTrayNotificationCallBack)


	ON_REGISTERED_MESSAGE(WM_CSHSENSOR_USERSIGNIN, &CCSHSensorDlg::OnCSHSensorUserSignIn)


	ON_REGISTERED_MESSAGE(WM_CSHSENSOR_USERSIGNUP, &CCSHSensorDlg::OnCSHSensorUserSignUp)


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_COMMAND(IDMC_CSHSENSOR_USERSIGNUP, &CCSHSensorDlg::OnCSHSensorUserSignUp)


	ON_COMMAND(IDMC_CSHSENSOR_USERSIGNOUT, &CCSHSensorDlg::OnCSHSensorUserSignOut)


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_BN_CLICKED(IDC_BUTTON1, &CCSHSensorDlg::OnSignUpConfirm)


	ON_BN_CLICKED(IDC_BUTTON2, &CCSHSensorDlg::OnSignUpCancle)


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_WM_TIMER()


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_WM_QUERYENDSESSION()


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

END_MESSAGE_MAP()


VOID CCSHSensorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);


		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);


		INT cxIcon = GetSystemMetrics(SM_CXICON);

		INT cyIcon = GetSystemMetrics(SM_CYICON);


		CRect rect;


		GetClientRect(&rect);


		INT x = (rect.Width() - cxIcon + 1) / 2;

		INT y = (rect.Height() - cyIcon + 1) / 2;


		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		CPaintDC dc(this);


		CRect rect;

		memset(&rect, NULL, sizeof(rect));


		GetClientRect(&rect);


		CImage managerBackgroundImage;


		if (managerBackgroundImage.Load(sensorBackgroundImagePath.GetBuffer()) != E_FAIL)
		{
			dc.SetStretchBltMode(STRETCH_HALFTONE);


			managerBackgroundImage.StretchBlt(dc.m_hDC, NULL, NULL, rect.Width(), rect.Height());
		}


		sensorBackgroundImagePath.ReleaseBuffer();


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
}


HBRUSH CCSHSensorDlg::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);


	switch (nCtlColor)
	{
		case CTLCOLOR_BTN:
		{
			if (pWnd->GetDlgCtrlID() <= IDC_BUTTON2)
			{
				pDC->SetBkColor(RGB(255, 255, 255));


				pDC->SetBkMode(TRANSPARENT);


				return (HBRUSH)GetStockObject(NULL_BRUSH);
			}


			break;
		}

		case CTLCOLOR_STATIC:
		{
			if (pWnd->GetDlgCtrlID() <= IDC_STATIC2)
			{
				pDC->SetBkMode(TRANSPARENT);


				return (HBRUSH)GetStockObject(NULL_BRUSH);
			}


			break;
		}

		default:
		{
			break;
		}
	}


	return hbr;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::OnWindowPosChanging(WINDOWPOS *lpwndpos)
{
	CDialogEx::OnWindowPosChanging(lpwndpos);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (visibleGUIFlag)
	{
		lpwndpos->flags |= SWP_SHOWWINDOW;
	}
	else
	{	
		lpwndpos->flags &= ~SWP_SHOWWINDOW;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BOOL CCSHSensorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	SetIcon(m_hIcon, TRUE);

	SetIcon(m_hIcon, FALSE);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CreateTrayNotification();


	PathConfiguration();


	LoadOracleDataBaseConfiguration();


	AuthenticateStatusPathConfiguration();


	AfxBeginThread(ShieldWatchThread, this);


	AfxBeginThread(SpecificSignOutEventWatchThread, this);


	return TRUE;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DDX_Control(pDX, IDC_STATIC1, signUpIDStatic);


	DDX_Control(pDX, IDC_EDIT1, signUpIDEdit);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DDX_Control(pDX, IDC_STATIC2, signUpPasswordStatic);


	DDX_Control(pDX, IDC_EDIT2, signUpPasswordEdit);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DDX_Control(pDX, IDC_BUTTON1, signUpConfirmButton);


	DDX_Control(pDX, IDC_BUTTON2, signUpCancleButton);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BOOL CCSHSensorDlg::PreTranslateMessage(MSG *pMsg)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
	{
		return TRUE;
	}


	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN))
	{
		return TRUE;
	}


	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_CANCEL))
	{
		return TRUE;
	}


	if ((pMsg->message == WM_SYSKEYDOWN) && (pMsg->wParam == VK_F4))
	{
		return TRUE;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	return CDialogEx::PreTranslateMessage(pMsg);
}


VOID CCSHSensorDlg::CreateTrayNotification()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	currentActiveUserNotifyIconData.cbSize = sizeof(currentActiveUserNotifyIconData);


	currentActiveUserNotifyIconData.hWnd = theApp.m_pMainWnd->GetSafeHwnd();


	currentActiveUserNotifyIconData.uID = CSHSENSOR_TRAYNOTIFICATION;


	currentActiveUserNotifyIconData.uFlags = (NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO);


	currentActiveUserNotifyIconData.uCallbackMessage = WM_CSHSENSOR_TRAYNOTIFICATIONCALLBACK;


	currentActiveUserNotifyIconData.hIcon = theApp.LoadIcon(IDI_CSHSENSOR_ICON);


	currentActiveUserNotifyIconData.uTimeout = (UINT)3000;


	currentActiveUserNotifyIconData.dwInfoFlags = (NIIF_INFO | NIIF_NOSOUND);


	_stprintf_s(currentActiveUserNotifyIconData.szInfoTitle, _T("CSH Data Loss Prevention Solution"));


	_stprintf_s(currentActiveUserNotifyIconData.szInfo, _T("Trying To Connect To The DataBase..."));


	_stprintf_s(currentActiveUserNotifyIconData.szTip, _T("CSH Data Loss Prevention Solution"));


	Shell_NotifyIcon(NIM_ADD, &currentActiveUserNotifyIconData);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::PathConfiguration()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	TCHAR sensorModulePath[MAX_PATH];

	memset(sensorModulePath, NULL, sizeof(sensorModulePath));


	DWORD dwSensorModulePath = GetModuleFileName(NULL, sensorModulePath, (DWORD)(MAX_PATH - 1));


	if ((dwSensorModulePath == NULL) || (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		CString errorMessage;


		errorMessage.Format(_T("GetModuleFileName Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	if (!(PathRemoveFileSpec(sensorModulePath))) 
	{
		CString errorMessage;


		errorMessage.Format(_T("PathRemoveFileSpec Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	sensorBackgroundImagePath.Format(_T("%s\\CSHSensor.png"), sensorModulePath);


	sensorConfigurationPath.Format(_T("%s\\CSHSensorConfiguration.ini"), sensorModulePath);


	shieldModulePath.Format(_T("%s\\CSHShield.exe"), sensorModulePath);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::LoadOracleDataBaseConfiguration()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	TCHAR configuredOracleUserID[128];

	memset(configuredOracleUserID, NULL, sizeof(configuredOracleUserID));


	GetPrivateProfileString(_T("CONFIGURATION"), _T("ORACLEUSERNAME"), NULL, configuredOracleUserID, (DWORD)(128 - 1), sensorConfigurationPath.GetBuffer());


	sensorConfigurationPath.ReleaseBuffer();


	TCHAR configuredOracleUserPassword[128];

	memset(configuredOracleUserPassword, NULL, sizeof(configuredOracleUserPassword));


	GetPrivateProfileString(_T("CONFIGURATION"), _T("ORACLEUSERPASSWORD"), NULL, configuredOracleUserPassword, (DWORD)(128 - 1), sensorConfigurationPath.GetBuffer());


	sensorConfigurationPath.ReleaseBuffer();


	TCHAR configuredOracleTableName[128];

	memset(configuredOracleTableName, NULL, sizeof(configuredOracleTableName));


	GetPrivateProfileString(_T("CONFIGURATION"), _T("ORACLETABLENAME"), NULL, configuredOracleTableName, (DWORD)(128 - 1), sensorConfigurationPath.GetBuffer());


	sensorConfigurationPath.ReleaseBuffer();


	currentActiveUserOracleTableName.Format(_T("%s"), configuredOracleTableName);


	TCHAR configuredOracleServiceName[128];

	memset(configuredOracleServiceName, NULL, sizeof(configuredOracleServiceName));


	GetPrivateProfileString(_T("CONFIGURATION"), _T("ORACLESERVICENAME"), NULL, configuredOracleServiceName, (DWORD)(128 - 1), sensorConfigurationPath.GetBuffer());


	sensorConfigurationPath.ReleaseBuffer();


	pCSHSensorOracleDataBase = new CCSHSensorOracleDataBase(configuredOracleUserID, configuredOracleUserPassword, configuredOracleTableName, configuredOracleServiceName);


	if (pCSHSensorOracleDataBase == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("new Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	pCSHSensorOracleDataBase->CreateOracleInstanceEnvironment();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::AuthenticateStatusPathConfiguration()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	HANDLE currentActiveUserProcessToken = NULL;


	if (!(OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &currentActiveUserProcessToken)))
	{
		CString errorMessage;


		errorMessage.Format(_T("OpenProcessToken Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	DWORD dwQueryTokenUser = NULL;


	GetTokenInformation(currentActiveUserProcessToken, TOKEN_INFORMATION_CLASS::TokenUser, NULL, NULL, &dwQueryTokenUser);


	PTOKEN_USER pQueryTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, (size_t)dwQueryTokenUser);


	if (pQueryTokenUser == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("LocalAlloc Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		CloseHandle(currentActiveUserProcessToken);


		currentActiveUserProcessToken = NULL;


		EndDialog(NULL);


		return;
	}


	if (!(GetTokenInformation(currentActiveUserProcessToken, TOKEN_INFORMATION_CLASS::TokenUser, (LPVOID)pQueryTokenUser, dwQueryTokenUser, &dwQueryTokenUser)))
	{
		CString errorMessage;


		errorMessage.Format(_T("GetTokenInformation Failed, GetLastError = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		LocalFree(pQueryTokenUser);


		pQueryTokenUser = NULL;


		CloseHandle(currentActiveUserProcessToken);


		currentActiveUserProcessToken = NULL;


		EndDialog(NULL);


		return;
	}


	CloseHandle(currentActiveUserProcessToken);


	currentActiveUserProcessToken = NULL;


	LPTSTR pSID = NULL;


	if (!(ConvertSidToStringSid(pQueryTokenUser->User.Sid, &pSID)))
	{
		CString errorMessage;


		errorMessage.Format(_T("ConvertSidToStringSid Failed, GetLastError = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		LocalFree(pQueryTokenUser);


		pQueryTokenUser = NULL;


		EndDialog(NULL);


		return;
	}


	currentActiveUserRegistryPath.Format(_T("%s\\SOFTWARE\\CSHDLP"), pSID);


	LocalFree(pSID);


	pSID = NULL;


	LocalFree(pQueryTokenUser);


	pQueryTokenUser = NULL;


	LSTATUS currentActiveUserRegistryStatus = currentActiveUserRegistryController.Open(HKEY_USERS, currentActiveUserRegistryPath.GetBuffer(), KEY_ALL_ACCESS);


	currentActiveUserRegistryPath.ReleaseBuffer();


	if (currentActiveUserRegistryStatus != ERROR_SUCCESS)
	{
		CString errorMessage;


		errorMessage.Format(_T("Unable To Verify System Session Information, Please Check Module \"CSHService.exe\""));


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	pCCSHSensorCryptography = new CCSHSensorCryptography();


	if (pCSHSensorOracleDataBase == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("new Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	DWORD dwCurrentActiveUser = (DWORD)(512 - 1);


	currentActiveUserRegistryController.QueryStringValue(_T("userIPAddress"), currentActiveUserIPAddress, &dwCurrentActiveUser);
	
	
	pCCSHSensorCryptography->ARIAEncrypt(currentActiveUserIPAddress);


	dwCurrentActiveUser = (DWORD)(512 - 1);


	currentActiveUserRegistryController.QueryStringValue(_T("userMACAddress"), currentActiveUserMACAddress, &dwCurrentActiveUser);
	
	
	pCCSHSensorCryptography->ARIAEncrypt(currentActiveUserMACAddress);


	dwCurrentActiveUser = (DWORD)(512 - 1);


	currentActiveUserRegistryController.QueryStringValue(_T("userSessionToken"), currentActiveUserSessionToken, &dwCurrentActiveUser);


	pCCSHSensorCryptography->ARIAEncrypt(currentActiveUserSessionToken);


	currentActiveUserRegistryController.Close();


	RegDeleteKey(HKEY_USERS, currentActiveUserRegistryPath.GetBuffer());


	currentActiveUserRegistryPath.ReleaseBuffer();
	

	CString signOutSQLTransactionStatementBuillder;


	signOutSQLTransactionStatementBuillder.Format(_T("UPDATE %s SET USER_TIMEOUTSTAMP=NULL, USER_IPADDRESS=NULL, USER_MACADDRESS=NULL, USER_SESSIONTOKEN=NULL, USER_SIGNIN='%s' WHERE USER_SESSIONTOKEN='%s'"), currentActiveUserOracleTableName.GetBuffer(), pCCSHSensorCryptography->cryptedWideStringN, currentActiveUserSessionToken);


	currentActiveUserOracleTableName.ReleaseBuffer();


	pCSHSensorOracleDataBase->CreateOracleInstanceConnection();


	pCSHSensorOracleDataBase->CreateOracleInstanceStatement(signOutSQLTransactionStatementBuillder.GetBuffer());


	signOutSQLTransactionStatementBuillder.ReleaseBuffer();


	if (pCSHSensorOracleDataBase->excuteOracleInstanceStatement())
	{
		CString notifyInformationMessage;


		notifyInformationMessage.Format(_T("Copyright (C) 2024 CSH, All Rights Reserved"));


		_stprintf_s(currentActiveUserNotifyIconData.szInfo, notifyInformationMessage.GetBuffer());


		notifyInformationMessage.ReleaseBuffer();


		Shell_NotifyIcon(NIM_MODIFY, &currentActiveUserNotifyIconData);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BOOL CCSHSensorDlg::InvestigateTargetProcess(LPCTSTR targetProcessName, HWND *pCurrentActiveUserProcessHandle)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	BOOL returnCode = FALSE;


	PROCESSENTRY32 processEntry32;

	memset(&processEntry32, NULL, sizeof(processEntry32));


	processEntry32.dwSize = sizeof(processEntry32);


	HANDLE toolhelp32SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);


	if (toolhelp32SnapshotHandle == INVALID_HANDLE_VALUE)
	{
		CString errorMessage;


		errorMessage.Format(_T("CreateToolhelp32Snapshot Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		return returnCode;
	}


	if (Process32First(toolhelp32SnapshotHandle, &processEntry32))
	{
		do
		{
			if (_tcscmp(processEntry32.szExeFile, targetProcessName) == NULL)
			{
				DWORD dwCurrentActiveSessionID = NULL;


				if (!(ProcessIdToSessionId(processEntry32.th32ProcessID, &dwCurrentActiveSessionID)))
				{
					CString errorMessage;


					errorMessage.Format(_T("ProcessIdToSessionId Failed, GetLastError Code = %ld"), GetLastError());


					MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


					break;
				}


				if (currentActiveUserProcessSessionID == dwCurrentActiveSessionID)
				{
					CloseHandle(toolhelp32SnapshotHandle);


					toolhelp32SnapshotHandle = NULL;


					if (*(pCurrentActiveUserProcessHandle) == NULL)
					{
						TCHAR targetProcessCaption[128];

						memset(targetProcessCaption, NULL, sizeof(targetProcessCaption));


						_stprintf_s(targetProcessCaption, targetProcessName);


						PathRemoveExtension(targetProcessCaption);


						DWORD dwTargetProcessWindowWindowThreadProcessId = NULL;


						CWnd *pTargetProcessWindow = FindWindow(NULL, targetProcessCaption);


						while (pTargetProcessWindow != NULL)
						{
							dwTargetProcessWindowWindowThreadProcessId = NULL;


							GetWindowThreadProcessId(pTargetProcessWindow->GetSafeHwnd(), &dwTargetProcessWindowWindowThreadProcessId);


							if (processEntry32.th32ProcessID == dwTargetProcessWindowWindowThreadProcessId)
							{
								*(pCurrentActiveUserProcessHandle) = pTargetProcessWindow->GetSafeHwnd();


								break;
							}


							pTargetProcessWindow = GetWindow(GW_HWNDNEXT);
						}
					}


					returnCode = TRUE;


					return returnCode;
				}
			}

		} while (Process32Next(toolhelp32SnapshotHandle, &processEntry32));
	}


	*(pCurrentActiveUserProcessHandle) = NULL;


	CloseHandle(toolhelp32SnapshotHandle);


	toolhelp32SnapshotHandle = NULL;


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::CreateTargetProcess(LPCTSTR targetProcessName)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	STARTUPINFO startUpInfo;

	memset(&startUpInfo, NULL, sizeof(startUpInfo));


	startUpInfo.cb = sizeof(startUpInfo);


	PROCESS_INFORMATION processInformation;

	memset(&processInformation, NULL, sizeof(processInformation));


	CString commandLine;


	commandLine.Format(_T("%s"), targetProcessName);


	BOOL bCreateProcess = CreateProcess(NULL, commandLine.GetBuffer(), NULL, NULL, FALSE, NULL, NULL, NULL, &startUpInfo, &processInformation);


	commandLine.ReleaseBuffer();


	if (!(bCreateProcess))
	{
		CString errorMessage;


		errorMessage.Format(_T("CreateProcess Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		return;
	}


	CloseHandle(processInformation.hProcess);


	CloseHandle(processInformation.hThread);


	memset(&processInformation, NULL, sizeof(processInformation));


	if (currentActiveUserShieldProcessHandle != NULL) 
	{
		CloseHandle(currentActiveUserShieldProcessHandle);


		currentActiveUserShieldProcessHandle = NULL;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BOOL CCSHSensorDlg::InvestigateUserAuthenticateStatus()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	BOOL returnCode = FALSE;


	if (currentActiveUserRegistryController.Open(HKEY_USERS, currentActiveUserRegistryPath, KEY_ALL_ACCESS) == ERROR_SUCCESS)
	{
		memset(currentActiveUserID, NULL, sizeof(currentActiveUserID));


		memset(currentActiveUserPassword, NULL, sizeof(currentActiveUserPassword));


		TCHAR queryBuffer[512];

		memset(queryBuffer, NULL, sizeof(queryBuffer));


		DWORD dwQueryBuffer = (DWORD)(512 - 1);


		CString signInSQLTransactionStatementBuillder;


		signInSQLTransactionStatementBuillder.Format(_T("UPDATE %s SET "), currentActiveUserOracleTableName.GetBuffer());


		currentActiveUserOracleTableName.ReleaseBuffer();

						
		if (currentActiveUserRegistryController.QueryStringValue(_T("userID"), queryBuffer, &dwQueryBuffer) == ERROR_SUCCESS)
		{
			pCCSHSensorCryptography->ARIAEncrypt(queryBuffer);


			_stprintf_s(currentActiveUserID, queryBuffer);


			memset(queryBuffer, NULL, sizeof(queryBuffer));


			dwQueryBuffer = (DWORD)(512 - 1);

						
			if (currentActiveUserRegistryController.QueryStringValue(_T("userPassword"), queryBuffer, &dwQueryBuffer) == ERROR_SUCCESS)
			{
				pCCSHSensorCryptography->ARIAEncrypt(queryBuffer);


				_stprintf_s(currentActiveUserPassword, queryBuffer);


				memset(queryBuffer, NULL, sizeof(queryBuffer));


				dwQueryBuffer = (DWORD)(512 - 1);


				CString investigateSQLTransactionStatementBuillder;


				investigateSQLTransactionStatementBuillder.Format(_T("SELECT USER_SIGNUP, USER_SIGNUPSTATUS, USER_SIGNIN, USER_TIMESTAMP, USER_SIGNINCOUNT, USER_PASSWORD FROM %s WHERE USER_ID='%s'"), currentActiveUserOracleTableName.GetBuffer(), currentActiveUserID);


				currentActiveUserOracleTableName.ReleaseBuffer();


				pCSHSensorOracleDataBase->CreateOracleInstanceConnection();


				pCSHSensorOracleDataBase->CreateOracleInstanceStatement(investigateSQLTransactionStatementBuillder.GetBuffer());


				investigateSQLTransactionStatementBuillder.ReleaseBuffer();


				returnCode = pCSHSensorOracleDataBase->excuteOracleInstanceStatement(currentActiveUserID, currentActiveUserPassword);


				switch (returnCode)
				{
					case TRUE:
					{
						if (currentActiveUserRegistryController.QueryStringValue(_T("userTimeStamp"), queryBuffer, &dwQueryBuffer) == ERROR_SUCCESS)
						{
							pCCSHSensorCryptography->ARIAEncrypt(queryBuffer);


							signInSQLTransactionStatementBuillder.AppendFormat(_T("USER_TIMESTAMP='%s', "), queryBuffer);


							memset(queryBuffer, NULL, sizeof(queryBuffer));


							dwQueryBuffer = (DWORD)(512 - 1);


							if (currentActiveUserRegistryController.QueryStringValue(_T("userTimeOutStamp"), queryBuffer, &dwQueryBuffer) == ERROR_SUCCESS)
							{
								pCCSHSensorCryptography->ARIAEncrypt(queryBuffer);


								signInSQLTransactionStatementBuillder.AppendFormat(_T("USER_TIMEOUTSTAMP='%s', USER_IPADDRESS='%s', USER_MACADDRESS='%s', USER_SESSIONTOKEN='%s', USER_SIGNIN='%s', USER_SIGNINCOUNT='%s' WHERE USER_ID='%s'"), queryBuffer, currentActiveUserIPAddress, currentActiveUserMACAddress, currentActiveUserSessionToken, pCCSHSensorCryptography->cryptedWideStringY, pCCSHSensorCryptography->cryptedWideString0, currentActiveUserID);


								pCSHSensorOracleDataBase->CreateOracleInstanceConnection();


								pCSHSensorOracleDataBase->CreateOracleInstanceStatement(signInSQLTransactionStatementBuillder.GetBuffer());


								signInSQLTransactionStatementBuillder.ReleaseBuffer();


								returnCode = pCSHSensorOracleDataBase->excuteOracleInstanceStatement();
							}
						}


						break;
					}

					case FALSE:
					{
						break;
					}

					default:
					{
						OnCSHSensorUserSignUp();


						returnCode = FALSE;


						break;
					}
				}
			}
		}


		if (returnCode)
		{
			TCHAR currentActiveUserIDPlain[512];

			memset(currentActiveUserIDPlain, NULL, sizeof(currentActiveUserIDPlain));


			_stprintf_s(currentActiveUserIDPlain, currentActiveUserID);


			pCCSHSensorCryptography->ARIADecrypt(currentActiveUserIDPlain);


			currentActiveUserNotificationDisplayID.Format(_T("%s"), currentActiveUserIDPlain);
		}


		currentActiveUserRegistryController.Close();


		RegDeleteKey(HKEY_USERS, currentActiveUserRegistryPath);
	}
	else 
	{
		MessageBox(_T("Unable To Verify User Sign In Information, Please Check Module \"CSHShield.exe\""), _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));
	}


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::SignUpDisplay()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CRect currentSignUpDisplayRect;

	memset(&currentSignUpDisplayRect, NULL, sizeof(currentSignUpDisplayRect));


	GetClientRect(currentSignUpDisplayRect);


	DOUBLE currentSignUpDisplayWidth = (DOUBLE)currentSignUpDisplayRect.Width();


	DOUBLE currentSignUpDisplayHeight = (DOUBLE)currentSignUpDisplayRect.Height();


	DOUBLE currentSignUpDisplayLeft = NULL;


	DOUBLE currentSignUpDisplayTop = NULL;


	currentSignUpDisplayLeft = (currentSignUpDisplayWidth * 0.54);


	currentSignUpDisplayTop = (currentSignUpDisplayHeight * 0.73);


	signUpIDStatic.MoveWindow((INT)currentSignUpDisplayLeft, (INT)currentSignUpDisplayTop, (INT)(currentSignUpDisplayWidth * 0.2), (INT)(currentSignUpDisplayHeight * 0.1));


	signUpIDStatic.SetWindowText(_T("ID :"));


	currentSignUpDisplayLeft = (currentSignUpDisplayWidth * 0.585);


	currentSignUpDisplayTop = (currentSignUpDisplayHeight * 0.72);


	signUpIDEdit.MoveWindow((INT)currentSignUpDisplayLeft, (INT)currentSignUpDisplayTop, (INT)(currentSignUpDisplayWidth * 0.375), (INT)(currentSignUpDisplayHeight * 0.065));


	signUpIDEdit.SetWindowText(NULL);


	signUpIDEdit.SetFocus();


	currentSignUpDisplayLeft = (currentSignUpDisplayWidth * 0.528);


	currentSignUpDisplayTop = (currentSignUpDisplayHeight * 0.8);


	signUpPasswordStatic.MoveWindow((INT)currentSignUpDisplayLeft, (INT)currentSignUpDisplayTop, (INT)(currentSignUpDisplayWidth * 0.2), (INT)(currentSignUpDisplayHeight * 0.1));


	signUpPasswordStatic.SetWindowText(_T("PW :"));


	currentSignUpDisplayLeft = (currentSignUpDisplayWidth * 0.585);


	currentSignUpDisplayTop = (currentSignUpDisplayHeight * 0.79);


	signUpPasswordEdit.MoveWindow((INT)currentSignUpDisplayLeft, (INT)currentSignUpDisplayTop, (INT)(currentSignUpDisplayWidth * 0.375), (INT)(currentSignUpDisplayHeight * 0.065));


	signUpPasswordEdit.SetWindowText(NULL);


	signUpPasswordEdit.SetPasswordChar('*');


	currentSignUpDisplayLeft = (currentSignUpDisplayWidth * 0.65);


	currentSignUpDisplayTop = (currentSignUpDisplayHeight * 0.865);


	signUpConfirmButton.MoveWindow((INT)currentSignUpDisplayLeft, (INT)currentSignUpDisplayTop, (INT)(currentSignUpDisplayWidth * 0.15), (INT)(currentSignUpDisplayHeight * 0.075));


	signUpConfirmButton.SetWindowText(_T("Confirm"));


	currentSignUpDisplayLeft = (currentSignUpDisplayWidth * 0.81);


	currentSignUpDisplayTop = (currentSignUpDisplayHeight * 0.865);


	signUpCancleButton.MoveWindow((INT)currentSignUpDisplayLeft, (INT)currentSignUpDisplayTop, (INT)(currentSignUpDisplayWidth * 0.15), (INT)(currentSignUpDisplayHeight * 0.075));


	signUpCancleButton.SetWindowText(_T("Cancle"));


	visibleGUIFlag = TRUE;


	::ShowWindow(currentActiveUserShieldProcessHandle, SW_HIDE);


	::ShowWindow(theApp.m_pMainWnd->GetSafeHwnd(), SW_SHOW);


	::SetForegroundWindow(theApp.m_pMainWnd->GetSafeHwnd());


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::GetSystemScreenSaveConfiguration() 
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	TCHAR queryBuffer[128];

	memset(queryBuffer, NULL, sizeof(queryBuffer));


	DWORD dwQueryBuffer = (DWORD)(128 - 1);


	CRegKey currentActiveUserSystemRegistryController;


	if (currentActiveUserSystemRegistryController.Open(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"), KEY_ALL_ACCESS) == ERROR_SUCCESS)
	{
		if (currentActiveUserSystemRegistryController.QueryStringValue(_T("SCRNSAVE.EXE"), queryBuffer, &dwQueryBuffer) == ERROR_SUCCESS)
		{
			DWORD dwScreenSaveTimeOut = NULL;


			if (!(SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, NULL, &dwScreenSaveTimeOut, NULL))) 
			{
				currentActiveUserSessionTimeout = CSHSENSOR_SESSIONTIMEOUT;
			}
			else
			{
				currentActiveUserSessionTimeout = (ULONGLONG)dwScreenSaveTimeOut;
			}
		}
		else
		{
			currentActiveUserSessionTimeout = CSHSENSOR_SESSIONTIMEOUT;
		}


		currentActiveUserSystemRegistryController.Close();
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


LRESULT CCSHSensorDlg::OnCSHSensorTrayNotificationCallBack(WPARAM wParam, LPARAM lParam)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (lParam == WM_RBUTTONUP)
	{
		CMenu popUpMenu;


		if (popUpMenu.LoadMenu(IDM_CSHSENSOR_MENU) != NULL)
		{
			if (!(SetForegroundWindow()))
			{
				return LRESULT();
			}


			CPoint currentPoint;

			memset(&currentPoint, NULL, sizeof(currentPoint));


			if (!(GetCursorPos(&currentPoint)))
			{
				return LRESULT();
			}


			CMenu *pPopUpMenu = popUpMenu.GetSubMenu(NULL);


			if (pPopUpMenu != NULL)
			{
				if (currentActiveUserSignInStatus)
				{
					pPopUpMenu->EnableMenuItem(IDMC_CSHSENSOR_USERSIGNOUT, MFS_ENABLED);
				}
				else
				{
					pPopUpMenu->EnableMenuItem(IDMC_CSHSENSOR_USERSIGNOUT, MFS_DISABLED);
				}


				pPopUpMenu->TrackPopupMenu((TPM_RIGHTBUTTON | TPM_RIGHTALIGN), currentPoint.x, currentPoint.y, this);
			}
		}
	}


	return LRESULT();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


LRESULT CCSHSensorDlg::OnCSHSensorUserSignIn(WPARAM wParam, LPARAM lParam)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (!(preventDuplicateMessageFlag))
	{
		preventDuplicateMessageFlag = TRUE;


		currentActiveUserSignInStatus = InvestigateUserAuthenticateStatus();


		if (currentActiveUserSignInStatus)
		{
			::SendMessage(currentActiveUserShieldProcessHandle, WM_CLOSE, NULL, NULL);


			#ifndef _UNICODE


				CHAR targetBlockReason[128];

				memset(targetBlockReason, NULL, sizeof(targetBlockReason));


				_stprintf_s(targetBlockReason, _T("Sign Out Is Required, Before Shutting Down The System"));


				INT convertedtargetBlockReasonLength = MultiByteToWideChar(CP_ACP, NULL, targetBlockReason, (INT)strlen(targetBlockReason), NULL, NULL);


				WCHAR convertedBlockReason[128];

				memset(convertedBlockReason, NULL, sizeof(convertedBlockReason));


				MultiByteToWideChar(CP_ACP, NULL, targetBlockReason, (INT)strlen(targetBlockReason), convertedBlockReason, convertedtargetBlockReasonLength);


				if (!(ShutdownBlockReasonCreate(theApp.m_pMainWnd->GetSafeHwnd(), convertedBlockReason)))
				{


			#else


				if (!(ShutdownBlockReasonCreate(theApp.m_pMainWnd->GetSafeHwnd(), _T("Sign Out Is Required, Before Shutting Down The System"))))
				{


			#endif

			
					CString errorMessage;


					errorMessage.Format(_T("ShutdownBlockReasonCreate Failed, GetLastError Code = %ld"), GetLastError());


					MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


					EndDialog(NULL);


					return LRESULT();
				}


			CString notifyInformationMessage(currentActiveUserNotificationDisplayID);


			notifyInformationMessage.Append(_T(" Sign In Complete"));


			_stprintf_s(currentActiveUserNotifyIconData.szInfo, notifyInformationMessage.GetBuffer());


			notifyInformationMessage.ReleaseBuffer();


			Shell_NotifyIcon(NIM_MODIFY, &currentActiveUserNotifyIconData);


			SetTimer(CSHSENSOR_SESSIONTIMER, (UINT)(30 * 1000), NULL);
		}


		preventDuplicateMessageFlag = FALSE;
	}


	return LRESULT();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


LRESULT CCSHSensorDlg::OnCSHSensorUserSignUp(WPARAM wParam, LPARAM lParam)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	SignUpDisplay();


	return LRESULT();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::OnCSHSensorUserSignUp()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	SignUpDisplay();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::OnCSHSensorUserSignOut()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CString signOutSQLTransactionStatementBuillder;


	signOutSQLTransactionStatementBuillder.Format(_T("UPDATE %s SET USER_TIMEOUTSTAMP=NULL, USER_IPADDRESS=NULL, USER_MACADDRESS=NULL, USER_SESSIONTOKEN=NULL, USER_SIGNIN='%s' WHERE USER_SESSIONTOKEN='%s'"), currentActiveUserOracleTableName.GetBuffer(), pCCSHSensorCryptography->cryptedWideStringN, currentActiveUserSessionToken);


	currentActiveUserOracleTableName.ReleaseBuffer();


	pCSHSensorOracleDataBase->CreateOracleInstanceConnection();


	pCSHSensorOracleDataBase->CreateOracleInstanceStatement(signOutSQLTransactionStatementBuillder.GetBuffer());


	signOutSQLTransactionStatementBuillder.ReleaseBuffer();


	if (pCSHSensorOracleDataBase->excuteOracleInstanceStatement())
	{
		if (theApp.m_pMainWnd->GetSafeHwnd() != NULL)
		{
			KillTimer(CSHSENSOR_SESSIONTIMER);
		}


		if (theApp.m_pMainWnd->GetSafeHwnd() != NULL)
		{
			if (!(ShutdownBlockReasonDestroy(theApp.m_pMainWnd->GetSafeHwnd())))
			{
				CString errorMessage;


				errorMessage.Format(_T("ShutdownBlockReasonDestroy Failed, GetLastError Code = %ld"), GetLastError());


				MessageBox(errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


				EndDialog(NULL);


				return;
			}
		}


		CString notifyInformationMessage(currentActiveUserNotificationDisplayID);


		notifyInformationMessage.Append(_T(" Sign Out Complete"));


		_stprintf_s(currentActiveUserNotifyIconData.szInfo, notifyInformationMessage.GetBuffer());


		notifyInformationMessage.ReleaseBuffer();


		Shell_NotifyIcon(NIM_MODIFY, &currentActiveUserNotifyIconData);


		currentActiveUserSignInStatus = FALSE;
	}
	

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::OnSignUpConfirm()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CString currentActiveUserInputtedSignUpID;


	signUpIDEdit.GetWindowText(currentActiveUserInputtedSignUpID);


	if (currentActiveUserInputtedSignUpID.IsEmpty())
	{
		signUpIDEdit.SetFocus();


		return;
	}


	CString currentActiveUserInputtedSignUpPassword;


	signUpPasswordEdit.GetWindowText(currentActiveUserInputtedSignUpPassword);


	if (currentActiveUserInputtedSignUpPassword.IsEmpty())
	{
		signUpPasswordEdit.SetFocus();


		return;
	}


	if (currentActiveUserInputtedSignUpPassword.GetLength() < 8)
	{
		MessageBox(_T("Please Check The Password Policy For Sign Up"), _T("CSHSensor"), (MB_OK | MB_ICONWARNING | MB_TOPMOST));


		signUpPasswordEdit.SetWindowText(NULL);


		signUpPasswordEdit.SetFocus();


		return;
	}


	memset(currentActiveUserSignUpID, NULL, sizeof(currentActiveUserSignUpID));


	_stprintf_s(currentActiveUserSignUpID, currentActiveUserInputtedSignUpID.GetBuffer());


	currentActiveUserInputtedSignUpID.ReleaseBuffer();


	pCCSHSensorCryptography->ARIAEncrypt(currentActiveUserSignUpID);


	memset(currentActiveUserSignUpPassword, NULL, sizeof(currentActiveUserSignUpPassword));


	_stprintf_s(currentActiveUserSignUpPassword, currentActiveUserInputtedSignUpPassword.GetBuffer());


	currentActiveUserInputtedSignUpPassword.ReleaseBuffer();


	pCCSHSensorCryptography->ARIAEncrypt(currentActiveUserSignUpPassword);


	CString investigateSQLTransactionStatementBuillder;


	investigateSQLTransactionStatementBuillder.Format(_T("SELECT USER_ID, USER_SIGNUP FROM %s WHERE USER_ID='%s'"), currentActiveUserOracleTableName.GetBuffer(), currentActiveUserSignUpID);


	currentActiveUserOracleTableName.ReleaseBuffer();


	pCSHSensorOracleDataBase->CreateOracleInstanceConnection();


	pCSHSensorOracleDataBase->CreateOracleInstanceStatement(investigateSQLTransactionStatementBuillder.GetBuffer());


	investigateSQLTransactionStatementBuillder.ReleaseBuffer();


	if (pCSHSensorOracleDataBase->excuteOracleInstanceStatement())
	{		
		CString signUpSQLTransactionStatementBuillder;


		signUpSQLTransactionStatementBuillder.Format(_T("UPDATE %s SET USER_PASSWORD='%s', USER_SIGNUP='%s' WHERE USER_ID='%s'"), currentActiveUserOracleTableName.GetBuffer(), currentActiveUserSignUpPassword, pCCSHSensorCryptography->cryptedWideStringN, currentActiveUserSignUpID);


		currentActiveUserOracleTableName.ReleaseBuffer();


		pCSHSensorOracleDataBase->CreateOracleInstanceConnection();


		pCSHSensorOracleDataBase->CreateOracleInstanceStatement(signUpSQLTransactionStatementBuillder.GetBuffer());


		signUpSQLTransactionStatementBuillder.ReleaseBuffer();


		if (pCSHSensorOracleDataBase->excuteOracleInstanceStatement())
		{				
			MessageBox(_T("Sign Up Complete"), _T("CSHSensor"), (MB_OK | MB_ICONINFORMATION | MB_TOPMOST));
		}
	}


	OnSignUpCancle();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::OnSignUpCancle()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	visibleGUIFlag = FALSE;


	::ShowWindow(theApp.m_pMainWnd->GetSafeHwnd(), SW_HIDE);


	::ShowWindow(currentActiveUserShieldProcessHandle, SW_SHOW);


	::SetForegroundWindow(currentActiveUserShieldProcessHandle);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorDlg::OnTimer(UINT_PTR nIDEvent)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	switch (nIDEvent)
	{
		case CSHSENSOR_SESSIONTIMER:
		{
			GetSystemScreenSaveConfiguration();


			LASTINPUTINFO currentActiveUserLastInputInformation;

			memset(&currentActiveUserLastInputInformation, NULL, sizeof(currentActiveUserLastInputInformation));


			currentActiveUserLastInputInformation.cbSize = sizeof(currentActiveUserLastInputInformation);


			if (!(GetLastInputInfo(&currentActiveUserLastInputInformation))) 
			{
				break;
			}


			if ((GetTickCount64() - (ULONGLONG)currentActiveUserLastInputInformation.dwTime) > (currentActiveUserSessionTimeout * (ULONGLONG)1000))
			{
				OnCSHSensorUserSignOut();


				break;
			}


			if ((GetTickCount64() - (ULONGLONG)currentActiveUserLastInputInformation.dwTime) < (ULONGLONG)(30 * 1000))
			{
				CTime currentActiveUserSignInTimeOut = (CTime::GetCurrentTime() + CTimeSpan(NULL, NULL, 30, NULL));


				TCHAR currentActiveUserSignInTimeOutStamp[512];
				
				memset(currentActiveUserSignInTimeOutStamp, NULL, sizeof(currentActiveUserSignInTimeOutStamp)); 
				
				
				CString currentActiveUserSignInTimeOutStampBuffer = currentActiveUserSignInTimeOut.Format(_T("%Y-%m-%d-%H-%M-%S"));


				_stprintf_s(currentActiveUserSignInTimeOutStamp, currentActiveUserSignInTimeOutStampBuffer.GetBuffer());


				currentActiveUserSignInTimeOutStampBuffer.ReleaseBuffer();


				pCCSHSensorCryptography->ARIAEncrypt(currentActiveUserSignInTimeOutStamp);


				CString timeOutSQLTransactionStatementBuillder;


				timeOutSQLTransactionStatementBuillder.Format(_T("UPDATE %s SET USER_TIMEOUTSTAMP='%s' WHERE USER_ID='%s'"), currentActiveUserOracleTableName.GetBuffer(), currentActiveUserSignInTimeOutStamp, currentActiveUserID);


				currentActiveUserOracleTableName.ReleaseBuffer();


				pCSHSensorOracleDataBase->CreateOracleInstanceConnection();


				pCSHSensorOracleDataBase->CreateOracleInstanceStatement(timeOutSQLTransactionStatementBuillder.GetBuffer());


				timeOutSQLTransactionStatementBuillder.ReleaseBuffer();


				pCSHSensorOracleDataBase->excuteOracleInstanceStatement();
			}


			break;
		}

		default:
		{
			break;
		}
	}

	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CDialogEx::OnTimer(nIDEvent);
}


BOOL CCSHSensorDlg::OnQueryEndSession()
{
	if (!CDialogEx::OnQueryEndSession())
	{
		return FALSE;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (currentActiveUserSignInStatus)
	{
		if (MessageBox(_T("Proceed With Sign Out?"), _T("CSHSensor"), (MB_OKCANCEL | MB_ICONINFORMATION | MB_TOPMOST)) == IDOK)
		{
			OnCSHSensorUserSignOut();
		}


		return FALSE;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	return TRUE;
}
