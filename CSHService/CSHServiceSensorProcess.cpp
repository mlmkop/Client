

#include "ws2tcpip.h"
#include "windows.h"
#include "tchar.h"
#include "CSHServiceSensorProcess.h"
#include "CSHServiceLogger.h"
#include "vector"
#include "tlhelp32.h"
#include "wtsApi32.h"
#include "userenv.h"
#include "sddl.h"
#include "iphlpapi.h"
#include "atlstr.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _UNICODE


	#include "stdio.h"


#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct _SESSIONPROCESSMANAGER
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD currentActiveProcessID;


	DWORD currentActiveSessionId;


	TCHAR currentModulePath[MAX_PATH];


	LPTSTR currentQueryBuffer;


	DWORD currentDwQueryBuffer;


	HANDLE currentActiveThreadHandle;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} SESSIONPROCESSMANAGER;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern std::vector<SESSIONPROCESSMANAGER *> sessionProcessManager;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct _SESSIONMANAGER
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	STARTUPINFO currentUserStartUpInfo;


	PROCESS_INFORMATION currentUserProcessInformation;


	DWORD currentUserSessionID;


	TCHAR currentUserName[128];


	TCHAR currentUserSID[128];


	TCHAR currentUserIP[128];


	TCHAR currentUserMAC[128];


	TCHAR currentUserSessionToken[128];


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} SESSIONMANAGER;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern std::vector<SESSIONMANAGER *> sessionManager;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern volatile BOOL closeSensorProcessFlag;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


