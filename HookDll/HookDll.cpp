// HookDll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

#include "HookDll.h"
#include "mhook.h"

#include <vector>

void sysprintA(LPCSTR format, ...)
{
	va_list ap;
	va_start(ap, format);
	int iNeed = _vscprintf(format, ap) + 1;
	char *pszOut = new char[iNeed];
	pszOut[iNeed - 1] = 0;
	vsprintf_s(pszOut, iNeed, format, ap);
	OutputDebugStringA(pszOut);
	va_end(ap);
	delete[] pszOut;
}

extern HWND g_hWnd;

void WriteLogHex(const BYTE* pbc, size_t len)
{
	std::vector<BYTE> vec;

	for (int i = 0; i < len; i++)
	{
		BYTE h = pbc[i] >> 4;
		BYTE l = pbc[i] & 0x0F;

		vec.push_back(h < 10 ? h + '0' : h + 'A' - 10);
		vec.push_back(l < 10 ? l + '0' : l + 'A' - 10);
	}

	vec.push_back('\r');
	vec.push_back('\n');
	vec.push_back('\0');

	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = vec.size();
	cpd.lpData = (PVOID)&vec[0];
	SendMessage(g_hWnd, WM_COPYDATA, 0, (LPARAM)&cpd);

}

void WriteLog(const char* str, const char* path = "C:\\Windows\\SerialData\\debug.log")
{
	if (g_hWnd == NULL)
		return;

	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = strlen(str) + 1;
	cpd.lpData = (PVOID)str;
	//memcpy(cpd.lpData, str, cpd.cbData);
	//((char*)cpd.lpData)[cpd.cbData] = '\0';
	SendMessage(g_hWnd, WM_COPYDATA, 0, (LPARAM)&cpd);

	//HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	//if (hFile == NULL)
	//	return;
	////设置文件中进行读写的当前位置
	//_llseek((HFILE)hFile, 0, SEEK_END);
	//DWORD dw;
	//WriteFile(hFile, str, strlen(str), &dw, NULL);
	//_lclose((HFILE)hFile);
}

//#define WriteLog sysprintA

