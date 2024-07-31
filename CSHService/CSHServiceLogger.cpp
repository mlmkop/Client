

#include "windows.h"
#include "tchar.h"
#include "CSHServiceLogger.h"
#include "CSHService.h"
#include "time.h"
#include "fstream"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _UNICODE


	#include "stdio.h"


#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


using namespace std;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef _UNICODE


	typedef basic_fstream<wchar_t, char_traits<wchar_t>> wfstream, tfstream;


#else


	typedef basic_fstream<char, char_traits<char>> fstream, tfstream;


#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


VOID ServiceLogger(LPCTSTR currentLogContents, ...)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	#ifndef DISABLE_SERVICELOGGER
	

		TCHAR *pCurrentLogContents = (TCHAR *)calloc((size_t)1, (size_t)1024);


		if (pCurrentLogContents == NULL)
		{
			_tprintf(_T("calloc Failed, GetLastError Code = %ld\n\n"), GetLastError());


			return;
		}


		time_t currentTime = time(NULL);


		struct tm currentTimeSet;

		memset(&currentTimeSet, NULL, sizeof(currentTimeSet));


		localtime_s(&currentTimeSet, &currentTime);


		TCHAR timeBuffer[128];

		memset(timeBuffer, NULL, sizeof(timeBuffer));


		_tcsftime(timeBuffer, (size_t)(128 - 1), _T("[ %Y-%m-%d %H:%M:%S ]"), &currentTimeSet);


		_stprintf_s(pCurrentLogContents, (size_t)(128 - 1), _T("%s"), timeBuffer);


		va_list vaList = NULL;


		va_start(vaList, currentLogContents);


		#ifdef _UNICODE


			vswprintf((pCurrentLogContents + _tcslen(timeBuffer)), ((size_t)(1024 - 1) - _tcslen(timeBuffer)), currentLogContents, vaList);


		#else


			vsnprintf((pCurrentLogContents + _tcslen(timeBuffer)), ((size_t)(1024 - 1) - _tcslen(timeBuffer)), currentLogContents, vaList);


		#endif


		va_end(vaList);


		memset(timeBuffer, NULL, sizeof(timeBuffer));


		_tcsftime(timeBuffer, (size_t)(128 - 1), _T("_%Y-%m-%d"), &currentTimeSet);


		TCHAR serviceModulePath[MAX_PATH];

		memset(serviceModulePath, NULL, sizeof(serviceModulePath));


		DWORD dwServiceModulePath = GetModuleFileName(NULL, serviceModulePath, (DWORD)(MAX_PATH - 1));


		if ((dwServiceModulePath == NULL) || (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
		{
			_tprintf(_T("GetModuleFileName Failed, GetLastError Code = %ld\n\n"), GetLastError());


			free(pCurrentLogContents);


			pCurrentLogContents = NULL;


			return;
		}


		TCHAR drive[_MAX_DRIVE];

		memset(drive, NULL, sizeof(drive));


		TCHAR directory[_MAX_DIR];

		memset(directory, NULL, sizeof(directory));


		_tsplitpath_s(serviceModulePath, drive, _MAX_DRIVE, directory, (size_t)(_MAX_DIR - 1), NULL, NULL, NULL, NULL);


		_tmakepath_s(serviceModulePath, drive, directory, NULL, NULL);


		TCHAR logFilePath[MAX_PATH];

		memset(logFilePath, NULL, sizeof(logFilePath));


		_stprintf_s(logFilePath, _T("%s\\%s%s.log"), serviceModulePath, SERVICE_NAME, timeBuffer);


		tfstream logFile;


		logFile.open(logFilePath, (ios::out | ios::app));


		if (logFile.is_open())
		{
			logFile.write(pCurrentLogContents, (streamsize)_tcslen(pCurrentLogContents));


			logFile << endl;


			logFile.close();
		}


		free(pCurrentLogContents);


		pCurrentLogContents = NULL;

	
	#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
