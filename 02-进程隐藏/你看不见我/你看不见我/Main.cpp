// CreateRemoteThread_Test.cpp : 定义控制台应用程序的入口点。
//
#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>


void ShowError(PCWSTR pszText)
{
	WCHAR szErr[MAX_PATH] = { 0 };
	::wsprintf(szErr, L"%s Error[%d]\n", pszText, ::GetLastError());
	::MessageBox(NULL, szErr, L"ERROR", MB_OK);
}


// 使用 CreateRemoteThread 实现远线程注入
BOOL CreateRemoteThreadInjectDll(DWORD dwProcessId, PCWSTR pszDllFileName)
{
	if (dwProcessId == 0 || lstrlen(pszDllFileName) == 0)
	{
		ShowError(L"Invalid pid or dll file name\n");
		return FALSE;
	}
	HANDLE hProcess = NULL;
	SIZE_T dwSize = 0;
	LPVOID pDllAddr = NULL;
	FARPROC pFuncProcAddr = NULL;

	// 打开注入进程，获取进程句柄
	hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	if (NULL == hProcess)
	{
		ShowError(L"OpenProcess");
		return FALSE;
	}
	// 在注入进程中申请内存
	dwSize = lstrlen(pszDllFileName) * 2;
	pDllAddr = ::VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (NULL == pDllAddr)
	{
		ShowError(L"VirtualAllocEx");
		return FALSE;
	}
	// 向申请的内存中写入数据
	if (FALSE == ::WriteProcessMemory(hProcess, pDllAddr, pszDllFileName, dwSize, NULL))
	{
		ShowError(L"WriteProcessMemory");
		return FALSE;
	}
	// 获取LoadLibraryW函数地址
	pFuncProcAddr = ::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
	if (NULL == pFuncProcAddr)
	{
		ShowError(L"GetProcAddress_LoadLibraryW");
		return FALSE;
	}
	// 使用 CreateRemoteThread 创建远线程, 实现 DLL 注入
	HANDLE hRemoteThread = ::CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFuncProcAddr, pDllAddr, 0, NULL);
	if (NULL == hRemoteThread)
	{
		ShowError(L"CreateRemoteThread for LoadLibraryW");
		return FALSE;
	}
	
	WaitForSingleObject(hRemoteThread, INFINITE);
	// 关闭句柄
	::CloseHandle(hRemoteThread);
	::CloseHandle(hProcess);
	// 提示成功
	printf("远程线程注入DLL成功,点赞关注唐老师编程~\n");
	

	return TRUE;
}
BOOL CreateRemoteThreadUnInjectDll(DWORD dwProcessId, PCWSTR pszDllFileName) 
{
	if (dwProcessId == 0 || lstrlen(pszDllFileName) == 0)
	{
		ShowError(L"Invalid pid or dll file name\n");
		return FALSE;
	}
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
	MODULEENTRY32 me32;
	me32.dwSize = sizeof(me32);
	BOOL bRet = Module32First(hSnap, &me32);
	while (bRet) 
	{
		if (lstrcmp(me32.szExePath,pszDllFileName) == 0)
		{
			break;
		}
		bRet = Module32Next(hSnap, &me32);
	}
	CloseHandle(hSnap);
	HANDLE hProcess = NULL;
	FARPROC pFuncProcAddr = NULL;

	// 打开注入进程，获取进程句柄
	hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	if (NULL == hProcess)
	{
		ShowError(L"OpenProcess");
		return FALSE;
	}
	// 获取FreeLibrary函数地址
	pFuncProcAddr = ::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "FreeLibrary");
	if (NULL == pFuncProcAddr)
	{
		ShowError(L"GetProcAddress_FreeLibrary");
		return FALSE;
	}
	// 使用 CreateRemoteThread 创建远线程, 实现 DLL 卸载
	HANDLE hRemoteThread = ::CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFuncProcAddr, me32.hModule, 0, NULL);
	if (NULL == hRemoteThread)
	{
		ShowError(L"CreateRemoteThread for FreeLibrary");
		return FALSE;
	}
	WaitForSingleObject(hRemoteThread, INFINITE);
	// 关闭句柄
	::CloseHandle(hRemoteThread);
	::CloseHandle(hProcess);
	printf("远程线程卸载DLL成功,点赞关注唐老师编程~\n");
}

DWORD GetPidByName(PCWSTR name)
{
	BOOL bRet;
	PROCESSENTRY32W pe;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(pe);
	//WCHAR nameArr[MAX_PATH] = { 0 };
	//lstrcpy(nameArr, name);
	bRet = Process32FirstW(hSnapshot, &pe);
	while (bRet)
	{
		if (lstrcmp(pe.szExeFile,name)== 0)
		{
			printf("taskmgr pid:%d\n", pe.th32ProcessID);
			return pe.th32ProcessID;
		}

		bRet = Process32NextW(hSnapshot, &pe);
	}
	return 0;
}


int main(int argc, WCHAR* argv[])
{
	printf("此时你看的见我\n");
	system("pause");
	//利用远程线程将我们生成的内联钩子DLL注入到任务管理器进程
	BOOL bRet = CreateRemoteThreadInjectDll(GetPidByName(L"Taskmgr.exe"), L"E:\\Users\\Ron\\source\\repos\\UseMinHook\\x64\\Release\\UseMinHook.dll");
	printf("此时你看不见我\n");
	system("pause");
	if (bRet) 
	{
		//利用远程线程将我们的内联钩子从任务管理器中卸载
		bRet = CreateRemoteThreadUnInjectDll(GetPidByName(L"Taskmgr.exe"), L"E:\\Users\\Ron\\source\\repos\\UseMinHook\\x64\\Release\\UseMinHook.dll");
		printf("此时你又看的见我\n");
		system("pause");
	}
	
	return 0;
}

