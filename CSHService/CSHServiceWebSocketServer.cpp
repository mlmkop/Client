

#include "ws2tcpip.h"
#include "windows.h"
#include "tchar.h"
#include "CSHServiceWebSocketServer.h"
#include "CSHServiceLogger.h"
#include "vector"
#include "atlstr.h"
#include "iphlpapi.h"
#include "wincrypt.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _UNICODE


	#include "stdio.h"


#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


using namespace std;


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


extern vector<WEBSOCKETSESSIONMANAGER *> webSocketSessionManager;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern BOOL closeWebSocketSessionFlag;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


extern SOCKET webSocketServerSocket;


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


extern vector<SESSIONMANAGER *> sessionManager;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


DWORD AllClientSignOut()
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	CString currentUserSessionToken;


	for (size_t index = NULL; index < sessionManager.size(); index++)
	{
		if (sessionManager[index]->currentUserSessionToken != NULL)
		{
			currentUserSessionToken.Format(_T("Global\\CSHSensor|%s"), sessionManager[index]->currentUserSessionToken);


			HANDLE allClientSignOutEventHandle = OpenEvent(EVENT_ALL_ACCESS, FALSE, currentUserSessionToken.GetBuffer());


			currentUserSessionToken.ReleaseBuffer();


			if (allClientSignOutEventHandle == NULL)
			{
				returnCode = GetLastError();


				ServiceLogger(_T("[ %s : %d ] OpenEvent Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
			}
			else
			{
				if (!(SetEvent(allClientSignOutEventHandle)))
				{
					returnCode = GetLastError();


					ServiceLogger(_T("[ %s : %d ] SetEvent Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
				}
				else
				{
					returnCode = ERROR_SUCCESS;
				}


				CloseHandle(allClientSignOutEventHandle);


				allClientSignOutEventHandle = NULL;
			}
		}
	}


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD SpecificClientSignOut(CString &currentSpecificClientSessionToken)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	HANDLE specificClientSignOutEventHandle = OpenEvent(EVENT_ALL_ACCESS, FALSE, currentSpecificClientSessionToken.GetBuffer());


	currentSpecificClientSessionToken.ReleaseBuffer();


	if (specificClientSignOutEventHandle == NULL)
	{
		returnCode = GetLastError();


		ServiceLogger(_T("[ %s : %d ] OpenEvent Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
	}
	else
	{
		if (!(SetEvent(specificClientSignOutEventHandle)))
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] SetEvent Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
		}
		else
		{
			returnCode = ERROR_SUCCESS;
		}


		CloseHandle(specificClientSignOutEventHandle);


		specificClientSignOutEventHandle = NULL;
	}


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD TranslateWebSocketMessage(VOID *lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	WEBSOCKETSESSIONMANAGER *pCurrentClientWebSocketConnection = (WEBSOCKETSESSIONMANAGER *)lpVoid;


	while (!(closeWebSocketSessionFlag))
	{
		if (recv(pCurrentClientWebSocketConnection->currentClientSocket, pCurrentClientWebSocketConnection->currentClientBuffer, (1024 - 1), NULL) == SOCKET_ERROR)
		{
			returnCode = (DWORD)WSAGetLastError();


			if (!(closeWebSocketSessionFlag))
			{
				ServiceLogger(_T("[ %s : %d ] recv Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
			}


			return returnCode;
		}


		BYTE bitScanner = (BYTE)pCurrentClientWebSocketConnection->currentClientBuffer[0];


		if (bitScanner == (BYTE)0x88)
		{
			ServiceLogger(_T("[ %s : %d ] Detected To WebSocket Connection Disconnect, pCurrentClientWebSocketConnection = %zd"), _T(__FUNCTION__), __LINE__, pCurrentClientWebSocketConnection);


			vector<WEBSOCKETSESSIONMANAGER *>::iterator currentWebSocketSessionCursor = webSocketSessionManager.begin();


			for (size_t index = NULL; index < webSocketSessionManager.size(); index++, currentWebSocketSessionCursor++)
			{
				if (webSocketSessionManager[index] == pCurrentClientWebSocketConnection)
				{
					if (webSocketSessionManager[index]->currentClientSocket != NULL)
					{
						closesocket(webSocketSessionManager[index]->currentClientSocket);


						webSocketSessionManager[index]->currentClientSocket = NULL;
					}


					memset(webSocketSessionManager[index], NULL, sizeof(WEBSOCKETSESSIONMANAGER));


					free(webSocketSessionManager[index]);


					webSocketSessionManager[index] = NULL;


					break;
				}
			}


			webSocketSessionManager.erase(currentWebSocketSessionCursor);


			break;
		}


		BYTE sizeProtocol = (BYTE)(pCurrentClientWebSocketConnection->currentClientBuffer[1] & (CHAR)0x7F);


		UINT64 payLoadSize = NULL;


		INT maskOffSet = NULL;


		if (sizeProtocol <= (BYTE)0x7D)
		{
			payLoadSize = (UINT64)sizeProtocol;


			maskOffSet = 2;
		}
		else if (sizeProtocol == (BYTE)0x7E)
		{
			payLoadSize = (UINT64)ntohs(*(u_short *)(pCurrentClientWebSocketConnection->currentClientBuffer + 2));


			maskOffSet = 4;
		}
		else if (sizeProtocol == (BYTE)0x7F)
		{
			payLoadSize = (UINT64)ntohl(*(u_long *)(pCurrentClientWebSocketConnection->currentClientBuffer + 2));


			maskOffSet = 10;
		}
		else
		{
			ServiceLogger(_T("[ %s : %d ] Contains Invalid Header Information In Received WebSocketMessage, sizeProtocol = %d"), _T(__FUNCTION__), __LINE__, sizeProtocol);


			continue;
		}


		BYTE mask[4];

		memset(mask, NULL, sizeof(mask));


		memcpy_s(mask, (rsize_t)4, (pCurrentClientWebSocketConnection->currentClientBuffer + maskOffSet), (rsize_t)4);


		CHAR *pUnMaskedData = (CHAR *)calloc((size_t)1, (size_t)(payLoadSize + (UINT64)1));


		if (pUnMaskedData == NULL)
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] calloc Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			continue;
		}


		memcpy_s(pUnMaskedData, (rsize_t)payLoadSize, (pCurrentClientWebSocketConnection->currentClientBuffer + maskOffSet + 4), (rsize_t)payLoadSize);


		for (UINT64 index = NULL; index < payLoadSize; index++)
		{
			pUnMaskedData[index] = (pUnMaskedData[index] ^ mask[(index % (UINT64)4)]);
		}


		pUnMaskedData[payLoadSize] = NULL;


		TCHAR messageConverter[1024];

		memset(messageConverter, NULL, sizeof(messageConverter));


		INT pUnMaskedDataLength = MultiByteToWideChar(CP_ACP, NULL, pUnMaskedData, (INT)strlen(pUnMaskedData), NULL, NULL);


		MultiByteToWideChar(CP_ACP, NULL, pUnMaskedData, (INT)strlen(pUnMaskedData), messageConverter, pUnMaskedDataLength);


		free(pUnMaskedData);


		pUnMaskedData = NULL;


		CString receivedWebSocketMessage(messageConverter);


		if (receivedWebSocketMessage.Find(_T("AllClientSignOut")) > (-1))
		{
			if (AllClientSignOut() == ERROR_SUCCESS)
			{
				receivedWebSocketMessage.Format(_T("AllClientSignOut Done"));
			}
			else
			{
				receivedWebSocketMessage.Format(_T("AllClientSignOut Failed"));
			}
		}
		else if (receivedWebSocketMessage.Find(_T("SpecificClientSignOut")) > (-1))
		{
			INT startPosition = (receivedWebSocketMessage.Find(_T("&")) + 1);


			INT endPosition = receivedWebSocketMessage.Find(_T("&"), startPosition);


			CString specificClientSessionToken(_T("Global\\CSHSensor|"));


			specificClientSessionToken.Append(receivedWebSocketMessage.Mid(startPosition, (endPosition - startPosition)));


			if (SpecificClientSignOut(specificClientSessionToken) == ERROR_SUCCESS)
			{
				receivedWebSocketMessage.Format(_T("SpecificClientSignOut Done"));
			}
			else
			{
				receivedWebSocketMessage.Format(_T("SpecificClientSignOut Failed"));
			}
		}
		else
		{
			receivedWebSocketMessage.Format(_T("None-Reserved Message"));
		}


		memset(messageConverter, NULL, sizeof(messageConverter));


		_stprintf_s(messageConverter, receivedWebSocketMessage.GetBuffer());


		receivedWebSocketMessage.ReleaseBuffer();


		memset(pCurrentClientWebSocketConnection->currentClientBuffer, NULL, (size_t)1024);


		payLoadSize = (UINT64)receivedWebSocketMessage.GetLength();


		pCurrentClientWebSocketConnection->currentClientBuffer[0] = (CHAR)0x81;


		if (payLoadSize <= (UINT64)0x7D)
		{
			pCurrentClientWebSocketConnection->currentClientBuffer[1] = (CHAR)payLoadSize;


			maskOffSet = 2;
		}
		else if ((payLoadSize > (UINT64)0x7D) && (payLoadSize <= (UINT64)0xFFFF))
		{
			pCurrentClientWebSocketConnection->currentClientBuffer[1] = (CHAR)0x7E;


			pCurrentClientWebSocketConnection->currentClientBuffer[2] = (CHAR)((payLoadSize >> 8) & (UINT64)0xFF);


			pCurrentClientWebSocketConnection->currentClientBuffer[3] = (CHAR)(payLoadSize & 0xFF);


			maskOffSet = 4;
		}
		else
		{
			pCurrentClientWebSocketConnection->currentClientBuffer[1] = (CHAR)0x7F;


			pCurrentClientWebSocketConnection->currentClientBuffer[2] = (CHAR)((payLoadSize >> 56) & (UINT64)0xFF);


			pCurrentClientWebSocketConnection->currentClientBuffer[3] = (CHAR)((payLoadSize >> 48) & (UINT64)0xFF);


			pCurrentClientWebSocketConnection->currentClientBuffer[4] = (CHAR)((payLoadSize >> 40) & (UINT64)0xFF);


			pCurrentClientWebSocketConnection->currentClientBuffer[5] = (CHAR)((payLoadSize >> 32) & (UINT64)0xFF);


			pCurrentClientWebSocketConnection->currentClientBuffer[6] = (CHAR)((payLoadSize >> 24) & (UINT64)0xFF);


			pCurrentClientWebSocketConnection->currentClientBuffer[7] = (CHAR)((payLoadSize >> 16) & (UINT64)0xFF);


			pCurrentClientWebSocketConnection->currentClientBuffer[8] = (CHAR)((payLoadSize >> 8) & (UINT64)0xFF);


			pCurrentClientWebSocketConnection->currentClientBuffer[9] = (CHAR)(payLoadSize & (UINT64)0xFF);


			maskOffSet = 10;
		}


		INT messageConverterLength = WideCharToMultiByte(CP_UTF8, NULL, messageConverter, (INT)_tcslen(messageConverter), NULL, NULL, NULL, NULL);


		WideCharToMultiByte(CP_UTF8, NULL, messageConverter, (INT)_tcslen(messageConverter), (pCurrentClientWebSocketConnection->currentClientBuffer + maskOffSet), messageConverterLength, NULL, NULL);


		if (send(pCurrentClientWebSocketConnection->currentClientSocket, pCurrentClientWebSocketConnection->currentClientBuffer, (INT)strlen(pCurrentClientWebSocketConnection->currentClientBuffer), NULL) == SOCKET_ERROR)
		{
			returnCode = (DWORD)WSAGetLastError();


			if (!(closeWebSocketSessionFlag))
			{
				ServiceLogger(_T("[ %s : %d ] send Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
			}
		}


		memset(pCurrentClientWebSocketConnection->currentClientBuffer, NULL, (size_t)1024);


		returnCode = ERROR_SUCCESS;
	}


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD WebSocketClientHandshaking(VOID *lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	WEBSOCKETSESSIONMANAGER *pCurrentClientWebSocketConnection = (WEBSOCKETSESSIONMANAGER *)lpVoid;


	if (recv(pCurrentClientWebSocketConnection->currentClientSocket, pCurrentClientWebSocketConnection->currentClientBuffer, (1024 - 1), NULL) == SOCKET_ERROR)
	{
		returnCode = (DWORD)WSAGetLastError();


		if (!(closeWebSocketSessionFlag))
		{
			ServiceLogger(_T("[ %s : %d ] recv Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
		}


		return returnCode;
	}


	CString receivedMessage(pCurrentClientWebSocketConnection->currentClientBuffer);


	INT startPosition = receivedMessage.Find(_T("Sec-WebSocket-Key:"));


	INT endPosition = receivedMessage.Find(_T("\r\n"), startPosition);


	CString keyValueMap(receivedMessage.Mid(startPosition, (endPosition - startPosition)));


	startPosition = keyValueMap.Find(_T(":"));


	endPosition = (keyValueMap.GetLength() - startPosition);


	CStringA parsedValue(keyValueMap.Mid((startPosition + 2), endPosition));


	parsedValue.Append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");


	HCRYPTPROV cryptProvHandle;


	HCRYPTHASH cryptHashHandle;


	if (!(CryptAcquireContext(&cryptProvHandle, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)))
	{
		returnCode = GetLastError();


		ServiceLogger(_T("[ %s : %d ] CryptAcquireContext Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		return returnCode;
	}


	if (!(CryptCreateHash(cryptProvHandle, CALG_SHA1, NULL, NULL, &cryptHashHandle)))
	{
		returnCode = GetLastError();


		ServiceLogger(_T("[ %s : %d ] CryptCreateHash Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		CryptReleaseContext(cryptProvHandle, NULL);


		cryptProvHandle = NULL;


		return returnCode;
	}


	BYTE encodedData[128];

	memset(encodedData, NULL, sizeof(encodedData));


	sprintf_s((LPSTR)encodedData, (size_t)(128 - 1), parsedValue.GetBuffer());


	parsedValue.ReleaseBuffer();


	if (!(CryptHashData(cryptHashHandle, encodedData, (DWORD)parsedValue.GetLength(), NULL)))
	{
		returnCode = GetLastError();


		ServiceLogger(_T("[ %s : %d ] CryptHashData Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		CryptDestroyHash(cryptHashHandle);


		cryptHashHandle = NULL;


		CryptReleaseContext(cryptProvHandle, NULL);


		cryptProvHandle = NULL;


		return returnCode;
	}


	DWORD dwEncodedData = sizeof(encodedData);


	if (!(CryptGetHashParam(cryptHashHandle, HP_HASHVAL, encodedData, &dwEncodedData, NULL)))
	{
		returnCode = GetLastError();


		ServiceLogger(_T("[ %s : %d ] CryptGetHashParam Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		CryptDestroyHash(cryptHashHandle);


		cryptHashHandle = NULL;


		CryptReleaseContext(cryptProvHandle, NULL);


		cryptProvHandle = NULL;


		return returnCode;
	}


	CryptDestroyHash(cryptHashHandle);


	cryptHashHandle = NULL;


	CryptReleaseContext(cryptProvHandle, NULL);


	cryptProvHandle = NULL;


	DWORD dwBasedCode = NULL;


	CryptBinaryToString(encodedData, dwEncodedData, (CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF), NULL, &dwBasedCode);


	TCHAR basedCode[128];

	memset(basedCode, NULL, sizeof(basedCode));


	if (!(CryptBinaryToString(encodedData, dwEncodedData, (CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF), basedCode, &dwBasedCode)))
	{
		returnCode = GetLastError();


		ServiceLogger(_T("[ %s : %d ] CryptBinaryToString Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		return returnCode;
	}


	TCHAR webSocketClientHandshakingMessage[1024];

	memset(webSocketClientHandshakingMessage, NULL, sizeof(webSocketClientHandshakingMessage));


	_stprintf_s(webSocketClientHandshakingMessage, _T("HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nUpgrade: websocket\r\n\r\n"), basedCode);


	memset(pCurrentClientWebSocketConnection->currentClientBuffer, NULL, (size_t)1024);


	INT webSocketClientHandshakingMessageLength = WideCharToMultiByte(CP_UTF8, NULL, webSocketClientHandshakingMessage, (INT)_tcslen(webSocketClientHandshakingMessage), NULL, NULL, NULL, NULL);


	WideCharToMultiByte(CP_UTF8, NULL, webSocketClientHandshakingMessage, (INT)_tcslen(webSocketClientHandshakingMessage), pCurrentClientWebSocketConnection->currentClientBuffer, webSocketClientHandshakingMessageLength, NULL, NULL);


	if (send(pCurrentClientWebSocketConnection->currentClientSocket, pCurrentClientWebSocketConnection->currentClientBuffer, (INT)strlen(pCurrentClientWebSocketConnection->currentClientBuffer), NULL) == SOCKET_ERROR)
	{
		returnCode = (DWORD)WSAGetLastError();


		if (!(closeWebSocketSessionFlag))
		{
			ServiceLogger(_T("[ %s : %d ] send Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
		}


		return returnCode;
	}


	memset(pCurrentClientWebSocketConnection->currentClientBuffer, NULL, (size_t)1024);


	returnCode = ERROR_SUCCESS;


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD GetCurrentActiveUserAdapterInformation(TCHAR *pIpv4)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	IP_ADAPTER_ADDRESSES ipAdapterAddresses[32];


	ULONG ulIpAdapterAddresses = sizeof(ipAdapterAddresses);


	ULONG currentActiveAdaptersAddressesStatus = GetAdaptersAddresses(PF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, ipAdapterAddresses, &ulIpAdapterAddresses);


	if (currentActiveAdaptersAddressesStatus != ERROR_SUCCESS)
	{
		returnCode = currentActiveAdaptersAddressesStatus;


		ServiceLogger(_T("[ %s : %d ] GetAdaptersAddresses Failed, currentActiveAdaptersAddressesStatus Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


		return returnCode;
	}


	returnCode = (DWORD)currentActiveAdaptersAddressesStatus;


	PIP_ADAPTER_ADDRESSES pIpAdapterAddresses = ipAdapterAddresses;


	while (pIpAdapterAddresses != NULL)
	{
		if ((_tcsstr(pIpAdapterAddresses->Description, _T("Virtual")) == NULL) && (_tcsstr(pIpAdapterAddresses->Description, _T("Pseudo")) == NULL))
		{
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


				_stprintf_s(pIpv4, (size_t)(128 - 1), convertedCurrentActiveAdapterUnicastIpAddress.GetBuffer());


				convertedCurrentActiveAdapterUnicastIpAddress.ReleaseBuffer();


				break;
			}
		}


		pIpAdapterAddresses = pIpAdapterAddresses->Next;
	}


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


DWORD CreateWebSocketServer(VOID *lpVoid)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	DWORD returnCode = (DWORD)1;


	WSADATA wsaData;

	memset(&wsaData, NULL, sizeof(wsaData));


	while (TRUE)
	{
		Sleep(1000);


		INT currentActiveWSAStartupStatus = WSAStartup(MAKEWORD(2, 2), &wsaData);


		if (currentActiveWSAStartupStatus != NO_ERROR)
		{
			returnCode = (DWORD)currentActiveWSAStartupStatus;


			ServiceLogger(_T("[ %s : %d ] WSAStartup Failed, currentActiveWSAStartupStatus Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			continue;
		}


		webSocketServerSocket = WSASocket(PF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);


		if (webSocketServerSocket == INVALID_SOCKET)
		{
			returnCode = (DWORD)WSAGetLastError();


			ServiceLogger(_T("[ %s : %d ] WSASocket Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			WSACleanup();


			continue;
		}


		SOCKADDR_IN socketAddress;

		memset(&socketAddress, NULL, sizeof(socketAddress));


		socketAddress.sin_family = (ADDRESS_FAMILY)PF_INET;


		CONST USHORT port = (USHORT)9090;


		socketAddress.sin_port = (USHORT)htons(port);


		TCHAR ipv4[128];

		memset(ipv4, NULL, sizeof(ipv4));


		if (GetCurrentActiveUserAdapterInformation(ipv4) != ERROR_SUCCESS)
		{
			closesocket(webSocketServerSocket);


			webSocketServerSocket = NULL;


			WSACleanup();


			continue;
		}


		InetPton(PF_INET, ipv4, &(socketAddress.sin_addr));


		if (bind(webSocketServerSocket, (SOCKADDR *)(&socketAddress), sizeof(socketAddress)) == SOCKET_ERROR)
		{
			returnCode = (DWORD)WSAGetLastError();


			ServiceLogger(_T("[ %s : %d ] bind Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			closesocket(webSocketServerSocket);


			webSocketServerSocket = NULL;


			WSACleanup();


			continue;
		}


		if (listen(webSocketServerSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			returnCode = (DWORD)WSAGetLastError();


			ServiceLogger(_T("[ %s : %d ] listen Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			closesocket(webSocketServerSocket);


			webSocketServerSocket = NULL;


			WSACleanup();


			continue;
		}


		ServiceLogger(_T("[ %s : %d ] CreateWebSocketServer Done, Keep Waiting For Accept To Connect WebSocket Communication [ %s : %d ]"), _T(__FUNCTION__), __LINE__, ipv4, port);


		break;
	}


	while (!(closeWebSocketSessionFlag))
	{
		WEBSOCKETSESSIONMANAGER *pCurrentWebSocketConnection = (WEBSOCKETSESSIONMANAGER *)calloc((size_t)1, sizeof(WEBSOCKETSESSIONMANAGER));


		if (pCurrentWebSocketConnection == NULL)
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] calloc Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			continue;
		}


		pCurrentWebSocketConnection->currentClientAddressLength = sizeof(SOCKADDR_IN);


		pCurrentWebSocketConnection->currentClientSocket = WSAAccept(webSocketServerSocket, (sockaddr *)&pCurrentWebSocketConnection->currentClientAddress, &pCurrentWebSocketConnection->currentClientAddressLength, NULL, NULL);


		if (pCurrentWebSocketConnection->currentClientSocket == INVALID_SOCKET)
		{
			returnCode = (DWORD)WSAGetLastError();


			if (!(closeWebSocketSessionFlag))
			{
				ServiceLogger(_T("[ %s : %d ] WSAAccept Failed, WSAGetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);
			}


			free(pCurrentWebSocketConnection);


			pCurrentWebSocketConnection = NULL;


			continue;
		}


		webSocketSessionManager.push_back(pCurrentWebSocketConnection);


		HANDLE webSocketClientHandshakingThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)WebSocketClientHandshaking, webSocketSessionManager.back(), NULL, NULL);


		if (webSocketClientHandshakingThreadHandle == NULL)
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] CreateThread Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			vector<WEBSOCKETSESSIONMANAGER *>::iterator currentWebSocketSessionCursor = webSocketSessionManager.begin();


			for (size_t index = NULL; index < webSocketSessionManager.size(); index++, currentWebSocketSessionCursor++)
			{
				if (webSocketSessionManager[index] == pCurrentWebSocketConnection)
				{
					if (webSocketSessionManager[index]->currentClientSocket != NULL)
					{
						closesocket(webSocketSessionManager[index]->currentClientSocket);


						webSocketSessionManager[index]->currentClientSocket = NULL;
					}


					memset(webSocketSessionManager[index], NULL, sizeof(WEBSOCKETSESSIONMANAGER));


					free(webSocketSessionManager[index]);


					webSocketSessionManager[index] = NULL;


					break;
				}
			}


			webSocketSessionManager.erase(currentWebSocketSessionCursor);


			continue;
		}


		if (WaitForSingleObject(webSocketClientHandshakingThreadHandle, INFINITE) == WAIT_OBJECT_0)
		{
			DWORD dwWebSocketClientHandshakingThreadExitcode = NULL;


			GetExitCodeThread(webSocketClientHandshakingThreadHandle, &dwWebSocketClientHandshakingThreadExitcode);


			if (dwWebSocketClientHandshakingThreadExitcode != ERROR_SUCCESS)
			{
				CloseHandle(webSocketClientHandshakingThreadHandle);


				webSocketClientHandshakingThreadHandle = NULL;


				vector<WEBSOCKETSESSIONMANAGER *>::iterator currentWebSocketSessionCursor = webSocketSessionManager.begin();


				for (size_t index = NULL; index < webSocketSessionManager.size(); index++, currentWebSocketSessionCursor++)
				{
					if (webSocketSessionManager[index] == pCurrentWebSocketConnection)
					{
						if (webSocketSessionManager[index]->currentClientSocket != NULL)
						{
							closesocket(webSocketSessionManager[index]->currentClientSocket);


							webSocketSessionManager[index]->currentClientSocket = NULL;
						}


						memset(webSocketSessionManager[index], NULL, sizeof(WEBSOCKETSESSIONMANAGER));


						free(webSocketSessionManager[index]);


						webSocketSessionManager[index] = NULL;


						break;
					}
				}


				webSocketSessionManager.erase(currentWebSocketSessionCursor);


				continue;
			}


			if (CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)TranslateWebSocketMessage, webSocketSessionManager.back(), NULL, NULL) == NULL)
			{
				returnCode = GetLastError();


				ServiceLogger(_T("[ %s : %d ] CreateThread Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


				CloseHandle(webSocketClientHandshakingThreadHandle);


				webSocketClientHandshakingThreadHandle = NULL;


				vector<WEBSOCKETSESSIONMANAGER *>::iterator currentWebSocketSessionCursor = webSocketSessionManager.begin();


				for (size_t index = NULL; index < webSocketSessionManager.size(); index++, currentWebSocketSessionCursor++)
				{
					if (webSocketSessionManager[index] == pCurrentWebSocketConnection)
					{
						if (webSocketSessionManager[index]->currentClientSocket != NULL)
						{
							closesocket(webSocketSessionManager[index]->currentClientSocket);


							webSocketSessionManager[index]->currentClientSocket = NULL;
						}


						memset(webSocketSessionManager[index], NULL, sizeof(WEBSOCKETSESSIONMANAGER));


						free(webSocketSessionManager[index]);


						webSocketSessionManager[index] = NULL;


						break;
					}
				}


				webSocketSessionManager.erase(currentWebSocketSessionCursor);


				continue;
			}
		}
		else
		{
			returnCode = GetLastError();


			ServiceLogger(_T("[ %s : %d ] WaitForSingleObject Failed, GetLastError Code = %ld"), _T(__FUNCTION__), __LINE__, returnCode);


			vector<WEBSOCKETSESSIONMANAGER *>::iterator currentWebSocketSessionCursor = webSocketSessionManager.begin();


			for (size_t index = NULL; index < webSocketSessionManager.size(); index++, currentWebSocketSessionCursor++)
			{
				if (webSocketSessionManager[index] == pCurrentWebSocketConnection)
				{
					if (webSocketSessionManager[index]->currentClientSocket != NULL)
					{
						closesocket(webSocketSessionManager[index]->currentClientSocket);


						webSocketSessionManager[index]->currentClientSocket = NULL;
					}


					memset(webSocketSessionManager[index], NULL, sizeof(WEBSOCKETSESSIONMANAGER));


					free(webSocketSessionManager[index]);


					webSocketSessionManager[index] = NULL;


					break;
				}
			}


			webSocketSessionManager.erase(currentWebSocketSessionCursor);
		}


		CloseHandle(webSocketClientHandshakingThreadHandle);


		webSocketClientHandshakingThreadHandle = NULL;


		returnCode = ERROR_SUCCESS;
	}


	for (size_t index = NULL; index < webSocketSessionManager.size(); index++)
	{
		if (webSocketSessionManager[index]->currentClientSocket != NULL)
		{
			closesocket(webSocketSessionManager[index]->currentClientSocket);


			webSocketSessionManager[index]->currentClientSocket = NULL;
		}


		ServiceLogger(_T("[ %s : %d ] Frees The webSocketSessionManager[%ld] Memory, webSocketSessionManager[%ld] = %zd -> NULL"), _T(__FUNCTION__), __LINE__, index, index, webSocketSessionManager[index]);


		memset(webSocketSessionManager[index], NULL, sizeof(WEBSOCKETSESSIONMANAGER));


		free(webSocketSessionManager[index]);


		webSocketSessionManager[index] = NULL;
	}


	webSocketSessionManager.clear();


	ServiceLogger(_T("[ %s : %d ] Remove webSocketSession Done, Current webSocketSessionManager's Size = %ld"), _T(__FUNCTION__), __LINE__, webSocketSessionManager.size());


	WSACleanup();


	return returnCode;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
