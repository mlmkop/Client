

#include "pch.h"
#include "CSHSensor.h"
#include "CSHSensorCryptography.h"
#include "wincrypt.h"
#include "atlstr.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CCSHSensorCryptography::CCSHSensorCryptography()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	memset(masterKey, NULL, sizeof(masterKey));


	for (INT index = (INT)0; index < (INT)16; index++)
	{
		masterKey[index] = (BYTE)(index * (INT)0x7F);
	}


	for (INT index = (INT)16; index < (INT)32; index++)
	{
		masterKey[index] = (BYTE)((index - (INT)16) * (INT)0xE9);
	}


	memset(roundKey, NULL, sizeof(roundKey));


	memset(cryptedWideStringY, NULL, sizeof(cryptedWideStringY));


	_stprintf_s(cryptedWideStringY, _T("Y"));


	ARIAEncrypt(cryptedWideStringY);


	memset(cryptedWideStringN, NULL, sizeof(cryptedWideStringN));


	_stprintf_s(cryptedWideStringN, _T("N"));


	ARIAEncrypt(cryptedWideStringN);


	memset(cryptedWideString0, NULL, sizeof(cryptedWideString0));


	_stprintf_s(cryptedWideString0, _T("0"));


	ARIAEncrypt(cryptedWideString0);


	memset(cryptedWideString1, NULL, sizeof(cryptedWideString1));


	_stprintf_s(cryptedWideString1, _T("1"));


	ARIAEncrypt(cryptedWideString1);


	memset(cryptedWideString2, NULL, sizeof(cryptedWideString2));


	_stprintf_s(cryptedWideString2, _T("2"));


	ARIAEncrypt(cryptedWideString2);


	memset(cryptedWideString3, NULL, sizeof(cryptedWideString3));


	_stprintf_s(cryptedWideString3, _T("3"));


	ARIAEncrypt(cryptedWideString3);


	memset(cryptedMultyByteStringY, NULL, sizeof(cryptedMultyByteStringY));


	ConvertWideCharToMultyByteChar(cryptedWideStringY, cryptedMultyByteStringY);


	memset(cryptedMultyByteStringN, NULL, sizeof(cryptedMultyByteStringN));


	ConvertWideCharToMultyByteChar(cryptedWideStringN, cryptedMultyByteStringN);


	memset(cryptedMultyByteString0, NULL, sizeof(cryptedMultyByteString0));


	ConvertWideCharToMultyByteChar(cryptedWideString0, cryptedMultyByteString0);


	memset(cryptedMultyByteString1, NULL, sizeof(cryptedMultyByteString1));


	ConvertWideCharToMultyByteChar(cryptedWideString1, cryptedMultyByteString1);


	memset(cryptedMultyByteString2, NULL, sizeof(cryptedMultyByteString2));


	ConvertWideCharToMultyByteChar(cryptedWideString2, cryptedMultyByteString2);


	memset(cryptedMultyByteString3, NULL, sizeof(cryptedMultyByteString3));


	ConvertWideCharToMultyByteChar(cryptedWideString3, cryptedMultyByteString3);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorCryptography::ARIAEncrypt(TCHAR *pCurrentQueryBuffer)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	BYTE plainText[128];

	memset(plainText, NULL, sizeof(plainText));


	INT pCurrentQueryBufferLength = WideCharToMultiByte(CP_UTF8, NULL, pCurrentQueryBuffer, (INT)_tcslen(pCurrentQueryBuffer), NULL, NULL, NULL, NULL);


	WideCharToMultiByte(CP_UTF8, NULL, pCurrentQueryBuffer, (INT)_tcslen(pCurrentQueryBuffer), (LPSTR)plainText, pCurrentQueryBufferLength, NULL, NULL);


	memset(pCurrentQueryBuffer, NULL, _tcslen(pCurrentQueryBuffer));


	memset(roundKey, NULL, sizeof(roundKey));


	EncKeySetup(masterKey, roundKey, (INT)256);


	BYTE cryptedCode[256];

	memset(cryptedCode, NULL, sizeof(cryptedCode));


	if ((INT)(pCurrentQueryBufferLength % 16) != NULL)
	{
		pCurrentQueryBufferLength = (INT)(((pCurrentQueryBufferLength / 16) + 1) * 16);
	}


	INT offset = NULL;


	while ((LONG)pCurrentQueryBufferLength > 0)
	{
		Crypt((plainText + offset), (INT)16, roundKey, (cryptedCode + offset));


		offset += (INT)16;


		pCurrentQueryBufferLength -= (INT)16;
	}


	cryptedCode[offset] = NULL;


	TCHAR basedCode[512];

	memset(basedCode, NULL, sizeof(basedCode));


	DWORD dwbasedCode = NULL;


	CryptBinaryToString(cryptedCode, (DWORD)offset, (CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF), NULL, &dwbasedCode);


	if (!(CryptBinaryToString(cryptedCode, (DWORD)offset, (CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF), basedCode, &dwbasedCode))) 
	{
		CString errorMessage;


		errorMessage.Format(_T("CryptBinaryToString Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		return;
	}


	memcpy_s(pCurrentQueryBuffer, (rsize_t)(512 - 1), basedCode, (rsize_t)(512 - 1));


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorCryptography::ARIADecrypt(TCHAR *pCurrentQueryBuffer)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	BYTE cryptedCode[256];

	memset(cryptedCode, NULL, sizeof(cryptedCode));


	DWORD dwCryptedCode = NULL;


	CryptStringToBinary(pCurrentQueryBuffer, (DWORD)_tcslen(pCurrentQueryBuffer), CRYPT_STRING_BASE64, NULL, &dwCryptedCode, NULL, NULL);


	if (!(CryptStringToBinary(pCurrentQueryBuffer, (DWORD)_tcslen(pCurrentQueryBuffer), CRYPT_STRING_BASE64, cryptedCode, &dwCryptedCode, NULL, NULL))) 
	{
		CString errorMessage;


		errorMessage.Format(_T("CryptStringToBinary Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		return;
	}


	memset(pCurrentQueryBuffer, NULL, _tcslen(pCurrentQueryBuffer));


	memset(roundKey, NULL, sizeof(roundKey));


	DecKeySetup(masterKey, roundKey, (INT)256);


	BYTE plainText[128];

	memset(plainText, NULL, sizeof(plainText));


	INT offset = NULL;


	while (((LONG)dwCryptedCode > 0))
	{
		Crypt((cryptedCode + offset), (INT)16, roundKey, (plainText + offset));


		offset += (INT)16;


		dwCryptedCode -= (INT)16;
	}


	plainText[offset] = NULL;


	INT plainTextLength = MultiByteToWideChar(CP_UTF8, NULL, (LPCCH)plainText, (INT)strlen((LPSTR)plainText), NULL, NULL);


	MultiByteToWideChar(CP_UTF8, NULL, (LPCCH)plainText, (INT)strlen((LPSTR)plainText), pCurrentQueryBuffer, plainTextLength);


	pCurrentQueryBuffer[plainTextLength] = NULL;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorCryptography::SHA256Encrypt(TCHAR *pCurrentQueryBuffer)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	HCRYPTPROV cryptProvHandle;


	HCRYPTHASH cryptHashHandle;


	if (!(CryptAcquireContext(&cryptProvHandle, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))) 
	{
		CString errorMessage;


		errorMessage.Format(_T("CryptAcquireContext Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		return;
	}


	if (!(CryptCreateHash(cryptProvHandle, CALG_SHA_256, NULL, NULL, &cryptHashHandle))) 
	{
		CString errorMessage;


		errorMessage.Format(_T("CryptCreateHash Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		CryptReleaseContext(cryptProvHandle, NULL);


		cryptProvHandle = NULL;


		return;
	}


	BYTE hashedData[512];

	memset(hashedData, NULL, sizeof(hashedData));


	INT pCurrentQueryBufferLength = WideCharToMultiByte(CP_UTF8, NULL, pCurrentQueryBuffer, (INT)_tcslen(pCurrentQueryBuffer), NULL, NULL, NULL, NULL);


	WideCharToMultiByte(CP_UTF8, NULL, pCurrentQueryBuffer, (INT)_tcslen(pCurrentQueryBuffer), (LPSTR)hashedData, pCurrentQueryBufferLength, NULL, NULL);


	if (!(CryptHashData(cryptHashHandle, hashedData, (DWORD)pCurrentQueryBufferLength, NULL))) 
	{
		CString errorMessage;


		errorMessage.Format(_T("CryptHashData Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		CryptDestroyHash(cryptHashHandle);


		cryptHashHandle = NULL;


		CryptReleaseContext(cryptProvHandle, NULL);


		cryptProvHandle = NULL;


		return;
	}


	DWORD dwHashedData = sizeof(hashedData);


	if (!(CryptGetHashParam(cryptHashHandle, HP_HASHVAL, hashedData, &dwHashedData, NULL))) 
	{
		CString errorMessage;


		errorMessage.Format(_T("CryptGetHashParam Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		CryptDestroyHash(cryptHashHandle);


		cryptHashHandle = NULL;


		CryptReleaseContext(cryptProvHandle, NULL);


		cryptProvHandle = NULL;


		return;
	}


	CryptDestroyHash(cryptHashHandle);


	cryptHashHandle = NULL;


	CryptReleaseContext(cryptProvHandle, NULL);


	DWORD dwHexedCode = NULL;


	cryptProvHandle = NULL;


	CryptBinaryToString(hashedData, dwHashedData, (CRYPT_STRING_HEX | CRYPT_STRING_NOCRLF), NULL, &dwHexedCode);


	if (!(CryptBinaryToString(hashedData, dwHashedData, (CRYPT_STRING_HEX | CRYPT_STRING_NOCRLF), pCurrentQueryBuffer, &dwHexedCode))) 
	{
		CString errorMessage;


		errorMessage.Format(_T("CryptBinaryToString Failed, GetLastError Code = %ld"), GetLastError());


		MessageBox(theApp.m_pMainWnd->GetSafeHwnd(), errorMessage, _T("CSHSensor"), (MB_OK | MB_ICONERROR | MB_TOPMOST));


		return;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorCryptography::ConvertWideCharToMultyByteChar(CONST TCHAR *pWideCharString, CHAR *pMultyByteCharString)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	INT pWideCharStringLength = WideCharToMultiByte(CP_ACP, NULL, pWideCharString, (INT)_tcslen(pWideCharString), NULL, NULL, NULL, NULL);


	WideCharToMultiByte(CP_ACP, NULL, pWideCharString, (INT)_tcslen(pWideCharString), pMultyByteCharString, pWideCharStringLength, NULL, NULL);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorCryptography::ConvertMultyByteCharToWideChar(CONST CHAR *pMultyByteCharString, TCHAR *pWideCharString)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	INT pMultyByteCharStringLength = MultiByteToWideChar(CP_ACP, NULL, pMultyByteCharString, (INT)strlen(pMultyByteCharString), NULL, NULL);


	MultiByteToWideChar(CP_ACP, NULL, pMultyByteCharString, (INT)strlen(pMultyByteCharString), pWideCharString, pMultyByteCharStringLength);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
