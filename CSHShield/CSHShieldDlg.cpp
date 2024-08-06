

#include "pch.h"
#include "framework.h"
#include "CSHShield.h"
#include "CSHShieldDlg.h"
#include "vector"
#include "sddl.h"
#include "tlhelp32.h"
#include "time.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma warning(disable:26495) 


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct _TRAYSESSIONMANAGER
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	HWND currentActiveUserTrayWindowHandle;


	UINT currentActiveUserTrayUID;


	UINT currentActiveUserTrayCallbackMessage;


	#ifdef _WIN64


		BYTE currentActiveUserTrayReserved[6];


	#elif _WIN32


		BYTE currentActiveUserTrayReserved[2];


	#endif


	HICON currentActiveUserTrayIconHandle;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} TRAYSESSIONMANAGER;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


UINT SensorWatchThread(LPVOID lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CCSHShieldDlg *pCCSHShieldDlg = (CCSHShieldDlg *)lpVoid;


	while (TRUE)
	{
		Sleep(1000);


		if (!(pCCSHShieldDlg->InvestigateTargetProcess(_T("CSHSensor.exe"), &pCCSHShieldDlg->currentActiveUserSensorProcessHandle)))
		{
			pCCSHShieldDlg->EndDialog(NULL);
		}
	}


	return UINT();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


CCSHShieldDlg::CCSHShieldDlg(CWnd *pParent /*=nullptr*/) : CDialogEx(IDD_CSHSHIELD_DIALOG, pParent), currentActiveUserProcessSessionID(NULL), currentActiveUserShieldMutex(NULL), currentActiveUserSensorProcessHandle(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_CSHSHIELD_ICON);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (!(ProcessIdToSessionId(GetCurrentProcessId(), &currentActiveUserProcessSessionID)))
	{
		CString errorMessage;


		errorMessage.Format(_T("ProcessIdToSessionId Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		exit(NULL);


		return;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CString currentActiveUserShieldMutexName(AfxGetAppName());


	currentActiveUserShieldMutexName.AppendFormat(_T("|%ld"), currentActiveUserProcessSessionID);


	currentActiveUserShieldMutex = CreateMutex(NULL, FALSE, currentActiveUserShieldMutexName.GetBuffer());


	currentActiveUserShieldMutexName.ReleaseBuffer();


	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		exit(NULL);


		return;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	shieldBackgroundImagePath.Empty();


	currentActiveUserRegistryPath.Empty();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


CCSHShieldDlg::~CCSHShieldDlg()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DeleteTrayNotification();


	CloseHandle(currentActiveUserShieldMutex);


	currentActiveUserShieldMutex = NULL;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BEGIN_MESSAGE_MAP(CCSHShieldDlg, CDialogEx)


	ON_WM_PAINT()


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_WM_CTLCOLOR()


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	ON_BN_CLICKED(IDC_BUTTON1, &CCSHShieldDlg::OnSignIn)


	ON_BN_CLICKED(IDC_BUTTON2, &CCSHShieldDlg::OnSignUp)


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


END_MESSAGE_MAP()


VOID CCSHShieldDlg::OnPaint()
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


		CImage shieldBackgroundImage;


		if (shieldBackgroundImage.Load(shieldBackgroundImagePath.GetBuffer()) != E_FAIL)
		{
			dc.SetStretchBltMode(STRETCH_HALFTONE);


			shieldBackgroundImage.StretchBlt(dc.m_hDC, NULL, NULL, rect.Width(), rect.Height());
		}


		shieldBackgroundImagePath.ReleaseBuffer();


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
}


HBRUSH CCSHShieldDlg::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
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


BOOL CCSHShieldDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	SetIcon(m_hIcon, TRUE);

	SetIcon(m_hIcon, FALSE);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DeleteTrayNotification();


	PathConfiguration();


	AuthenticateStatusPathConfiguration();


	SignInDisplay();


	AfxBeginThread(SensorWatchThread, this);


	return TRUE;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHShieldDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DDX_Control(pDX, IDC_STATIC1, signInIDStatic);


	DDX_Control(pDX, IDC_EDIT1, signInIDEdit);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DDX_Control(pDX, IDC_STATIC2, signInPasswordStatic);


	DDX_Control(pDX, IDC_EDIT2, signInPasswordEdit);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DDX_Control(pDX, IDC_BUTTON1, signInButton);


	DDX_Control(pDX, IDC_BUTTON2, signUpButton);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BOOL CCSHShieldDlg::PreTranslateMessage(MSG *pMsg)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
	{
		return TRUE;
	}


	if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN))
	{
		OnSignIn();


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


VOID CCSHShieldDlg::DeleteTrayNotification()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	HWND currentActiveUserShellTrayWindow = ::FindWindow(_T("Shell_TrayWnd"), NULL);


	if (currentActiveUserShellTrayWindow == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("FindWindow Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	HWND currentActiveUserTrayNotifyWindow = ::FindWindowEx(currentActiveUserShellTrayWindow, NULL, _T("TrayNotifyWnd"), NULL);


	if (currentActiveUserTrayNotifyWindow == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("FindWindowEx Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	HWND currentActiveUserSysPagerWindow = ::FindWindowEx(currentActiveUserTrayNotifyWindow, NULL, _T("SysPager"), NULL);


	if (currentActiveUserSysPagerWindow == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("FindWindowEx Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	HWND currentActiveUserToolbarWindow32Window = ::FindWindowEx(currentActiveUserSysPagerWindow, NULL, _T("ToolbarWindow32"), NULL);


	if (currentActiveUserToolbarWindow32Window == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("FindWindowEx Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	DWORD dwCurrentActiveUserToolbarWindow32 = NULL;


	if (GetWindowThreadProcessId(currentActiveUserToolbarWindow32Window, &dwCurrentActiveUserToolbarWindow32) == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("GetWindowThreadProcessId Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	HANDLE currentActiveUserToolbarWindow32ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwCurrentActiveUserToolbarWindow32);


	if (currentActiveUserToolbarWindow32ProcessHandle == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("OpenProcess Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	LPVOID pVirtualMemory = VirtualAllocEx(currentActiveUserToolbarWindow32ProcessHandle, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);


	if (pVirtualMemory == NULL)
	{
		CString errorMessage;


		errorMessage.Format(_T("VirtualAllocEx Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		CloseHandle(currentActiveUserToolbarWindow32ProcessHandle);


		currentActiveUserToolbarWindow32ProcessHandle = NULL;


		EndDialog(NULL);


		return;
	}


	LRESULT currentActiveUserToolbarWindow32ButtonCount = ::SendMessage(currentActiveUserToolbarWindow32Window, TB_BUTTONCOUNT, NULL, NULL);


	for (INT index = NULL; index < currentActiveUserToolbarWindow32ButtonCount; index++)
	{
		TRAYSESSIONMANAGER currentActiveUserTraySessionManger;

		memset(&currentActiveUserTraySessionManger, NULL, sizeof(currentActiveUserTraySessionManger));


		TBBUTTON currentActiveUserToolbarWindow32WindowButton;

		memset(&currentActiveUserToolbarWindow32WindowButton, NULL, sizeof(currentActiveUserToolbarWindow32WindowButton));


		if (::SendMessage(currentActiveUserToolbarWindow32Window, TB_GETBUTTON, index, (LPARAM)pVirtualMemory) == NULL)
		{
			continue;
		}


		if (!(ReadProcessMemory(currentActiveUserToolbarWindow32ProcessHandle, pVirtualMemory, (LPVOID)&currentActiveUserToolbarWindow32WindowButton, sizeof(currentActiveUserToolbarWindow32WindowButton), NULL)))
		{
			continue;
		}


		if (!(ReadProcessMemory(currentActiveUserToolbarWindow32ProcessHandle, (LPVOID)currentActiveUserToolbarWindow32WindowButton.dwData, (LPVOID)&currentActiveUserTraySessionManger, sizeof(currentActiveUserTraySessionManger), NULL)))
		{
			continue;
		}


		DWORD dwCurrentActiveUserTrayWindowHandle = NULL;


		GetWindowThreadProcessId(currentActiveUserTraySessionManger.currentActiveUserTrayWindowHandle, &dwCurrentActiveUserTrayWindowHandle);


		if (dwCurrentActiveUserTrayWindowHandle == NULL)
		{
			NOTIFYICONDATA currentActiveUserNotifyIconData;

			memset(&currentActiveUserNotifyIconData, NULL, sizeof(currentActiveUserNotifyIconData));


			currentActiveUserNotifyIconData.cbSize = sizeof(currentActiveUserNotifyIconData);


			currentActiveUserNotifyIconData.hWnd = currentActiveUserTraySessionManger.currentActiveUserTrayWindowHandle;


			currentActiveUserNotifyIconData.uID = currentActiveUserTraySessionManger.currentActiveUserTrayUID;


			currentActiveUserNotifyIconData.uCallbackMessage = currentActiveUserTraySessionManger.currentActiveUserTrayCallbackMessage;


			currentActiveUserNotifyIconData.hIcon = currentActiveUserTraySessionManger.currentActiveUserTrayIconHandle;


			Shell_NotifyIcon(NIM_DELETE, &currentActiveUserNotifyIconData);
		}
	}


	VirtualFreeEx(currentActiveUserToolbarWindow32ProcessHandle, pVirtualMemory, NULL, MEM_RELEASE);


	pVirtualMemory = NULL;


	CloseHandle(currentActiveUserToolbarWindow32ProcessHandle);


	currentActiveUserToolbarWindow32ProcessHandle = NULL;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHShieldDlg::PathConfiguration()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	TCHAR gateKeeperModulePath[MAX_PATH];

	memset(gateKeeperModulePath, NULL, sizeof(gateKeeperModulePath));


	DWORD dwGateKeeperModulePath = GetModuleFileName(NULL, gateKeeperModulePath, (DWORD)(MAX_PATH - 1));


	if ((dwGateKeeperModulePath == NULL) || (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		CString errorMessage;


		errorMessage.Format(_T("GetModuleFileName Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	if (!(PathRemoveFileSpec(gateKeeperModulePath)))
	{
		CString errorMessage;


		errorMessage.Format(_T("PathRemoveFileSpec Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		EndDialog(NULL);


		return;
	}


	shieldBackgroundImagePath.Format(_T("%s\\CSHShield.png"), gateKeeperModulePath);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHShieldDlg::AuthenticateStatusPathConfiguration()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	HANDLE currentActiveUserProcessToken = NULL;


	if (!(OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &currentActiveUserProcessToken)))
	{
		CString errorMessage;


		errorMessage.Format(_T("OpenProcessToken Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


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


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		CloseHandle(currentActiveUserProcessToken);


		currentActiveUserProcessToken = NULL;


		EndDialog(NULL);


		return;
	}


	if (!(GetTokenInformation(currentActiveUserProcessToken, TOKEN_INFORMATION_CLASS::TokenUser, (LPVOID)pQueryTokenUser, dwQueryTokenUser, &dwQueryTokenUser)))
	{
		CString errorMessage;


		errorMessage.Format(_T("GetTokenInformation Failed, GetLastError = %ld"), GetLastError());


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


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


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


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


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BOOL CCSHShieldDlg::InvestigateTargetProcess(LPCTSTR targetProcessName, HWND *pCurrentActiveUserProcessHandle)
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


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


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


					MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


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


VOID CCSHShieldDlg::SignInDisplay()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CRect currentSignInDisplayRect;


	GetClientRect(currentSignInDisplayRect);


	DOUBLE currentSignInDisplayWidth = (DOUBLE)currentSignInDisplayRect.Width();


	DOUBLE currentSignInDisplayHeight = (DOUBLE)currentSignInDisplayRect.Height();


	DOUBLE currentSignInDisplayLeft = NULL;


	DOUBLE currentSignInDisplayTop = NULL;

		
	currentSignInDisplayLeft = (currentSignInDisplayWidth * 0.54);


	currentSignInDisplayTop = (currentSignInDisplayHeight * 0.73);


	signInIDStatic.MoveWindow((INT)currentSignInDisplayLeft, (INT)currentSignInDisplayTop, (INT)(currentSignInDisplayWidth * 0.2), (INT)(currentSignInDisplayHeight * 0.1));


	signInIDStatic.SetWindowText(_T("ID :"));


	currentSignInDisplayLeft = (currentSignInDisplayWidth * 0.585);


	currentSignInDisplayTop = (currentSignInDisplayHeight * 0.72);


	signInIDEdit.MoveWindow((INT)currentSignInDisplayLeft, (INT)currentSignInDisplayTop, (INT)(currentSignInDisplayWidth * 0.375), (INT)(currentSignInDisplayHeight * 0.065));


	signInIDEdit.SetWindowText(NULL);


	signInIDEdit.SetFocus();


	currentSignInDisplayLeft = (currentSignInDisplayWidth * 0.528);


	currentSignInDisplayTop = (currentSignInDisplayHeight * 0.8);


	signInPasswordStatic.MoveWindow((INT)currentSignInDisplayLeft, (INT)currentSignInDisplayTop, (INT)(currentSignInDisplayWidth * 0.2), (INT)(currentSignInDisplayHeight * 0.1));


	signInPasswordStatic.SetWindowText(_T("PW :"));


	currentSignInDisplayLeft = (currentSignInDisplayWidth * 0.585);


	currentSignInDisplayTop = (currentSignInDisplayHeight * 0.79);


	signInPasswordEdit.MoveWindow((INT)currentSignInDisplayLeft, (INT)currentSignInDisplayTop, (INT)(currentSignInDisplayWidth * 0.375), (INT)(currentSignInDisplayHeight * 0.065));


	signInPasswordEdit.SetWindowText(NULL);


	signInPasswordEdit.SetPasswordChar('*');


	currentSignInDisplayLeft = (currentSignInDisplayWidth * 0.65);


	currentSignInDisplayTop = (currentSignInDisplayHeight * 0.865);


	signInButton.MoveWindow((INT)currentSignInDisplayLeft, (INT)currentSignInDisplayTop, (INT)(currentSignInDisplayWidth * 0.15), (INT)(currentSignInDisplayHeight * 0.075));


	signInButton.SetWindowText(_T("Sign In"));


	signInButton.ShowWindow(SW_SHOW);


	currentSignInDisplayLeft = (currentSignInDisplayWidth * 0.81);


	currentSignInDisplayTop = (currentSignInDisplayHeight * 0.865);


	signUpButton.MoveWindow((INT)currentSignInDisplayLeft, (INT)currentSignInDisplayTop, (INT)(currentSignInDisplayWidth * 0.15), (INT)(currentSignInDisplayHeight * 0.075));


	signUpButton.SetWindowText(_T("Sign Up"));


	signUpButton.ShowWindow(SW_SHOW);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHShieldDlg::OnSignIn()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CString currentActiveUserInputtedSignInID;


	signInIDEdit.GetWindowText(currentActiveUserInputtedSignInID);


	if (currentActiveUserInputtedSignInID.IsEmpty())
	{
		signInIDEdit.SetFocus();


		return;
	}


	CString currentActiveUserInputtedSignInPassword;


	signInPasswordEdit.GetWindowText(currentActiveUserInputtedSignInPassword);


	if (currentActiveUserInputtedSignInPassword.IsEmpty())
	{
		signInPasswordEdit.SetFocus();


		return;
	}


	if (currentActiveUserInputtedSignInPassword.GetLength() < 8)
	{
		MessageBox(_T("Please Check The Password Policy For Sign In"), _T("CSHSensor"), (MB_OK | MB_ICONWARNING | MB_TOPMOST));


		signInPasswordEdit.SetWindowText(NULL);


		signInPasswordEdit.SetFocus();


		return;
	}


	CTime currentActiveUserSignInTime = CTime::GetCurrentTime();


	CTime currentActiveUserSignInTimeOut = (currentActiveUserSignInTime + CTimeSpan(NULL, NULL, 30, NULL));


	CString currentActiveUserSignInTimeStamp = currentActiveUserSignInTime.Format(_T("%Y-%m-%d-%H-%M-%S"));


	CString currentActiveUserSignInTimeOutStamp = currentActiveUserSignInTimeOut.Format(_T("%Y-%m-%d-%H-%M-%S"));


	HKEY currentActiveUserRegistryKey;


	LSTATUS currentActiveUserRegistryStatus = RegCreateKey(HKEY_USERS, currentActiveUserRegistryPath.GetBuffer(), &currentActiveUserRegistryKey);


	currentActiveUserRegistryPath.ReleaseBuffer();


	if (currentActiveUserRegistryStatus != ERROR_SUCCESS)
	{
		CString errorMessage;


		errorMessage.Format(_T("RegCreateKey Failed, currentActiveUserRegistryStatus Code = %ld"), currentActiveUserRegistryStatus);


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		return;
	}


	RegCloseKey(currentActiveUserRegistryKey);


	currentActiveUserRegistryStatus = currentActiveUserRegistryController.Open(HKEY_USERS, currentActiveUserRegistryPath, KEY_ALL_ACCESS);


	if (currentActiveUserRegistryStatus != ERROR_SUCCESS)
	{
		CString errorMessage;


		errorMessage.Format(_T("Open Failed, currentActiveUserRegistryStatus Code = %ld"), currentActiveUserRegistryStatus);


		MessageBox(errorMessage, _T("CSHShield"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		return;
	}


	currentActiveUserRegistryController.SetStringValue(_T("userID"), currentActiveUserInputtedSignInID.GetBuffer());


	currentActiveUserInputtedSignInID.ReleaseBuffer();


	currentActiveUserRegistryController.SetStringValue(_T("userPassword"), currentActiveUserInputtedSignInPassword.GetBuffer());


	currentActiveUserInputtedSignInPassword.ReleaseBuffer();


	currentActiveUserRegistryController.SetStringValue(_T("userTimeStamp"), currentActiveUserSignInTimeStamp.GetBuffer());


	currentActiveUserSignInTimeStamp.ReleaseBuffer();


	currentActiveUserRegistryController.SetStringValue(_T("userTimeOutStamp"), currentActiveUserSignInTimeOutStamp.GetBuffer());


	currentActiveUserSignInTimeOutStamp.ReleaseBuffer();


	currentActiveUserRegistryController.Close();


	::SendMessageTimeout(currentActiveUserSensorProcessHandle, WM_CSHSENSOR_USERSIGNIN, NULL, NULL, (SMTO_NORMAL | SMTO_BLOCK), (UINT)1000, NULL);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHShieldDlg::OnSignUp()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	::SendMessageTimeout(currentActiveUserSensorProcessHandle, WM_CSHSENSOR_USERSIGNUP, NULL, NULL, (SMTO_NORMAL | SMTO_BLOCK), (UINT)1000, NULL);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