DWORD EachUserSessionWatchThread(VOID *lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	SESSIONMANAGER *pCurrentSessionManager = (SESSIONMANAGER *)lpVoid;


	PROCESSENTRY32 processEntry32;

	memset(&processEntry32, NULL, sizeof(processEntry32));


	processEntry32.dwSize = sizeof(processEntry32);


	HANDLE toolhelp32SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);


	if (toolhelp32SnapshotHandle == INVALID_HANDLE_VALUE)
	{
		returnCode = GetLastError();


		ServiceLogger(_T("[ %s : %d ] CreateToolhelp32Snapshot Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		return returnCode;
	}


	if (Process32First(toolhelp32SnapshotHandle, &processEntry32))
	{
		do
		{
			if (_tcscmp(processEntry32.szExeFile, _T("CSHSensor.exe")) == NULL)
			{
				if (pCurrentSessionManager->currentUserProcessInformation.dwProcessId == processEntry32.th32ProcessID)
				{
					ServiceLogger(_T("[ %s : %d ] Target Process ID Detected, pCurrentSessionManager->userPi.dwProcessId = %ld, processEntry32.th32ProcessID = %ld"), _T(__FUNCTION__), __LINE__, pCurrentSessionManager->currentUserProcessInformation.dwProcessId, processEntry32.th32ProcessID);


					TCHAR currentActiveUserRegistryPath[128];

					memset(currentActiveUserRegistryPath, NULL, sizeof(currentActiveUserRegistryPath));


					_stprintf_s(currentActiveUserRegistryPath, _T("%s\\SOFTWARE\\CSHDLP"), pCurrentSessionManager->currentUserSID);


					HKEY currentActiveUserRegistryKey;


					LSTATUS currentActiveUserRegistryStatus = RegCreateKey(HKEY_USERS, currentActiveUserRegistryPath, &currentActiveUserRegistryKey);


					if (currentActiveUserRegistryStatus != ERROR_SUCCESS) 
					{
						returnCode = (DWORD)currentActiveUserRegistryStatus;


						ServiceLogger(_T("[ %s : %d ] RegCreateKey Failed, currentActiveUserRegistryStatus Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


						break;
					}


					RegCloseKey(currentActiveUserRegistryKey);


					currentActiveUserRegistryKey = NULL;


					CRegKey registryController;


					currentActiveUserRegistryStatus = registryController.Open(HKEY_USERS, currentActiveUserRegistryPath, KEY_ALL_ACCESS);


					if (currentActiveUserRegistryStatus != ERROR_SUCCESS)
					{
						returnCode = (DWORD)currentActiveUserRegistryStatus;


						ServiceLogger(_T("[ %s : %d ] Open Failed, currentActiveUserRegistryStatus Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


						break;
					}


					returnCode = (DWORD)currentActiveUserRegistryStatus;
					

					ServiceLogger(_T("[ %s : %d ] Initializing To %s User's %s Residual Token Data"), _T(__FUNCTION__), __LINE__, pCurrentSessionManager->currentUserName, currentActiveUserRegistryPath);


					_stprintf_s(pCurrentSessionManager->currentUserSessionToken, _T("%s|%s|%s"), pCurrentSessionManager->currentUserIP, pCurrentSessionManager->currentUserName, pCurrentSessionManager->currentUserMAC);


					registryController.SetStringValue(_T("userSessionToken"), pCurrentSessionManager->currentUserSessionToken);


					registryController.SetStringValue(_T("userMACAddress"), pCurrentSessionManager->currentUserMAC);


					registryController.SetStringValue(_T("userIPAddress"), pCurrentSessionManager->currentUserIP);


					registryController.Close();


					ServiceLogger(_T("[ %s : %d ] CreateSensorProcess Done, Waiting For %s User's %ld Process ID INFINITE..."), _T(__FUNCTION__), __LINE__, pCurrentSessionManager->currentUserName, pCurrentSessionManager->currentUserProcessInformation.dwProcessId);


					WaitForSingleObject(pCurrentSessionManager->currentUserProcessInformation.hProcess, INFINITE);


					CloseHandle(pCurrentSessionManager->currentUserProcessInformation.hProcess);


					CloseHandle(pCurrentSessionManager->currentUserProcessInformation.hThread);


					memset(pCurrentSessionManager, NULL, sizeof(SESSIONMANAGER));


					break;
				}
			}
		} while (Process32Next(toolhelp32SnapshotHandle, &processEntry32));
	}


	CloseHandle(toolhelp32SnapshotHandle);


	toolhelp32SnapshotHandle = NULL;


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD GetCurrentActiveUserAdapterInformation(SESSIONMANAGER *pCurrentSession)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	IP_ADAPTER_ADDRESSES ipAdapterAddresses[32];


	ULONG ulIpAdapterAddresses = sizeof(ipAdapterAddresses);


	ULONG currentActiveAdaptersAddressesStatus = GetAdaptersAddresses(PF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, ipAdapterAddresses, &ulIpAdapterAddresses);


	if (currentActiveAdaptersAddressesStatus != ERROR_SUCCESS)
	{
		returnCode = (DWORD)currentActiveAdaptersAddressesStatus;


		ServiceLogger(_T("[ %s : %d ] GetAdaptersAddresses Failed, currentActiveAdaptersAddressesStatus Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		return returnCode;
	}


	returnCode = (DWORD)currentActiveAdaptersAddressesStatus;


	PIP_ADAPTER_ADDRESSES pIpAdapterAddresses = ipAdapterAddresses;


	while (pIpAdapterAddresses != NULL)
	{
		#ifndef _UNICODE


			CHAR targetDescription[128];

			memset(targetDescription, NULL, sizeof(targetDescription));


			_stprintf_s(targetDescription, _T("Virtual"));


			INT convertedTargetDescriptionLength = MultiByteToWideChar(CP_ACP, NULL, targetDescription, (INT)_tcslen(targetDescription), NULL, NULL);


			WCHAR convertedVirtualDescription[128];

			memset(convertedVirtualDescription, NULL, sizeof(convertedVirtualDescription));


			MultiByteToWideChar(CP_ACP, NULL, targetDescription, (INT)_tcslen(targetDescription), convertedVirtualDescription, convertedTargetDescriptionLength);


			_stprintf_s(targetDescription, _T("Pseudo"));


			convertedTargetDescriptionLength = MultiByteToWideChar(CP_ACP, NULL, targetDescription, (INT)_tcslen(targetDescription), NULL, NULL);


			WCHAR convertedPseudoDescription[128];

			memset(convertedPseudoDescription, NULL, sizeof(convertedPseudoDescription));


			MultiByteToWideChar(CP_ACP, NULL, targetDescription, (INT)_tcslen(targetDescription), convertedPseudoDescription, convertedTargetDescriptionLength);


			if ((wcsstr(pIpAdapterAddresses->Description, convertedVirtualDescription) == NULL) && (wcsstr(pIpAdapterAddresses->Description, convertedPseudoDescription) == NULL))
			{


		#else


			if ((_tcsstr(pIpAdapterAddresses->Description, _T("Virtual")) == NULL) && (_tcsstr(pIpAdapterAddresses->Description, _T("Pseudo")) == NULL))
			{


		#endif
		

				if (pIpAdapterAddresses->IfType == IF_TYPE_ETHERNET_CSMACD)
				{
					CHAR currentActiveAdapterUnicastIpAddress[128];

					memset(currentActiveAdapterUnicastIpAddress, NULL, sizeof(currentActiveAdapterUnicastIpAddress));


					if (getnameinfo(pIpAdapterAddresses->FirstUnicastAddress->Address.lpSockaddr, pIpAdapterAddresses->FirstUnicastAddress->Address.iSockaddrLength, currentActiveAdapterUnicastIpAddress, sizeof(currentActiveAdapterUnicastIpAddress), NULL, NULL, NI_NUMERICHOST) != NULL)
					{
						returnCode = (DWORD)WSAGetLastError();


						ServiceLogger(_T("[ %s : %d ] getnameinfo Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


						break;
					}


					CString convertedCurrentActiveAdapterUnicastIpAddress(currentActiveAdapterUnicastIpAddress);


					_stprintf_s(pCurrentSession->currentUserIP, convertedCurrentActiveAdapterUnicastIpAddress.GetBuffer());


					convertedCurrentActiveAdapterUnicastIpAddress.ReleaseBuffer();


					CString convertedCurrentActiveAdapterUnicastMACAddress;


					convertedCurrentActiveAdapterUnicastMACAddress.Format(_T("%02X:%02X:%02X:%02X:%02X:%02X"), pIpAdapterAddresses->PhysicalAddress[0], pIpAdapterAddresses->PhysicalAddress[1], pIpAdapterAddresses->PhysicalAddress[2], pIpAdapterAddresses->PhysicalAddress[3], pIpAdapterAddresses->PhysicalAddress[4], pIpAdapterAddresses->PhysicalAddress[5]);


					_stprintf_s(pCurrentSession->currentUserMAC, convertedCurrentActiveAdapterUnicastMACAddress.GetBuffer());


					convertedCurrentActiveAdapterUnicastMACAddress.ReleaseBuffer();


					break;
				}
			}


		pIpAdapterAddresses = pIpAdapterAddresses->Next;
	}


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


HANDLE RegistEachUserSession(SESSIONMANAGER *pCurrentSessionManager, DWORD currentActiveSessionId, LPCTSTR queryBuffer)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	HANDLE returnThreadHandle = NULL;


	pCurrentSessionManager->currentUserSessionID = currentActiveSessionId;


	_stprintf_s(pCurrentSessionManager->currentUserName, queryBuffer);


	HANDLE currentActiveUserProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pCurrentSessionManager->currentUserProcessInformation.dwProcessId);


	if (currentActiveUserProcessHandle == INVALID_HANDLE_VALUE)
	{
		ServiceLogger(_T("[ %s : %d ] OpenProcess Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


		return returnThreadHandle;
	}


	HANDLE currentActiveUserProcessToken = NULL;


	if (!(OpenProcessToken(currentActiveUserProcessHandle, TOKEN_ALL_ACCESS, &currentActiveUserProcessToken)))
	{
		ServiceLogger(_T("[ %s : %d ] OpenProcessToken Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


		CloseHandle(currentActiveUserProcessHandle);


		currentActiveUserProcessHandle = NULL;


		return returnThreadHandle;
	}


	CloseHandle(currentActiveUserProcessHandle);


	currentActiveUserProcessHandle = NULL;


	DWORD dwQueryTokenUser = NULL;


	GetTokenInformation(currentActiveUserProcessToken, TOKEN_INFORMATION_CLASS::TokenUser, NULL, NULL, &dwQueryTokenUser);


	PTOKEN_USER pQueryTokenUser = (PTOKEN_USER)LocalAlloc(LPTR, (size_t)dwQueryTokenUser);


	if (pQueryTokenUser == NULL)
	{
		ServiceLogger(_T("[ %s : %d ] LocalAlloc Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


		CloseHandle(currentActiveUserProcessToken);


		currentActiveUserProcessToken = NULL;


		return returnThreadHandle;
	}


	if (!(GetTokenInformation(currentActiveUserProcessToken, TOKEN_INFORMATION_CLASS::TokenUser, (LPVOID)pQueryTokenUser, dwQueryTokenUser, &dwQueryTokenUser))) 
	{
		ServiceLogger(_T("[ %s : %d ] GetTokenInformation Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


		LocalFree(pQueryTokenUser);


		pQueryTokenUser = NULL;


		CloseHandle(currentActiveUserProcessToken);


		currentActiveUserProcessToken = NULL;


		return returnThreadHandle;
	}


	CloseHandle(currentActiveUserProcessToken);


	currentActiveUserProcessToken = NULL;


	LPTSTR pSID = NULL;


	if (!(ConvertSidToStringSid(pQueryTokenUser->User.Sid, &pSID))) 
	{
		ServiceLogger(_T("[ %s : %d ] ConvertSidToStringSid Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


		LocalFree(pQueryTokenUser);


		pQueryTokenUser = NULL;


		return returnThreadHandle;
	}


	_stprintf_s(pCurrentSessionManager->currentUserSID, pSID);


	LocalFree(pSID);


	pSID = NULL;


	LocalFree(pQueryTokenUser);


	pQueryTokenUser = NULL;


	if (GetCurrentActiveUserAdapterInformation(pCurrentSessionManager) != ERROR_SUCCESS) 
	{
		return returnThreadHandle;
	}


	returnThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)EachUserSessionWatchThread, pCurrentSessionManager, NULL, NULL);


	if (returnThreadHandle == NULL)
	{
		ServiceLogger(_T("[ %s : %d ] CreateThread Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());
	}


	return returnThreadHandle;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD EstablishProcessEachUserSessionThread(VOID *lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	SESSIONPROCESSMANAGER *pCurrentSessionProcessManager = (SESSIONPROCESSMANAGER *)lpVoid;


	SESSIONMANAGER *pCurrentSessionManager = (SESSIONMANAGER *)calloc((size_t)1, sizeof(SESSIONMANAGER));


	if (pCurrentSessionManager == NULL)
	{
		returnCode = GetLastError();


		ServiceLogger(_T("[ %s : %d ] calloc Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		return returnCode;
	}


	sessionManager.push_back(pCurrentSessionManager);


	ServiceLogger(_T("[ %s : %d ] Save Session Done, Current sessionManager's Size = %ld"), _T(__FUNCTION__), __LINE__, sessionManager.size());


	while (!(closeSensorProcessFlag))
	{
		Sleep(1000);


		HANDLE currentActiveSystemProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pCurrentSessionProcessManager->currentActiveProcessID);

						
		if (currentActiveSystemProcessHandle == NULL)
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] OpenProcess Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			break;
		}


		ServiceLogger(_T("[ %s : %d ] pCurrentSessionProcessManager->currentActiveProcessID = %ld"), _T(__FUNCTION__), __LINE__, pCurrentSessionProcessManager->currentActiveProcessID);


		HANDLE currentActiveSystemProcessToken = NULL;

								
		if (!(OpenProcessToken(currentActiveSystemProcessHandle, TOKEN_ALL_ACCESS, &currentActiveSystemProcessToken)))
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] OpenProcessToken Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			CloseHandle(currentActiveSystemProcessHandle);


			currentActiveSystemProcessHandle = NULL;

																																	
			break;
		}


		LUID luid;

		memset(&luid, NULL, sizeof(luid));


		if (!(LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)))
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] LookupPrivilegeValue Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			CloseHandle(currentActiveSystemProcessToken);


			currentActiveSystemProcessToken = NULL;


			CloseHandle(currentActiveSystemProcessHandle);


			currentActiveSystemProcessHandle = NULL;

												
			break;
		}


		TOKEN_PRIVILEGES tokenPriVileges;

		memset(&tokenPriVileges, NULL, sizeof(tokenPriVileges));


		tokenPriVileges.PrivilegeCount = (DWORD)1;


		tokenPriVileges.Privileges[0].Luid = luid;


		tokenPriVileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


		if (!(AdjustTokenPrivileges(currentActiveSystemProcessToken, FALSE, &tokenPriVileges, sizeof(tokenPriVileges), NULL, NULL)))
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] AdjustTokenPrivileges Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			CloseHandle(currentActiveSystemProcessToken);


			currentActiveSystemProcessToken = NULL;


			CloseHandle(currentActiveSystemProcessHandle);


			currentActiveSystemProcessHandle = NULL;

												
			break;
		}


		HANDLE duplicatedToken = NULL;

										
		if (!(DuplicateTokenEx(currentActiveSystemProcessToken, TOKEN_ALL_ACCESS, NULL, SECURITY_IMPERSONATION_LEVEL::SecurityDelegation, TOKEN_TYPE::TokenImpersonation, &duplicatedToken)))
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] DuplicateTokenEx Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			CloseHandle(currentActiveSystemProcessToken);


			currentActiveSystemProcessToken = NULL;


			CloseHandle(currentActiveSystemProcessHandle);


			currentActiveSystemProcessHandle = NULL;

								
			break;
		}


		LPVOID pEnvironmentBlock = NULL;

								
		if (!(CreateEnvironmentBlock(&pEnvironmentBlock, duplicatedToken, FALSE)))
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] CreateEnvironmentBlock Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			CloseHandle(duplicatedToken);


			duplicatedToken = NULL;


			CloseHandle(currentActiveSystemProcessToken);


			currentActiveSystemProcessToken = NULL;


			CloseHandle(currentActiveSystemProcessHandle);


			currentActiveSystemProcessHandle = NULL;

																
			break;
		}


		TCHAR targetDesktop[128];

		memset(targetDesktop, NULL, sizeof(targetDesktop));


		_stprintf_s(targetDesktop, _T("winsta0\\default"));


		pCurrentSessionManager->currentUserStartUpInfo.cb = sizeof(SESSIONMANAGER);


		pCurrentSessionManager->currentUserStartUpInfo.lpDesktop = (LPTSTR)targetDesktop;


		BOOL bCreateProcessAsUser = CreateProcessAsUser(duplicatedToken, pCurrentSessionProcessManager->currentModulePath, NULL, NULL, NULL, FALSE, (CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT), pEnvironmentBlock, NULL, &pCurrentSessionManager->currentUserStartUpInfo, &pCurrentSessionManager->currentUserProcessInformation);


		if (!(bCreateProcessAsUser))
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] CreateProcessAsUser Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			DestroyEnvironmentBlock(pEnvironmentBlock);


			pEnvironmentBlock = NULL;


			CloseHandle(duplicatedToken);


			duplicatedToken = NULL;


			CloseHandle(currentActiveSystemProcessToken);


			currentActiveSystemProcessToken = NULL;


			CloseHandle(currentActiveSystemProcessHandle);


			currentActiveSystemProcessHandle = NULL;

																
			break;
		}


		DestroyEnvironmentBlock(pEnvironmentBlock);


		pEnvironmentBlock = NULL;


		CloseHandle(duplicatedToken);


		duplicatedToken = NULL;


		CloseHandle(currentActiveSystemProcessToken);


		currentActiveSystemProcessToken = NULL;


		CloseHandle(currentActiveSystemProcessHandle);


		currentActiveSystemProcessHandle = NULL;


		HANDLE currentEachUserSessionWatchThreadHandle = RegistEachUserSession(pCurrentSessionManager, pCurrentSessionProcessManager->currentActiveSessionId, pCurrentSessionProcessManager->currentQueryBuffer);


		if (currentEachUserSessionWatchThreadHandle == NULL)
		{
			TerminateProcess(pCurrentSessionManager->currentUserProcessInformation.hProcess, NULL);


			CloseHandle(pCurrentSessionManager->currentUserProcessInformation.hProcess);


			CloseHandle(pCurrentSessionManager->currentUserProcessInformation.hThread);

								
			continue;
		}


		WaitForSingleObject(currentEachUserSessionWatchThreadHandle, INFINITE);


		CloseHandle(currentEachUserSessionWatchThreadHandle);


		currentEachUserSessionWatchThreadHandle = NULL;
	}


	std::vector<SESSIONMANAGER *>::iterator currentSessionCursor = sessionManager.begin();


	for (size_t index = NULL; index < sessionManager.size(); index++, currentSessionCursor++)
	{
		if (sessionManager[index] == pCurrentSessionManager)
		{
			ServiceLogger(_T("[ %s : %d ] Frees The sessionManager[%ld] Memory, sessionManager[%ld] = %zd -> NULL"), _T(__FUNCTION__), __LINE__, index, index, sessionManager[index]);


			memset(sessionManager[index], NULL, sizeof(SESSIONMANAGER));


			free(sessionManager[index]);


			sessionManager[index] = NULL;


			break;
		}
	}


	sessionManager.erase(currentSessionCursor);


	ServiceLogger(_T("[ %s : %d ] Remove session Done, Current sessionManager's Size = %ld"), _T(__FUNCTION__), __LINE__, sessionManager.size());


	std::vector<SESSIONPROCESSMANAGER *>::iterator currentSessionProcessCursor = sessionProcessManager.begin();


	for (size_t index = NULL; index < sessionProcessManager.size(); index++, currentSessionProcessCursor++)
	{
		if (sessionProcessManager[index] == pCurrentSessionProcessManager)
		{
			ServiceLogger(_T("[ %s : %d ] Frees The sessionProcessManager[%ld] Memory, sessionProcessManager[%ld] = %zd -> NULL"), _T(__FUNCTION__), __LINE__, index, index, sessionProcessManager[index]);


			CloseHandle(sessionProcessManager[index]->currentActiveThreadHandle);


			memset(sessionProcessManager[index], NULL, sizeof(SESSIONPROCESSMANAGER));


			free(sessionProcessManager[index]);


			sessionProcessManager[index] = NULL;


			break;
		}
	}


	sessionProcessManager.erase(currentSessionProcessCursor);


	ServiceLogger(_T("[ %s : %d ] Remove sessionProcess Done, Current sessionProcessManager's Size = %ld"), _T(__FUNCTION__), __LINE__, sessionProcessManager.size());


	returnCode = ERROR_SUCCESS;


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CreateSensorProcess(VOID *lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (lpVoid != NULL)
	{
		for (size_t index = NULL; index < sessionProcessManager.size(); index++)
		{
			if (_tcscmp(sessionProcessManager[index]->currentQueryBuffer, (LPTSTR)lpVoid) == NULL)
			{
				ServiceLogger(_T("[ %s : %d ] Already Have Established Manager Process, sessionProcessManager.currentQueryBuffer = %s"), _T(__FUNCTION__), __LINE__, sessionProcessManager[index]->currentQueryBuffer);


				return;
			}
		}


		typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);


		LPFN_ISWOW64PROCESS lpIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("Kernel32")), "IsWow64Process");


		if (lpIsWow64Process == NULL)
		{
			ServiceLogger(_T("[ %s : %d ] GetProcAddress Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			return;
		}


		BOOL isWow64Flag = FALSE;


		lpIsWow64Process(GetCurrentProcess(), &isWow64Flag);


		TCHAR environmentVariablePath[MAX_PATH];

		memset(environmentVariablePath, NULL, sizeof(environmentVariablePath));


		DWORD dwEnvironmentVariablePath = NULL;


		if (isWow64Flag)
		{
			dwEnvironmentVariablePath = GetEnvironmentVariable(_T("Programfiles(x86)"), environmentVariablePath, (DWORD)(MAX_PATH - 1));
		}
		else
		{
			dwEnvironmentVariablePath = GetEnvironmentVariable(_T("Programfiles"), environmentVariablePath, (DWORD)(MAX_PATH - 1));
		}


		if ((dwEnvironmentVariablePath == NULL) || (GetLastError() == ERROR_ENVVAR_NOT_FOUND))
		{
			ServiceLogger(_T("[ %s : %d ] GetEnvironmentVariable Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


			return;
		}


		TCHAR sensorModulePath[MAX_PATH];

		memset(sensorModulePath, NULL, sizeof(sensorModulePath));


		_stprintf_s(sensorModulePath, _T("%s\\CSHDLP\\%s"), environmentVariablePath, _T("CSHSensor.exe"));


		while (!(closeSensorProcessFlag))
		{
			Sleep(1000);


			WTS_SESSION_INFO wtsSessionInfo;

			memset(&wtsSessionInfo, NULL, sizeof(wtsSessionInfo));


			PWTS_SESSION_INFO pWtsSessionInfo = NULL;


			DWORD dwSessionCount = NULL;


			if (!(WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, NULL, (DWORD)1, &pWtsSessionInfo, &dwSessionCount))) 
			{
				ServiceLogger(_T("[ %s : %d ] WTSEnumerateSessions Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


				continue;
			}


			for (DWORD index = NULL; index < dwSessionCount; index++)
			{
				wtsSessionInfo = pWtsSessionInfo[index];


				if (wtsSessionInfo.State == WTS_CONNECTSTATE_CLASS::WTSActive)
				{
					PROCESSENTRY32 processEntry32;

					memset(&processEntry32, NULL, sizeof(processEntry32));


					processEntry32.dwSize = sizeof(processEntry32);


					HANDLE toolhelp32SnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);


					if (toolhelp32SnapshotHandle == INVALID_HANDLE_VALUE)
					{
						ServiceLogger(_T("[ %s : %d ] CreateToolhelp32Snapshot Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


						break;
					}


					if (Process32First(toolhelp32SnapshotHandle, &processEntry32))
					{
						do
						{
							if (_tcscmp(processEntry32.szExeFile, _T("explorer.exe")) == NULL)
							{
								DWORD dwCurrentActiveSessionID = NULL;


								if (!(ProcessIdToSessionId(processEntry32.th32ProcessID, &dwCurrentActiveSessionID))) 
								{
									ServiceLogger(_T("[ %s : %d ] ProcessIdToSessionId Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


									break;
								}


								if (wtsSessionInfo.SessionId == dwCurrentActiveSessionID)
								{
									LPTSTR queryBuffer = NULL;


									DWORD dwQueryBuffer = NULL;


									if (!(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, wtsSessionInfo.SessionId, WTS_INFO_CLASS::WTSUserName, &queryBuffer, &dwQueryBuffer)))
									{
										ServiceLogger(_T("[ %s : %d ] WTSQuerySessionInformation Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


										break;
									}

																		
									ServiceLogger(_T("[ %s : %d ] Establish To Sensor Process, wtsSessionInfo.SessionId = %ld, WTSUserName = %s"), _T(__FUNCTION__), __LINE__, wtsSessionInfo.SessionId, queryBuffer);


									SESSIONPROCESSMANAGER *pCurrentSessionProcessManager = (SESSIONPROCESSMANAGER *)calloc((size_t)1, sizeof(SESSIONPROCESSMANAGER));


									if (pCurrentSessionProcessManager == NULL)
									{
										ServiceLogger(_T("[ %s : %d ] calloc Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


										break;
									}


									pCurrentSessionProcessManager->currentActiveProcessID = processEntry32.th32ProcessID;


									pCurrentSessionProcessManager->currentActiveSessionId = wtsSessionInfo.SessionId;


									_stprintf_s(pCurrentSessionProcessManager->currentModulePath, sensorModulePath);


									pCurrentSessionProcessManager->currentQueryBuffer = queryBuffer;


									pCurrentSessionProcessManager->currentDwQueryBuffer = dwQueryBuffer;


									sessionProcessManager.push_back(pCurrentSessionProcessManager);
																																					

									HANDLE currentEstablishProcessEachUserSessionThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)EstablishProcessEachUserSessionThread, sessionProcessManager.back(), NULL, NULL);


									if (currentEstablishProcessEachUserSessionThreadHandle == NULL)
									{
										ServiceLogger(_T("[ %s : %d ] CreateThread Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


										sessionProcessManager.pop_back();


										memset(pCurrentSessionProcessManager, NULL, sizeof(SESSIONPROCESSMANAGER));


										free(pCurrentSessionProcessManager);


										pCurrentSessionProcessManager = NULL;


										break;
									}


									pCurrentSessionProcessManager->currentActiveThreadHandle = currentEstablishProcessEachUserSessionThreadHandle;


									CloseHandle(toolhelp32SnapshotHandle);


									toolhelp32SnapshotHandle = NULL;


									WTSFreeMemory(pWtsSessionInfo);


									pWtsSessionInfo = NULL;


									ServiceLogger(_T("[ %s : %d ] Save SessionProcess Done, Current sessionProcessManager's Size = %ld"), _T(__FUNCTION__), __LINE__, sessionProcessManager.size());


									return;
								}
							}
						} while (Process32Next(toolhelp32SnapshotHandle, &processEntry32));
					}


					CloseHandle(toolhelp32SnapshotHandle);


					toolhelp32SnapshotHandle = NULL;
				}
			}


			WTSFreeMemory(pWtsSessionInfo);


			pWtsSessionInfo = NULL;
		}
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
