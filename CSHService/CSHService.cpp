

#include "windows.h"
#include "tchar.h"
#include "CSHService.h"
#include "CSHServiceWizard.h"
#include "CSHServiceManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _UNICODE


	#include "stdio.h"


#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


INT _tmain(INT argc, TCHAR *argv[])
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	INT returnCode = 1;


	if (GetConsoleWindow() == NULL) 
	{
		CCSHServiceManager CSHServiceManager((LPTSTR)SERVICE_NAME);


		returnCode = (INT)CSHServiceManager.ServiceRun(CSHServiceManager);
	}
	else
	{
		if ((argc > 1) && (((*(argv[1]) == '-') || (*(argv[1]) == '/'))))
		{
			if (_tccmp(_T("i"), (argv[1] + 1)) == NULL)
			{
				returnCode = (INT)InstallService();
			}
			else if (_tccmp(_T("u"), (argv[1] + 1)) == NULL)
			{
				returnCode = (INT)UninstallService();
			}
			else
			{
				_tprintf_s(_T("\n\n"));

			
				_tprintf_s(_T("Please Check The Second Option For Use\n\n"));


				_tprintf_s(_T("Parameter : [ -i or /i ], It Means To Install CSH Data Loss Prevention Solution\n\n"));


				_tprintf_s(_T("Parameter : [ -u or /u ], It Means To Uninstall CSH Data Loss Prevention Solution\n\n"));
			}
		}
		else
		{
			_tprintf_s(_T("\n\n"));


			_tprintf_s(_T("Please Check The Second Option For Use\n\n"));


			_tprintf_s(_T("Parameter : [ -i or /i ], It Means To Install CSH Data Loss Prevention Solution\n\n"));


			_tprintf_s(_T("Parameter : [ -u or /u ], It Means To Uninstall CSH Data Loss Prevention Solution\n\n"));
		}
	}


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
