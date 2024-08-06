

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


	for (INT index = 0; index < 16; index++)
	{
		masterKey[index] = (BYTE)(index * 0x7F);
	}


	for (INT index = 16; index < 32; index++)
	{
		masterKey[index] = (BYTE)((index - 16) * 0xE9);
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


	ConvertWideCharToMultyByteCharANSI(cryptedWideStringY, cryptedMultyByteStringY);


	memset(cryptedMultyByteStringN, NULL, sizeof(cryptedMultyByteStringN));


	ConvertWideCharToMultyByteCharANSI(cryptedWideStringN, cryptedMultyByteStringN);


	memset(cryptedMultyByteString0, NULL, sizeof(cryptedMultyByteString0));


	ConvertWideCharToMultyByteCharANSI(cryptedWideString0, cryptedMultyByteString0);


	memset(cryptedMultyByteString1, NULL, sizeof(cryptedMultyByteString1));


	ConvertWideCharToMultyByteCharANSI(cryptedWideString1, cryptedMultyByteString1);


	memset(cryptedMultyByteString2, NULL, sizeof(cryptedMultyByteString2));


	ConvertWideCharToMultyByteCharANSI(cryptedWideString2, cryptedMultyByteString2);


	memset(cryptedMultyByteString3, NULL, sizeof(cryptedMultyByteString3));


	ConvertWideCharToMultyByteCharANSI(cryptedWideString3, cryptedMultyByteString3);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


VOID CCSHSensorCryptography::ARIAEncrypt(TCHAR *pCurrentQueryBuffer)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	BYTE plainText[128];

	memset(plainText, NULL, sizeof(plainText));


	INT pCurrentQueryBufferLength = (INT)_tcslen(pCurrentQueryBuffer);


	ConvertWideCharToMultyByteCharUTF8(pCurrentQueryBuffer, (LPSTR)plainText);


	memset(pCurrentQueryBuffer, NULL, _tcslen(pCurrentQueryBuffer));


	memset(roundKey, NULL, sizeof(roundKey));


	EncKeySetup(masterKey, roundKey, 256);


	BYTE cryptedCode[256];

	memset(cryptedCode, NULL, sizeof(cryptedCode));


	if ((pCurrentQueryBufferLength % 16) != NULL)
	{
		pCurrentQueryBufferLength = (((pCurrentQueryBufferLength / 16) + 1) * 16);
	}


	LONG offset = NULL;


	LONG currentCryptedBufferLength = (LONG)pCurrentQueryBufferLength;


	while (currentCryptedBufferLength > (LONG)0)
	{
		Crypt((plainText + offset), 16, roundKey, (cryptedCode + offset));


		offset += (LONG)16;


		currentCryptedBufferLength -= (LONG)16;
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


	DecKeySetup(masterKey, roundKey, 256);


	BYTE plainText[128];

	memset(plainText, NULL, sizeof(plainText));


	LONG offset = NULL;


	LONG currentDecryptedBufferLength = (LONG)dwCryptedCode;


	while ((currentDecryptedBufferLength > (LONG)0))
	{
		Crypt((cryptedCode + offset), 16, roundKey, (plainText + offset));


		offset += (LONG)16;


		currentDecryptedBufferLength -= (LONG)16;
	}


	plainText[offset] = NULL;


	ConvertMultyByteCharToWideCharUTF8((LPSTR)plainText, pCurrentQueryBuffer);


	size_t plainTextLength = strlen((LPSTR)plainText);


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


	DWORD pCurrentQueryBufferLength = (DWORD)_tcslen(pCurrentQueryBuffer);


	ConvertWideCharToMultyByteCharUTF8(pCurrentQueryBuffer, (LPSTR)hashedData);


	if (!(CryptHashData(cryptHashHandle, hashedData, pCurrentQueryBufferLength, NULL))) 
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


VOID CCSHSensorCryptography::ConvertWideCharToMultyByteCharANSI(CONST TCHAR *pWideCharString, CHAR *pMultyByteCharString)
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


VOID CCSHSensorCryptography::ConvertMultyByteCharToWideCharANSI(CONST CHAR *pMultyByteCharString, TCHAR *pWideCharString)
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


VOID CCSHSensorCryptography::ConvertWideCharToMultyByteCharUTF8(CONST TCHAR *pWideCharString, CHAR *pMultyByteCharString)
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


VOID CCSHSensorCryptography::ConvertMultyByteCharToWideCharUTF8(CONST CHAR *pMultyByteCharString, TCHAR *pWideCharString)
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
