

#include "windows.h"
#include "tchar.h"
#include "CSHServiceManager.h"
#include "CSHServiceLogger.h"
#include "CSHServiceWebSocketServer.h"
#include "CSHServiceSensorProcess.h"
#include "vector"
#include "wtsapi32.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#pragma warning(disable:6258) 


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _UNICODE


    #include "stdio.h"


#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


using namespace std;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCSHServiceManager *CCSHServiceManager::pCCSHServiceManager = NULL;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct _WEBSOCKETSESSIONMANAGER
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    SOCKET currentClientSocket;


    SOCKADDR_IN currentClientAddress;


    INT currentClientAddressLength;


    CHAR currentClientBuffer[1024];


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} WEBSOCKETSESSIONMANAGER;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


vector<WEBSOCKETSESSIONMANAGER *> webSocketSessionManager;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


BOOL closeWebSocketSessionFlag;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


SOCKET webSocketServerSocket;


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


vector<SESSIONPROCESSMANAGER *> sessionProcessManager;


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


vector<SESSIONMANAGER *> sessionManager;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


BOOL closeSensorProcessFlag;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCSHServiceManager::CCSHServiceManager(LPTSTR lpServiceName) : serviceName(NULL), serviceStatus({NULL, }), serviceStatusHandle(NULL), webSocketServerThreadHandle(NULL)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    serviceName = lpServiceName;


    serviceStatus.dwServiceType = (DWORD)(SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS);


    serviceStatus.dwControlsAccepted = (DWORD)(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE);
    

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD CCSHServiceManager::ServiceRun(CCSHServiceManager &CSHServiceManager)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    DWORD returnCode = (DWORD)1;


    pCCSHServiceManager = &CSHServiceManager;


    SERVICE_TABLE_ENTRY serviceTableEntry;

    memset(&serviceTableEntry, NULL, sizeof(serviceTableEntry));


    serviceTableEntry.lpServiceName = serviceName;


    serviceTableEntry.lpServiceProc = ServiceMain;


    if (!(StartServiceCtrlDispatcher(&serviceTableEntry))) 
    {
        returnCode = GetLastError();


        ServiceLogger(_T("[ %s : %d ] StartServiceCtrlDispatcher Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


        return returnCode;
    }
    

    returnCode = ERROR_SUCCESS;
    

    return returnCode;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID WINAPI CCSHServiceManager::ServiceMain(DWORD dwNumServicesArgs, LPTSTR *lpServiceArgVectors)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    pCCSHServiceManager->serviceStatusHandle = RegisterServiceCtrlHandlerEx(pCCSHServiceManager->serviceName, (LPHANDLER_FUNCTION_EX)ServiceCtrlHandler, NULL);


    if (pCCSHServiceManager->serviceStatusHandle == NULL) 
    {
        ServiceLogger(_T("[ %s : %d ] RegisterServiceCtrlHandlerEx Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


        return;
    }


    pCCSHServiceManager->Start(dwNumServicesArgs, lpServiceArgVectors);


    while (pCCSHServiceManager->serviceStatus.dwCurrentState != SERVICE_STOPPED)
    {
        Sleep(1000);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD WINAPI CCSHServiceManager::ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    DWORD returnCode = (DWORD)1;


    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
        {
            pCCSHServiceManager->Stop();


            returnCode = ERROR_SUCCESS;


            break;
        }

        case SERVICE_CONTROL_SESSIONCHANGE:
        {
            if ((dwEventType == WTS_CONSOLE_CONNECT) || (dwEventType == WTS_SESSION_LOGON))
            {
                WTSSESSION_NOTIFICATION wtsSessionNotification;

                memset(&wtsSessionNotification, NULL, sizeof(wtsSessionNotification));


                memcpy_s(&wtsSessionNotification, sizeof(wtsSessionNotification), lpEventData, sizeof(wtsSessionNotification));


                LPTSTR queryBuffer = NULL;


                DWORD dwQueryBuffer = NULL;


                if (!(WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, wtsSessionNotification.dwSessionId, WTSUserName, &queryBuffer, &dwQueryBuffer)))
                {
                    returnCode = GetLastError();


                    ServiceLogger(_T("[ %s : %d ] WTSQuerySessionInformation Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


                    break;
                }


                if (_tcslen(queryBuffer) > NULL)
                {
                    ServiceLogger(_T("[ %s : %d ] Detected To SERVICE_CONTROL_SESSIONCHANGE, dwSessionID = %ld, WTSUserName = %s"), _T(__FUNCTION__), __LINE__, wtsSessionNotification.dwSessionId, queryBuffer);


                    pCCSHServiceManager->SessionChange(queryBuffer);


                    returnCode = ERROR_SUCCESS;
                }
            }


            break;
        }

        default:
        {
            break;
        }
    }


    return returnCode;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHServiceManager::Start(DWORD dwNumServicesArgs, LPTSTR *lpServiceArgVectors)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    try
    {
        SetServiceStatus(SERVICE_START_PENDING);


        ServiceLogger(_T("[ %s : %d ] Service Start"), _T(__FUNCTION__), __LINE__);


        closeWebSocketSessionFlag = FALSE;


        webSocketServerSocket = NULL;


        webSocketServerThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CreateWebSocketServer, NULL, NULL, NULL);


        if (webSocketServerThreadHandle == NULL) 
        {
            ServiceLogger(_T("[ %s : %d ] CreateThread Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


            throw;
        }


        closeSensorProcessFlag = FALSE;


        SetServiceStatus(SERVICE_RUNNING, NULL, (DWORD)3000);
    }
    catch (...)
    {
        DWORD dwCurrentErrorCode = GetLastError();


        SetServiceStatus(SERVICE_STOPPED, dwCurrentErrorCode, (DWORD)3000);


        ReportErrorEvent((LPTSTR)_T("Service Start"), dwCurrentErrorCode);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHServiceManager::Stop()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    try
    {
        SetServiceStatus(SERVICE_STOP_PENDING);


        ServiceLogger(_T("[ %s : %d ] Service Stop"), _T(__FUNCTION__), __LINE__);


        closeWebSocketSessionFlag = TRUE;


        CloseWebSocketSession();


        closeSensorProcessFlag = TRUE;


        CloseSensorProcess();


        SetServiceStatus(SERVICE_STOPPED, NULL, (DWORD)3000);
    }
    catch (...)
    {
        DWORD dwCurrentErrorCode = GetLastError();


        SetServiceStatus(SERVICE_RUNNING, dwCurrentErrorCode, (DWORD)3000);


        ReportErrorEvent((LPTSTR)_T("Service Stop"), dwCurrentErrorCode);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHServiceManager::SessionChange(LPTSTR pQueryBuffer)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    try
    {
        ServiceLogger(_T("[ %s : %d ] Service SessionChange"), _T(__FUNCTION__), __LINE__);


        CreateSensorProcess(pQueryBuffer);
    }
    catch (...)
    {
        DWORD dwCurrentErrorCode = GetLastError();


        ReportErrorEvent((LPTSTR)_T("Service SessionChange"), dwCurrentErrorCode);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHServiceManager::SetServiceStatus(DWORD dwCurrentState, DWORD dwCurrentWin32ExitCode, DWORD dwCurrentWaitHint)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    serviceStatus.dwCurrentState = dwCurrentState;


    serviceStatus.dwWin32ExitCode = dwCurrentWin32ExitCode;


    serviceStatus.dwWaitHint = dwCurrentWaitHint;


    serviceStatus.dwCheckPoint = ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) ? NULL : serviceStatus.dwCheckPoint++;


    if (!(::SetServiceStatus(serviceStatusHandle, &serviceStatus)))
    {
        ServiceLogger(_T("[ %s : %d ] SetServiceStatus Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHServiceManager::ReportErrorEvent(LPTSTR lpCurrentFunctionName, DWORD dwCurrentErrorCode)
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    TCHAR errorMessage[128];

    memset(errorMessage, NULL, sizeof(errorMessage));


    _stprintf_s(errorMessage, _T("%s Failed, GetLastError Code = %ld"), lpCurrentFunctionName, dwCurrentErrorCode);


    HANDLE eventSourceHandle = RegisterEventSource(NULL, serviceName);


    if (eventSourceHandle == NULL)
    {
        ServiceLogger(_T("[ %s : %d ] RegisterEventSource Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());


        return;
    }


    LPCTSTR eventLogEntry[2] = {NULL, NULL};


    eventLogEntry[0] = serviceName;


    eventLogEntry[1] = errorMessage;


    WORD wEventLogEntry = (WORD)2;


    if (!(ReportEvent(eventSourceHandle, EVENTLOG_ERROR_TYPE, NULL, NULL, NULL, wEventLogEntry, NULL, eventLogEntry, NULL)))
    {
        ServiceLogger(_T("[ %s : %d ] ReportEvent Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());
    }


    DeregisterEventSource(eventSourceHandle);


    eventSourceHandle = NULL;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHServiceManager::CloseWebSocketSession()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ServiceLogger(_T("[ %s : %d ] CloseWebSocketSession Start, webSocketSessionManager's Size = %ld"), _T(__FUNCTION__), __LINE__, webSocketSessionManager.size());


    for (size_t index = NULL; index < webSocketSessionManager.size(); index++)
    {
        if (webSocketSessionManager[index]->currentClientSocket != NULL)
        {
            closesocket(webSocketSessionManager[index]->currentClientSocket);


            webSocketSessionManager[index]->currentClientSocket = NULL;
        }
    }


    closesocket(webSocketServerSocket);


    webSocketServerSocket = NULL;


    ServiceLogger(_T("[ %s : %d ] Waiting For %ld Thread, Until Clean Up Current WebSocket Session Information INFINITE..."), _T(__FUNCTION__), __LINE__, webSocketServerThreadHandle);


    WaitForSingleObject(webSocketServerThreadHandle, INFINITE);


    CloseHandle(webSocketServerThreadHandle);


    webSocketServerThreadHandle = NULL;


    ServiceLogger(_T("[ %s : %d ] CloseWebSocketSession Done, Remain webSocketSessionManager's Size = %ld"), _T(__FUNCTION__), __LINE__, webSocketSessionManager.size());


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHServiceManager::CloseSensorProcess()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    ServiceLogger(_T("[ %s : %d ] CloseSensorProcess Start, sessionManager's Size = %ld, sessionProcessManager's Size = %ld"), _T(__FUNCTION__), __LINE__, sessionManager.size(), sessionProcessManager.size());


    TCHAR currentUserSessionToken[128];


	while (sessionManager.size())
	{
        size_t index = (sessionManager.size() - (size_t)1);


        if (_tcslen(sessionManager[index]->currentUserSessionToken) > NULL)
        {
            memset(currentUserSessionToken, NULL, sizeof(currentUserSessionToken));


            _stprintf_s(currentUserSessionToken, _T("Global\\CSHSensor|%s"), sessionManager[index]->currentUserSessionToken);


            HANDLE currentUserSessionSignOutEventHandle = OpenEvent(EVENT_ALL_ACCESS, FALSE, currentUserSessionToken);


            if (currentUserSessionSignOutEventHandle == NULL)
            {
                ServiceLogger(_T("[ %s : %d ] OpenEvent Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());
            }
            else
            {
                if (!(SetEvent(currentUserSessionSignOutEventHandle)))
                {
                    ServiceLogger(_T("[ %s : %d ] SetEvent Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, GetLastError());
                }
                else
                {
                    ServiceLogger(_T("[ %s : %d ] Session SignOut Done, sessionManager[%ld]->currentUserSessionToken = %s"), _T(__FUNCTION__), __LINE__, index, sessionManager[index]->currentUserSessionToken);


                    WaitForSingleObject(sessionProcessManager[index]->currentActiveThreadHandle, (DWORD)2000);


                    if (sessionManager[index]->currentUserProcessInformation.hProcess != INVALID_HANDLE_VALUE)
                    {
                        ServiceLogger(_T("[ %s : %d ] TerminateProcess sessionManager[%ld]->currentUserProcessInformation.hProcess = %ld"), _T(__FUNCTION__), __LINE__, index, sessionManager[index]->currentUserProcessInformation.hProcess);


                        TerminateProcess(sessionManager[index]->currentUserProcessInformation.hProcess, NULL);


                        ServiceLogger(_T("[ %s : %d ] Waiting For %ld Thread, Until Clean Up Current Session Information INFINITE..."), _T(__FUNCTION__), __LINE__, sessionProcessManager[index]->currentActiveThreadHandle);


                        WaitForSingleObject(sessionProcessManager[index]->currentActiveThreadHandle, INFINITE);
                    }
                }


                CloseHandle(currentUserSessionSignOutEventHandle);


                currentUserSessionSignOutEventHandle = NULL;
            }
        }
	}


    ServiceLogger(_T("[ %s : %d ] CloseSensorProcess Done, Remain sessionManager's Size = %ld, Remain sessionProcessManager's Size = %ld"), _T(__FUNCTION__), __LINE__, sessionManager.size(), sessionProcessManager.size());


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
