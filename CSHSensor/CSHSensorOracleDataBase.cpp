

#include "pch.h"
#include "CSHSensor.h"
#include "CSHSensorOracleDataBase.h"
#include "CSHSensorCryptography.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CString oracleDataBaseUserTableName;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern CCSHSensorCryptography *pCCSHSensorCryptography;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCSHSensorOracleDataBase::CCSHSensorOracleDataBase(TCHAR *pCurrentOracleUserName, TCHAR *pCurrentOracleUserPassword, TCHAR *pCurrentOracleServiceName) : currentOracleDataBaseConfigurationFlag(FALSE), currentOracleEnvironment(NULL), currentOracleConnection(NULL), currentOracleStatement(NULL), currentOracleResultSet(NULL)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CHAR convertedCurrentOracleUserName[512];

	memset(convertedCurrentOracleUserName, NULL, sizeof(convertedCurrentOracleUserName));


	ConvertWideCharToMultyByteChar(pCurrentOracleUserName, convertedCurrentOracleUserName);


	this->currentOracleUserName.clear();


	this->currentOracleUserName.assign(convertedCurrentOracleUserName);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CHAR convertedCurrentOraclePassword[512];

	memset(convertedCurrentOraclePassword, NULL, sizeof(convertedCurrentOraclePassword));


	ConvertWideCharToMultyByteChar(pCurrentOracleUserPassword, convertedCurrentOraclePassword);


	this->currentOracleUserPassword.clear();


	this->currentOracleUserPassword.assign(convertedCurrentOraclePassword);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CHAR convertedCurrentOracleServiceName[512];

	memset(convertedCurrentOracleServiceName, NULL, sizeof(convertedCurrentOracleServiceName));


	ConvertWideCharToMultyByteChar(pCurrentOracleServiceName, convertedCurrentOracleServiceName);


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


	while (!(currentOracleDataBaseConfigurationFlag))
	{
		Sleep(1000);


		try
		{
			currentOracleEnvironment = oracle::occi::Environment::createEnvironment("KO16MSWIN949", "UTF8", oracle::occi::Environment::DEFAULT);


			if (currentOracleEnvironment != NULL)
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


VOID CCSHSensorOracleDataBase::DeleteOracleInstanceEnvironment()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	if (currentOracleEnvironment != NULL)
	{
		oracle::occi::Environment::terminateEnvironment(currentOracleEnvironment);


		currentOracleEnvironment = NULL;


		currentOracleDataBaseConfigurationFlag = FALSE;
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


		INT currentSQLTransactionStatementLength = WideCharToMultiByte(CP_ACP, NULL, pCurrentSQLTransactionStatement, (INT)_tcslen(pCurrentSQLTransactionStatement), NULL, NULL, NULL, NULL);


		WideCharToMultiByte(CP_ACP, NULL, pCurrentSQLTransactionStatement, (INT)_tcslen(pCurrentSQLTransactionStatement), convertedCurrentSQLTransactionStatement, currentSQLTransactionStatementLength, NULL, NULL);


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


									ConvertMultyByteCharToWideChar(currentOracleResultSetFourthQueryBuffer.c_str(), convertedCurrentOracleResultSetFourthQueryBuffer);


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


								ConvertWideCharToMultyByteChar(pCurrentActiveUserPassword, convertedCurrentActiveUserPassword);


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


									CString signInCountSQLTransactionStatementBuillder;


									signInCountSQLTransactionStatementBuillder.Format(_T("UPDATE %s SET USER_SIGNINCOUNT='%s' WHERE USER_ID='%s'"), oracleDataBaseUserTableName.GetBuffer(), currentOracleResultSetFifthQueryBufferString.GetBuffer(), pCurrentActiveUserID);


									oracleDataBaseUserTableName.ReleaseBuffer();


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


VOID CCSHSensorOracleDataBase::ConvertWideCharToMultyByteChar(CONST TCHAR *pWideCharString, CHAR *pMultyByteCharString)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	INT pWideCharStringLength = WideCharToMultiByte(CP_ACP, NULL, pWideCharString, (INT)_tcslen(pWideCharString), NULL, NULL, NULL, NULL);


	WideCharToMultiByte(CP_ACP, NULL, pWideCharString, (INT)_tcslen(pWideCharString), pMultyByteCharString, pWideCharStringLength, NULL, NULL);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorOracleDataBase::ConvertMultyByteCharToWideChar(CONST CHAR *pMultyByteCharString, TCHAR *pWideCharString)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	INT pMultyByteCharStringLength = MultiByteToWideChar(CP_ACP, NULL, pMultyByteCharString, (INT)strlen(pMultyByteCharString), NULL, NULL);


	MultiByteToWideChar(CP_ACP, NULL, pMultyByteCharString, (INT)strlen(pMultyByteCharString), pWideCharString, pMultyByteCharStringLength);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