typedef int (WINAPI *_MessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
typedef int (WINAPI *_MessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
typedef BOOL (WINAPI *_WriteFile)(
	__in        HANDLE hFile,
	__in_bcount_opt(nNumberOfBytesToWrite) LPCVOID lpBuffer,
	__in        DWORD nNumberOfBytesToWrite,
	__out_opt   LPDWORD lpNumberOfBytesWritten,
	__inout_opt LPOVERLAPPED lpOverlapped
);
typedef BOOL (WINAPI *_WriteFileEx)(
	__in     HANDLE hFile,
	__in_bcount_opt(nNumberOfBytesToWrite) LPCVOID lpBuffer,
	__in     DWORD nNumberOfBytesToWrite,
	__inout  LPOVERLAPPED lpOverlapped,
	__in_opt LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

_MessageBoxA TrueMessageBoxA = (_MessageBoxA)
	GetProcAddress(GetModuleHandle(L"User32"), "MessageBoxA");

_MessageBoxW TrueMessageBoxW = (_MessageBoxW)
	GetProcAddress(GetModuleHandle(L"User32"), "MessageBoxW");

_WriteFile TrueWriteFile = (_WriteFile)
	GetProcAddress(GetModuleHandle(L"kernel32"), "WriteFile");

_WriteFileEx TrueWriteFileEx = (_WriteFileEx)
	GetProcAddress(GetModuleHandle(L"kernel32"), "WriteFileEx");

extern HHOOK g_hhk;
extern HINSTANCE g_hInstance;


int WINAPI HookMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	WriteLog("***** HookMessageBoxA \r\n");
	return TrueMessageBoxA(hWnd, lpText, lpCaption, uType);
}

int WINAPI HookMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	WriteLog("***** HookMessageBoxW \r\n");
	return TrueMessageBoxW(hWnd, lpText, lpCaption, uType);
}

BOOL WINAPI HookWriteFile (
	__in        HANDLE hFile,
	__in_bcount_opt(nNumberOfBytesToWrite) LPCVOID lpBuffer,
	__in        DWORD nNumberOfBytesToWrite,
	__out_opt   LPDWORD lpNumberOfBytesWritten,
	__inout_opt LPOVERLAPPED lpOverlapped
	)
{
	char szBuf[64] = { 0 };
	snprintf(szBuf, 63, "***** HookWriteFile %d \r\n", GetCurrentProcessId());
	WriteLog(szBuf);
	if (nNumberOfBytesToWrite > 0 && nNumberOfBytesToWrite <= 16)
		WriteLogHex((BYTE*)lpBuffer, nNumberOfBytesToWrite);
	return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

BOOL WINAPI HookWriteFileEx(
	__in     HANDLE hFile,
	__in_bcount_opt(nNumberOfBytesToWrite) LPCVOID lpBuffer,
	__in     DWORD nNumberOfBytesToWrite,
	__inout  LPOVERLAPPED lpOverlapped,
	__in_opt LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
	char szBuf[64] = { 0 };
	snprintf(szBuf, 63, "***** HookWriteFileEx %d \r\n", GetCurrentProcessId());
	WriteLog(szBuf);
	if (nNumberOfBytesToWrite > 0 && nNumberOfBytesToWrite <= 16)
		WriteLogHex((BYTE*)lpBuffer, nNumberOfBytesToWrite);
	return TrueWriteFileEx(hFile, lpBuffer, nNumberOfBytesToWrite, lpOverlapped, lpCompletionRoutine);
}


LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LRESULT RetVal = CallNextHookEx(g_hhk, nCode, wParam, lParam);
	return RetVal;
}



APIHOOK_API INT HookInstall()
{
	if (g_hhk == NULL)
	{

		g_hhk = ::SetWindowsHookEx(WH_KEYBOARD, MouseProc, g_hInstance, 0);

		if (g_hhk == NULL)
		{
			char szErr[64];
			snprintf(szErr, 63, "fail to SetWindowsHookEx() %d \r\n", GetLastError());
			WriteLog(szErr);
			sysprintA("fail to SetWindowsHookEx() %d \r\n", GetLastError());
			return GetLastError();
		}

	}

	return NO_ERROR;
}

APIHOOK_API INT HookUnstall()
{
	//HookOff();

	if (g_hhk != NULL) 
	{
		if (!UnhookWindowsHookEx(g_hhk))
		{
			char szErr[64];
			snprintf(szErr, 63, "fail to UnhookWindowsHookEx() %d \r\n", GetLastError());
			WriteLog(szErr);
			sysprintA("fail to UnhookWindowsHookEx() %d \r\n", GetLastError());
			return GetLastError();
		}
		g_hhk = NULL;
	}

	return NO_ERROR;
}


static BOOL s_bHookMessageBoxA = FALSE;
static BOOL s_bHookMessageBoxW = FALSE;
static BOOL s_bHookWriteFile   = FALSE;
static BOOL s_bHookWriteFileEx = FALSE;

INT HookOn()
{
	WriteLog("-- HookOn -- \r\n");

	if (!s_bHookMessageBoxA)
		s_bHookMessageBoxA = Mhook_SetHook((PVOID*)&TrueMessageBoxA, HookMessageBoxA);
	if (!s_bHookMessageBoxW)
		s_bHookMessageBoxW = Mhook_SetHook((PVOID*)&TrueMessageBoxW, HookMessageBoxW);
	if (!s_bHookWriteFile)
		s_bHookWriteFile = Mhook_SetHook((PVOID*)&TrueWriteFile, HookWriteFile);
	if (!s_bHookWriteFileEx)
		s_bHookWriteFileEx = Mhook_SetHook((PVOID*)&TrueWriteFileEx, HookWriteFileEx);

	if (!s_bHookMessageBoxA)
	{
		WriteLog("fail to Mhook_SetHook() MessageBoxA \r\n");
	}
	if (!s_bHookMessageBoxW)
	{
		WriteLog("fail to Mhook_SetHook() MessageBoxW \r\n");
	}
	if (!s_bHookWriteFile)
	{
		WriteLog("fail to Mhook_SetHook() WriteFile \r\n");
	}
	if (!s_bHookWriteFileEx)
	{
		WriteLog("fail to Mhook_SetHook() WriteFileEx \r\n");
	}

	return NO_ERROR;
}

INT HookOff()
{
	WriteLog("-- HookOff -- \r\n");

	if (s_bHookMessageBoxA)
	{
		Mhook_Unhook((PVOID*)&TrueMessageBoxA);
		s_bHookMessageBoxA = FALSE;
	}
	if (s_bHookMessageBoxW)
	{
		Mhook_Unhook((PVOID*)&TrueMessageBoxW);
		s_bHookMessageBoxW = FALSE;
	}
	if (s_bHookWriteFile)
	{
		Mhook_Unhook((PVOID*)&TrueWriteFile);
		s_bHookWriteFile = FALSE;
	}
	if (s_bHookWriteFileEx)
	{
		Mhook_Unhook((PVOID*)&TrueWriteFileEx);
		s_bHookWriteFileEx = FALSE;
	}

	return NO_ERROR;
}