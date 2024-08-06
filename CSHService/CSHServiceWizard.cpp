

#include "windows.h"
#include "tchar.h"
#include "CSHServiceWizard.h"
#include "CSHService.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _UNICODE


	#include "stdio.h"


#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


DWORD InstallService()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	TCHAR serviceModulePath[MAX_PATH];

	memset(serviceModulePath, NULL, sizeof(serviceModulePath));


	DWORD dwServiceModulePath = GetModuleFileName(NULL, serviceModulePath, (DWORD)(MAX_PATH - 1));


	if ((dwServiceModulePath == NULL) || (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
	{
		returnCode = GetLastError();


		_tprintf_s(_T("\n\nGetModuleFileName Failed, GetLastError Code = %ld\n\n"), returnCode);


		return returnCode;
	}


	SC_HANDLE SCManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);


	if (SCManagerHandle == NULL)
	{
		returnCode = GetLastError();


		_tprintf_s(_T("\n\nOpenSCManager Failed, GetLastError Code = %ld\n\n"), returnCode);


		return returnCode;
	}


	SC_HANDLE SCServiceHandle = CreateService(SCManagerHandle, SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_ALL_ACCESS, (SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS), SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, serviceModulePath, NULL, NULL, NULL, NULL, NULL);


	if (SCServiceHandle == NULL)
	{
		returnCode = GetLastError();


		_tprintf_s(_T("\n\nCreateService Failed, GetLastError Code = %ld\n\n"), returnCode);


		CloseServiceHandle(SCManagerHandle);


		SCManagerHandle = NULL;


		return returnCode;
	}


	_tprintf_s(_T("\n\nCreateService Done, %s Has Been Installed Completely"), SERVICE_NAME);


	SERVICE_DESCRIPTION serviceDescription;

	memset(&serviceDescription, NULL, sizeof(serviceDescription));


	serviceDescription.lpDescription = (LPTSTR)SERVICE_DESCRIPTION_NAME;


	if (ChangeServiceConfig2(SCServiceHandle, SERVICE_CONFIG_DESCRIPTION, &serviceDescription))
	{
		if (StartService(SCServiceHandle, NULL, NULL))
		{
			returnCode = ERROR_SUCCESS;


			_tprintf_s(_T("\n\nStartService Done, %s Will Be Started As Soon As Possible\n\n"), SERVICE_NAME);
		}
		else
		{
			returnCode = GetLastError();


			_tprintf_s(_T("\n\nStartService Failed, GetLastError Code = %ld\n\n"), returnCode);
		}
	}
	else
	{
		returnCode = GetLastError();


		_tprintf_s(_T("\n\nChangeServiceConfig2 Failed, GetLastError Code = %ld\n\n"), returnCode);
	}


	CloseServiceHandle(SCServiceHandle);


	SCServiceHandle = NULL;


	CloseServiceHandle(SCManagerHandle);


	SCManagerHandle = NULL;


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD UninstallService()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	SC_HANDLE SCManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);


	if (SCManagerHandle == NULL)
	{
		returnCode = GetLastError();


		_tprintf_s(_T("\n\nOpenSCManager Failed, GetLastError Code = %ld\n\n"), returnCode);


		return returnCode;
	}


	SC_HANDLE SCServiceHandle = OpenService(SCManagerHandle, SERVICE_NAME, SERVICE_ALL_ACCESS);


	if (SCServiceHandle == NULL)
	{
		returnCode = GetLastError();


		_tprintf_s(_T("\n\nOpenService Failed, GetLastError Code = %ld\n\n"), returnCode);


		CloseServiceHandle(SCManagerHandle);


		SCManagerHandle = NULL;


		return returnCode;
	}


	SERVICE_STATUS serviceStatus;

	memset(&serviceStatus, NULL, sizeof(serviceStatus));


	if (ControlService(SCServiceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
	{
		while (QueryServiceStatus(SCServiceHandle, &serviceStatus))
		{
			Sleep(1000);


			if (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				_tprintf_s(_T("\n\nCurrent Status Is SERVICE_STOP_PENDING, Please Wait A Second, %s Will Be Stopped As Soon As Possible"), SERVICE_NAME);
			}
			else
			{
				break;
			}
		}


		if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
		{
			_tprintf_s(_T("\n\nControlService Done, %s Has Been Stopped"), SERVICE_NAME);


			if (DeleteService(SCServiceHandle))
			{
				returnCode = ERROR_SUCCESS;


				_tprintf_s(_T("\n\nDeleteService Done, %s Has Been UnInstalled Completely\n\n"), SERVICE_NAME);
			}
			else
			{
				returnCode = GetLastError();


				_tprintf_s(_T("\n\nDeleteService Failed, GetLastError Code = %ld\n\n"), returnCode);
			}
		}
	}
	else
	{
		returnCode = GetLastError();


		_tprintf_s(_T("\n\nControlService Failed, GetLastError Code = %ld\n\n"), returnCode);
	}


	CloseServiceHandle(SCServiceHandle);


	SCServiceHandle = NULL;


	CloseServiceHandle(SCManagerHandle);


	SCManagerHandle = NULL;


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
