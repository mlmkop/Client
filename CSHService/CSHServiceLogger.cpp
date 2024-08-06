

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


#ifndef _UNICODE


	typedef std::basic_fstream<char, std::char_traits<char>> fstream, tfstream;


#else


	typedef std::basic_fstream<wchar_t, std::char_traits<wchar_t>> wfstream, tfstream;


#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


VOID ServiceLogger(LPCTSTR currentLogContents, ...)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	#ifndef DISABLE_SERVICELOGGER
	

		if (_tcslen(currentLogContents) == NULL)
		{
			return;
		}


		TCHAR currentLogContentsBuffer[1024];

		memset(currentLogContentsBuffer, NULL, sizeof(currentLogContentsBuffer));


		time_t currentTime = time(NULL);


		struct tm currentTimeSet;

		memset(&currentTimeSet, NULL, sizeof(currentTimeSet));


		localtime_s(&currentTimeSet, &currentTime);


		TCHAR timeBuffer[128];

		memset(timeBuffer, NULL, sizeof(timeBuffer));


		_tcsftime(timeBuffer, (size_t)(128 - 1), _T("[ %Y-%m-%d %H:%M:%S ]"), &currentTimeSet);


		_stprintf_s(currentLogContentsBuffer, timeBuffer);


		va_list vaList = NULL;


		va_start(vaList, currentLogContents);


		_vstprintf_s((currentLogContentsBuffer + _tcslen(timeBuffer)), ((size_t)(1024 - 1) - _tcslen(timeBuffer)), currentLogContents, vaList);


		va_end(vaList);


		memset(timeBuffer, NULL, sizeof(timeBuffer));


		_tcsftime(timeBuffer, (size_t)(128 - 1), _T("_%Y-%m-%d"), &currentTimeSet);


		TCHAR serviceModulePath[MAX_PATH];

		memset(serviceModulePath, NULL, sizeof(serviceModulePath));


		DWORD dwServiceModulePath = GetModuleFileName(NULL, serviceModulePath, (DWORD)(MAX_PATH - 1));


		if ((dwServiceModulePath == NULL) || (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
		{
			_tprintf(_T("GetModuleFileName Failed, GetLastError Code = %ld\n\n"), GetLastError());


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


		logFile.open(logFilePath, (std::ios::out | std::ios::app));


		if (logFile.is_open())
		{
			logFile.write(currentLogContentsBuffer, (std::streamsize)_tcslen(currentLogContentsBuffer));


			logFile << std::endl;


			logFile.close();
		}

	
	#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
