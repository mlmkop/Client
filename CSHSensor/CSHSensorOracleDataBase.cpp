

#include "pch.h"
#include "CSHSensor.h"
#include "CSHSensorOracleDataBase.h"
#include "CSHSensorCryptography.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern CCSHSensorCryptography *pCCSHSensorCryptography;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCSHSensorOracleDataBase::CCSHSensorOracleDataBase(TCHAR *pCurrentOracleUserName, TCHAR *pCurrentOracleUserPassword, TCHAR *pCurrentOracleTableName, TCHAR *pCurrentOracleServiceName) : currentOracleDataBaseConfigurationFlag(FALSE), currentOracleEnvironment(NULL), currentOracleConnection(NULL), currentOracleStatement(NULL), currentOracleResultSet(NULL)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CHAR convertedCurrentOracleUserName[128];

	memset(convertedCurrentOracleUserName, NULL, sizeof(convertedCurrentOracleUserName));


	ConvertWideCharToMultyByteCharANSI(pCurrentOracleUserName, convertedCurrentOracleUserName);


	this->currentOracleUserName.clear();


	this->currentOracleUserName.assign(convertedCurrentOracleUserName);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CHAR convertedCurrentOraclePassword[128];

	memset(convertedCurrentOraclePassword, NULL, sizeof(convertedCurrentOraclePassword));


	ConvertWideCharToMultyByteCharANSI(pCurrentOracleUserPassword, convertedCurrentOraclePassword);


	this->currentOracleUserPassword.clear();


	this->currentOracleUserPassword.assign(convertedCurrentOraclePassword);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CHAR convertedCurrentOracleTableName[128];

	memset(convertedCurrentOracleTableName, NULL, sizeof(convertedCurrentOracleTableName));


	ConvertWideCharToMultyByteCharANSI(pCurrentOracleTableName, convertedCurrentOracleTableName);


	this->currentOracleTableName.clear();


	this->currentOracleTableName.assign(convertedCurrentOracleTableName);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CHAR convertedCurrentOracleServiceName[128];

	memset(convertedCurrentOracleServiceName, NULL, sizeof(convertedCurrentOracleServiceName));


	ConvertWideCharToMultyByteCharANSI(pCurrentOracleServiceName, convertedCurrentOracleServiceName);


	this->currentOracleServiceName.clear();


	this->currentOracleServiceName.assign(convertedCurrentOracleServiceName);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


CCSHSensorOracleDataBase::~CCSHSensorOracleDataBase()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CleanUpOracleInstance();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::CleanUpOracleInstance()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DeleteOracleInstanceResultSet();


	DeleteOracleInstanceStatement();


	DeleteOracleInstanceConnection();


	DeleteOracleInstanceEnvironment();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::CreateOracleInstanceEnvironment()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	while (TRUE)
	{
		Sleep(1000);


		try
		{
			currentOracleEnvironment = oracle::occi::Environment::createEnvironment("KO16MSWIN949", "UTF8", oracle::occi::Environment::DEFAULT);


			if (currentOracleEnvironment != NULL)
			{
				break;
			}
		}
		catch (oracle::occi::SQLException currentSQLException)
		{
			
		}
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::DeleteOracleInstanceEnvironment()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (currentOracleEnvironment != NULL)
	{
		oracle::occi::Environment::terminateEnvironment(currentOracleEnvironment);


		currentOracleEnvironment = NULL;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::CreateOracleInstanceConnection()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	while (!(currentOracleDataBaseConfigurationFlag))
	{
		Sleep(1000);


		try
		{
			currentOracleConnection = currentOracleEnvironment->createConnection(currentOracleUserName, currentOracleUserPassword, currentOracleServiceName);


			if (currentOracleConnection != NULL)
			{
				currentOracleDataBaseConfigurationFlag = TRUE;
			}
		}
		catch (oracle::occi::SQLException currentSQLException)
		{
			
		}
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::DeleteOracleInstanceConnection()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (currentOracleConnection != NULL)
	{
		currentOracleEnvironment->terminateConnection(currentOracleConnection);


		currentOracleConnection = NULL;


		currentOracleDataBaseConfigurationFlag = FALSE;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::CreateOracleInstanceStatement(TCHAR *pCurrentSQLTransactionStatement)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	try
	{
		CHAR convertedCurrentSQLTransactionStatement[512];

		memset(convertedCurrentSQLTransactionStatement, NULL, sizeof(convertedCurrentSQLTransactionStatement));


		ConvertWideCharToMultyByteCharANSI(pCurrentSQLTransactionStatement, convertedCurrentSQLTransactionStatement);


		currentOracleSQLTransactionStatement.assign(convertedCurrentSQLTransactionStatement);


		currentOracleStatement = currentOracleConnection->createStatement(currentOracleSQLTransactionStatement);
	}
	catch (oracle::occi::SQLException currentSQLException)
	{

	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::DeleteOracleInstanceStatement()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (currentOracleStatement != NULL)
	{
		currentOracleConnection->terminateStatement(currentOracleStatement);


		currentOracleStatement = NULL;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


BOOL CCSHSensorOracleDataBase::excuteOracleInstanceStatement(TCHAR *pCurrentActiveUserID, TCHAR *pCurrentActiveUserPassword)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	BOOL returnCode = FALSE;


	try
	{
		currentOracleResultSet = currentOracleStatement->executeQuery();


		if (currentOracleResultSet != NULL)
		{
			if (currentOracleSQLTransactionStatement.find("SELECT") != std::string::npos)
			{
				currentOracleResultSet->next();


				std::string currentOracleResultSetFirstQueryBuffer;


				currentOracleResultSetFirstQueryBuffer.assign(currentOracleResultSet->getString(1));


				if (currentOracleSQLTransactionStatement.find("USER_SIGNIN") != std::string::npos)
				{
					if (!(currentOracleResultSetFirstQueryBuffer.empty()))
					{
						if (currentOracleResultSetFirstQueryBuffer.compare(pCCSHSensorCryptography->cryptedMultyByteStringN) != NULL)
						{
							std::string currentOracleResultSetSecondQueryBuffer;


							currentOracleResultSetSecondQueryBuffer.assign(currentOracleResultSet->getString(2));


							DWORD currentOracleResultSetSecondQueryBufferCompareFlag = NULL;


							if (currentOracleResultSetSecondQueryBuffer.compare(pCCSHSensorCryptography->cryptedMultyByteString1) == NULL)
							{
								currentOracleResultSetSecondQueryBufferCompareFlag = (DWORD)1;


								MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("This Account Has Been Suspended, Please Contact Administrator"), _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));
							}
							else if (currentOracleResultSetSecondQueryBuffer.compare(pCCSHSensorCryptography->cryptedMultyByteString2) == NULL)
							{
								currentOracleResultSetSecondQueryBufferCompareFlag = (DWORD)2;


								MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("This Account Has Been Canceled, Please Contact Administrator"), _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));
							}


							if (currentOracleResultSetSecondQueryBufferCompareFlag != NULL)
							{
								DeleteOracleInstanceResultSet();


								DeleteOracleInstanceStatement();


								DeleteOracleInstanceConnection();


								return returnCode;
							}


							std::string currentOracleResultSetThirdQueryBuffer;


							currentOracleResultSetThirdQueryBuffer.assign(currentOracleResultSet->getString(3));


							if (currentOracleResultSetThirdQueryBuffer.compare(pCCSHSensorCryptography->cryptedMultyByteStringY) != NULL)
							{
								std::string currentOracleResultSetFourthQueryBuffer;


								currentOracleResultSetFourthQueryBuffer.assign(currentOracleResultSet->getString(4));


								if (!(currentOracleResultSetFourthQueryBuffer.empty()))
								{
									TCHAR convertedCurrentOracleResultSetFourthQueryBuffer[512];

									memset(convertedCurrentOracleResultSetFourthQueryBuffer, NULL, sizeof(convertedCurrentOracleResultSetFourthQueryBuffer));


									ConvertMultyByteCharToWideCharUTF8(currentOracleResultSetFourthQueryBuffer.c_str(), convertedCurrentOracleResultSetFourthQueryBuffer);


									pCCSHSensorCryptography->ARIADecrypt(convertedCurrentOracleResultSetFourthQueryBuffer);


									CString convertedCurrentOracleResultSetFourthQueryBufferString;

									
									convertedCurrentOracleResultSetFourthQueryBufferString.Format(_T("%s"), convertedCurrentOracleResultSetFourthQueryBuffer);


									std::vector<INT> tokenManager;


									INT tokenizerPosition = NULL;


									CString currentToken;


									do
									{
										currentToken = convertedCurrentOracleResultSetFourthQueryBufferString.Tokenize(_T("-"), tokenizerPosition);


										tokenManager.push_back(_tcstol(currentToken, NULL, (INT)10));

									} while (!(currentToken.IsEmpty()));


									CTime currentOracleResultSetQueryTimeBuffer(tokenManager.at((size_t)0), tokenManager.at((size_t)1), tokenManager.at((size_t)2), tokenManager.at((size_t)3), tokenManager.at((size_t)4), tokenManager.at((size_t)5));


									CTime currentTime = CTime::GetCurrentTime();


									CTimeSpan timeDifference = (currentTime - currentOracleResultSetQueryTimeBuffer);


									if (timeDifference.GetDays() > (LONGLONG)14)
									{
										DeleteOracleInstanceResultSet();


										DeleteOracleInstanceStatement();


										DeleteOracleInstanceConnection();


										MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("This Account Has Been Blocked More Than 14 days Since Last Sign In, Please Contact Administrator"), _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


										return returnCode;
									}
								}


								std::string currentOracleResultSetFifthQueryBuffer;


								currentOracleResultSetFifthQueryBuffer.assign(currentOracleResultSet->getString(5));


								if (currentOracleResultSetFifthQueryBuffer.compare(pCCSHSensorCryptography->cryptedMultyByteString3) == NULL)
								{
									DeleteOracleInstanceResultSet();


									DeleteOracleInstanceStatement();


									DeleteOracleInstanceConnection();


									MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("This Account Has Been Blocked Due To Incorrect Password, Please Contact Administrator"), _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


									return returnCode;
								}
								

								std::string currentOracleResultSetSixthQueryBuffer;


								currentOracleResultSetSixthQueryBuffer.assign(currentOracleResultSet->getString(6));


								CHAR convertedCurrentActiveUserPassword[512];

								memset(convertedCurrentActiveUserPassword, NULL, sizeof(convertedCurrentActiveUserPassword));


								ConvertWideCharToMultyByteCharUTF8(pCurrentActiveUserPassword, convertedCurrentActiveUserPassword);


								if (currentOracleResultSetSixthQueryBuffer.compare(convertedCurrentActiveUserPassword) != NULL)
								{
									CString currentOracleResultSetFifthQueryBufferString;


									CString errorMessage;


									if (currentOracleResultSetFifthQueryBuffer.compare(pCCSHSensorCryptography->cryptedMultyByteString0) == NULL)
									{
										currentOracleResultSetFifthQueryBufferString.Format(_T("%s"), pCCSHSensorCryptography->cryptedWideString1);


										errorMessage.Format(_T("Sign In Failed. If You Fail More Than 2 Times, This Account Will Be Blocked"));
									}
									else if (currentOracleResultSetFifthQueryBuffer.compare(pCCSHSensorCryptography->cryptedMultyByteString1) == NULL)
									{
										currentOracleResultSetFifthQueryBufferString.Format(_T("%s"), pCCSHSensorCryptography->cryptedWideString2);


										errorMessage.Format(_T("Sign In Failed. If You Fail More Than 1 Times, This Account Will Be Blocked"));
									}
									else
									{
										currentOracleResultSetFifthQueryBufferString.Format(_T("%s"), pCCSHSensorCryptography->cryptedWideString3);


										errorMessage.Format(_T("According To Account Policy, This Account Has Been Blocked"));
									}


									MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONWARNING | MB_TOPMOST));


									TCHAR convertedCurrentOracleTableName[128];

									memset(convertedCurrentOracleTableName, NULL, sizeof(convertedCurrentOracleTableName));


									ConvertMultyByteCharToWideCharANSI(currentOracleTableName.c_str(), convertedCurrentOracleTableName);


									CString signInCountSQLTransactionStatementBuillder;


									signInCountSQLTransactionStatementBuillder.Format(_T("UPDATE %s SET USER_SIGNINCOUNT='%s' WHERE USER_ID='%s'"), convertedCurrentOracleTableName, currentOracleResultSetFifthQueryBufferString.GetBuffer(), pCurrentActiveUserID);


									currentOracleResultSetFifthQueryBufferString.ReleaseBuffer();


									DeleteOracleInstanceResultSet();


									DeleteOracleInstanceStatement();


									CreateOracleInstanceStatement(signInCountSQLTransactionStatementBuillder.GetBuffer());


									signInCountSQLTransactionStatementBuillder.ReleaseBuffer();


									excuteOracleInstanceStatement();


									return returnCode;
								}


								returnCode = TRUE;
							}
							else
							{
								MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("This Account Is Already Signed In"), _T("CSHSensor"), (MB_OK | MB_ICONWARNING | MB_TOPMOST));
							}
						}
						else
						{
							MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("This account Requires Final Confirm From Administrator, Please Contact Administrator"), _T("CSHSensor"), (MB_OK | MB_ICONWARNING | MB_TOPMOST));
						}
					}
					else
					{
						if (MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("Would You Like To Sign Up?"), _T("CSHSensor"), (MB_OKCANCEL | MB_ICONINFORMATION | MB_TOPMOST)) == IDOK)
						{
							returnCode--;
						}
					}
				}
				else
				{
					if (!(currentOracleResultSetFirstQueryBuffer.empty()))
					{
						std::string currentOracleResultSetSecondQueryBuffer;


						currentOracleResultSetSecondQueryBuffer.assign(currentOracleResultSet->getString(2));


						if (currentOracleResultSetSecondQueryBuffer.compare(pCCSHSensorCryptography->cryptedMultyByteStringY) == NULL)
						{
							DeleteOracleInstanceResultSet();


							DeleteOracleInstanceStatement();


							DeleteOracleInstanceConnection();


							MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("This Account Is Already Confirmed"), _T("CSHSensor"), (MB_OK | MB_ICONWARNING | MB_TOPMOST));


							return returnCode;
						}


						returnCode = TRUE;
					}
					else
					{
						MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), _T("Only Can Sign Up Registered In The System"), _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));
					}
				}


				DeleteOracleInstanceResultSet();


				DeleteOracleInstanceStatement();


				DeleteOracleInstanceConnection();


				return returnCode;
			}


			currentOracleConnection->commit();


			DeleteOracleInstanceResultSet();


			returnCode = TRUE;
		}


		DeleteOracleInstanceStatement();


		DeleteOracleInstanceConnection();
	}
	catch (oracle::occi::SQLException currentSQLException)
	{
		CString errorMessage;


		errorMessage.Format(_T("executeQuery Failed, currentSQLException.getErrorCode = %d"), currentSQLException.getErrorCode());


		MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));
	}
	

	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::DeleteOracleInstanceResultSet()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (currentOracleResultSet != NULL)
	{
		currentOracleStatement->closeResultSet(currentOracleResultSet);


		currentOracleResultSet = NULL;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::ConvertWideCharToMultyByteCharANSI(CONST TCHAR *pWideCharString, CHAR *pMultyByteCharString)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	#ifndef _UNICODE


		_stprintf_s(pMultyByteCharString, (_tcslen(pWideCharString) + (size_t)1), pWideCharString);


	#else


		INT pWideCharStringLength = WideCharToMultiByte(CP_ACP, NULL, pWideCharString, (INT)_tcslen(pWideCharString), NULL, NULL, NULL, NULL);


		WideCharToMultiByte(CP_ACP, NULL, pWideCharString, (INT)_tcslen(pWideCharString), pMultyByteCharString, pWideCharStringLength, NULL, NULL);


	#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::ConvertMultyByteCharToWideCharANSI(CONST CHAR *pMultyByteCharString, TCHAR *pWideCharString)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	#ifndef _UNICODE


		_stprintf_s(pWideCharString, (_tcslen(pMultyByteCharString) + (size_t)1), pMultyByteCharString);


	#else


		INT pMultyByteCharStringLength = MultiByteToWideChar(CP_ACP, NULL, pMultyByteCharString, (INT)strlen(pMultyByteCharString), NULL, NULL);


		MultiByteToWideChar(CP_ACP, NULL, pMultyByteCharString, (INT)strlen(pMultyByteCharString), pWideCharString, pMultyByteCharStringLength);


	#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::ConvertWideCharToMultyByteCharUTF8(CONST TCHAR *pWideCharString, CHAR *pMultyByteCharString)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	#ifndef _UNICODE


		INT pWideCharStringLength = MultiByteToWideChar(CP_ACP, NULL, pWideCharString, (INT)_tcslen(pWideCharString), NULL, NULL);


		WCHAR *pUTF8String = new WCHAR[pWideCharStringLength + 1];


		if (pUTF8String == NULL)
		{
			CString errorMessage;


			errorMessage.Format(_T("calloc Failed, GetLastError Code = %ld"), GetLastError());


			MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


			return;
		}


		MultiByteToWideChar(CP_ACP, NULL, pWideCharString, (INT)_tcslen(pWideCharString), pUTF8String, pWideCharStringLength);


		pUTF8String[pWideCharStringLength] = NULL;


		INT pUTF8StringLength = WideCharToMultiByte(CP_UTF8, NULL, pUTF8String, (INT)wcslen(pUTF8String), NULL, NULL, NULL, NULL);


		WideCharToMultiByte(CP_UTF8, NULL, pUTF8String, (INT)wcslen(pUTF8String), pMultyByteCharString, pUTF8StringLength, NULL, NULL);


		delete [] pUTF8String;


		pUTF8String = NULL;


	#else


		INT pWideCharStringLength = WideCharToMultiByte(CP_UTF8, NULL, pWideCharString, (INT)_tcslen(pWideCharString), NULL, NULL, NULL, NULL);


		WideCharToMultiByte(CP_UTF8, NULL, pWideCharString, (INT)_tcslen(pWideCharString), pMultyByteCharString, pWideCharStringLength, NULL, NULL);


	#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::ConvertMultyByteCharToWideCharUTF8(CONST CHAR *pMultyByteCharString, TCHAR *pWideCharString)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	#ifndef _UNICODE


		INT pMultyByteCharStringLength = MultiByteToWideChar(CP_UTF8, NULL, pMultyByteCharString, (INT)_tcslen(pMultyByteCharString), NULL, NULL);


		WCHAR *pUTF8String = new WCHAR[pMultyByteCharStringLength + 1];


		if (pUTF8String == NULL)
		{
			CString errorMessage;


			errorMessage.Format(_T("calloc Failed, GetLastError Code = %ld"), GetLastError());


			MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


			return;
		}


		MultiByteToWideChar(CP_UTF8, NULL, pMultyByteCharString, (INT)_tcslen(pMultyByteCharString), pUTF8String, pMultyByteCharStringLength);


		pUTF8String[pMultyByteCharStringLength] = NULL;


		INT pUTF8StringLength = WideCharToMultiByte(CP_ACP, NULL, pUTF8String, (INT)wcslen(pUTF8String), NULL, NULL, NULL, NULL);


		WideCharToMultiByte(CP_ACP, NULL, pUTF8String, (INT)wcslen(pUTF8String), pWideCharString, pUTF8StringLength, NULL, NULL);


		delete [] pUTF8String;


		pUTF8String = NULL;


	#else


		INT pMultyByteCharStringLength = MultiByteToWideChar(CP_UTF8, NULL, pMultyByteCharString, (INT)strlen(pMultyByteCharString), NULL, NULL);


		MultiByteToWideChar(CP_UTF8, NULL, pMultyByteCharString, (INT)strlen(pMultyByteCharString), pWideCharString, pMultyByteCharStringLength);


	#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

