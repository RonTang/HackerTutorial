//
// created by AheadLib
// github:https://github.com/strivexjun/AheadLib-x86-x64
//
//#define _CRT_SECURE_NO_WARNINGS
//本次实验选择 notepad++ 插件HexEidtor.dll劫持,其导出表中有6个函数。
#include "pch.h"
#include <windows.h>
#include <Shlwapi.h>
#include <stdlib.h>
#pragma comment( lib, "Shlwapi.lib")
#include <taskschd.h>
#include <comdef.h>
//#include <wincred.h>
#pragma comment(lib, "taskschd.lib")
//DLL导出表与原插件一致
#pragma comment(linker, "/EXPORT:beNotified=AheadLib_beNotified,@1")
#pragma comment(linker, "/EXPORT:getFuncsArray=AheadLib_getFuncsArray,@2")
#pragma comment(linker, "/EXPORT:getName=AheadLib_getName,@3")
#pragma comment(linker, "/EXPORT:isUnicode=AheadLib_isUnicode,@4")
#pragma comment(linker, "/EXPORT:messageProc=AheadLib_messageProc,@5")
#pragma comment(linker, "/EXPORT:setInfo=AheadLib_setInfo,@6")

typedef unsigned long long QWORD;
//以下函数指针将指向被劫持DLL函数地址
extern "C"
{
	PVOID pfnAheadLib_beNotified;
	PVOID pfnAheadLib_getFuncsArray;
	PVOID pfnAheadLib_getName;
	PVOID pfnAheadLib_isUnicode;
	PVOID pfnAheadLib_messageProc;
	PVOID pfnAheadLib_setInfo;
}


static
HMODULE g_OldModule = NULL;

VOID WINAPI Free()
{
	if (g_OldModule)
	{
		FreeLibrary(g_OldModule);
	}
}


BOOL WINAPI Load()
{
	TCHAR tzPath[MAX_PATH];
	TCHAR tzTemp[MAX_PATH * 2];

	//
	// 这里是否从系统目录或当前目录加载原始DLL
	//
	GetModuleFileName(NULL,tzPath,MAX_PATH); //获取本目录下的
	PathRemoveFileSpec(tzPath);

	//GetSystemDirectory(tzPath, MAX_PATH); //默认获取系统目录的

	lstrcat(tzPath, TEXT("\\HexEditorOrg.dll"));
	g_OldModule = LoadLibrary(tzPath);
	//g_OldModule = LoadLibraryEx(tzPath,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
	if (g_OldModule == NULL)
	{
		wsprintf(tzTemp, TEXT("无法找到模块 %s,程序无法正常运行"), tzPath);
		MessageBox(NULL, tzTemp, TEXT("AheadLib"), MB_ICONSTOP);
	}

	return (g_OldModule != NULL);

}


FARPROC WINAPI GetAddress(PCSTR pszProcName)
{
	FARPROC fpAddress;
	CHAR szProcName[64];
	TCHAR tzTemp[MAX_PATH];

	fpAddress = GetProcAddress(g_OldModule, pszProcName);
	if (fpAddress == NULL)
	{
		if (HIWORD(pszProcName) == 0)
		{
			wsprintfA(szProcName, "#%d", pszProcName);
			pszProcName = szProcName;
		}

		wsprintf(tzTemp, TEXT("无法找到函数 %hs,程序无法正常运行"), pszProcName);
		MessageBox(NULL, tzTemp, TEXT("AheadLib"), MB_ICONSTOP);
		ExitProcess(-2);
	}
	return fpAddress;
}

BOOL WINAPI Init()
{
	//获取被劫持DLL函数地址，在ASM代码中使用jmp跳转
	pfnAheadLib_beNotified = GetAddress("beNotified");
	pfnAheadLib_getFuncsArray = GetAddress("getFuncsArray");
	pfnAheadLib_getName = GetAddress("getName");
	pfnAheadLib_isUnicode = GetAddress("isUnicode");
	pfnAheadLib_messageProc = GetAddress("messageProc");
	pfnAheadLib_setInfo = GetAddress("setInfo");
	return TRUE;
}

DWORD WINAPI ThreadProc(LPVOID lpThreadParameter)
{
	HANDLE hProcess;

	PVOID addr1 = reinterpret_cast<PVOID>(0x00401000);
	BYTE data1[] = { 0x90, 0x90, 0x90, 0x90 };

	//
	// 绕过VMP3.x 的内存保护
	//
	hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, GetCurrentProcessId());
	if (hProcess)
	{
		WriteProcessMemory(hProcess, addr1, data1, sizeof(data1), NULL);

		CloseHandle(hProcess);
	}
	MessageBox(NULL, L"初始化COM...", TEXT("DLL消息"), MB_OK);
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		return 1;
	}
	MessageBox(NULL, L"初始化COM安全设置...", TEXT("DLL消息"), MB_OK);
	//  Set general COM security levels.
	hr = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		0,
		NULL);

	if (FAILED(hr) && hr!= RPC_E_TOO_LATE)
	{
		CoUninitialize();
		return 1;
	}
	MessageBox(NULL, L"初始化变量", TEXT("DLL消息"), MB_OK);
	LPCWSTR wszTaskName = L"Time Trigger Test Task";

	//  Get the windows directory and set the path to notepad.exe.
	TCHAR wstrExecutablePath[MAX_PATH];
	size_t requiredSize;
	_wgetenv_s(&requiredSize, wstrExecutablePath, MAX_PATH,L"WINDIR");
	lstrcat(wstrExecutablePath, TEXT("\\SYSTEM32\\NOTEPAD.EXE"));
	MessageBox(NULL, L"连接Task Service...", TEXT("DLL消息"), MB_OK);
	//以下代码参考:
    //https://learn.microsoft.com/zh-cn/windows/win32/taskschd/time-trigger-example--c---
	//  ------------------------------------------------------
	//  Create an instance of the Task Service. 
	ITaskService *pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void**)&pService);
	if (FAILED(hr))
	{
		CoUninitialize();
		return 1;
	}
	//  Connect to the task service.
	hr = pService->Connect(_variant_t(), _variant_t(),
		_variant_t(), _variant_t());
	if (FAILED(hr))
	{
		pService->Release();
		CoUninitialize();
		return 1;
	}
	MessageBox(NULL, L"获取Task Root Folder...", TEXT("DLL消息"), MB_OK);
	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.  This folder will hold the
	//  new task that is registered.
	ITaskFolder *pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr))
	{
		pService->Release();
		CoUninitialize();
		return 1;
	}
	//  If the same task exists, remove it.
	pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);

	MessageBox(NULL, L"创建新Task...", TEXT("DLL消息"), MB_OK);
	//  Create the task definition object to create the task.
	ITaskDefinition *pTask = NULL;
	hr = pService->NewTask(0, &pTask);

	pService->Release();  // COM clean up.  Pointer is no longer used.
	if (FAILED(hr))
	{
		pRootFolder->Release();
		CoUninitialize();
		return 1;
	}
	
	MessageBox(NULL, L"设置Task标识，注册作者为Microsoft", TEXT("DLL消息"), MB_OK);
	//  ------------------------------------------------------
	//  Get the registration info for setting the identification.
	IRegistrationInfo *pRegInfo = NULL;
	hr = pTask->get_RegistrationInfo(&pRegInfo);
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	
	hr = pRegInfo->put_Author(_bstr_t(L"Microsoft"));
	pRegInfo->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	MessageBox(NULL, L"设置Task Principal...", TEXT("DLL消息"), MB_OK);
	//  ------------------------------------------------------
	//  Create the principal for the task - these credentials
	//  are overwritten with the credentials passed to RegisterTaskDefinition
	IPrincipal *pPrincipal = NULL;
	hr = pTask->get_Principal(&pPrincipal);
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	
	//  Set up principal logon type to interactive logon
	hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
	pPrincipal->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	MessageBox(NULL, L"创建Task Settings...", TEXT("DLL消息"), MB_OK);
	//  ------------------------------------------------------
	//  Create the settings for the task
	ITaskSettings *pSettings = NULL;
	hr = pTask->get_Settings(&pSettings);
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	//  Set setting values for the task.  
	hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
	pSettings->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	// Set the idle settings for the task.
	IIdleSettings *pIdleSettings = NULL;
	hr = pSettings->get_IdleSettings(&pIdleSettings);
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	
	hr = pIdleSettings->put_WaitTimeout(_bstr_t(L"PT5M"));
	pIdleSettings->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	//  ------------------------------------------------------
	//  Get the trigger collection to insert the time trigger.
	MessageBox(NULL, L"设置Task Trigger...", TEXT("DLL消息"), MB_OK);
	ITriggerCollection *pTriggerCollection = NULL;
	hr = pTask->get_Triggers(&pTriggerCollection);
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  Add the time trigger to the task.
	ITrigger *pTrigger = NULL;
	hr = pTriggerCollection->Create(TASK_TRIGGER_TIME, &pTrigger);
	pTriggerCollection->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	
	ITimeTrigger *pTimeTrigger = NULL;
	hr = pTrigger->QueryInterface(
		IID_ITimeTrigger, (void**)&pTimeTrigger);
	pTrigger->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	hr = pTimeTrigger->put_Id(_bstr_t(L"Trigger1"));
	hr = pTimeTrigger->put_EndBoundary(_bstr_t(L"2023-05-02T08:00:00"));

	//  Set the task to start at a certain time. The time 
	//  format should be YYYY-MM-DDTHH:MM:SS(+-)(timezone).
	
	hr = pTimeTrigger->put_StartBoundary(_bstr_t(L"2023-01-01T12:05:00"));
	pTimeTrigger->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	MessageBox(NULL, L"设置Task Action...", TEXT("DLL消息"), MB_OK);
	//  ------------------------------------------------------
	//  Add an action to the task. This task will execute notepad.exe.     
	IActionCollection *pActionCollection = NULL;

	//  Get the task action collection pointer.
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}

	//  Create the action, specifying that it is an executable action.
	IAction *pAction = NULL;
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	pActionCollection->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	
	IExecAction *pExecAction = NULL;
	//  QI for the executable task pointer.
	hr = pAction->QueryInterface(
		IID_IExecAction, (void**)&pExecAction);
	pAction->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	MessageBox(NULL, L"设置Action为Notepad.exe", TEXT("DLL消息"), MB_OK);
	//  Set the path of the executable to notepad.exe.
	hr = pExecAction->put_Path(_bstr_t(wstrExecutablePath));
	pExecAction->Release();
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	//  ------------------------------------------------------
	//  Save the task in the root folder.
	IRegisteredTask *pRegisteredTask = NULL;
	
	wchar_t abc[64];
	wsprintf(abc, L"%p,%p", &ITaskFolder::RegisterTaskDefinition,sizeof(&ITaskFolder::RegisterTaskDefinition));
	//MessageBox(NULL, abc, TEXT("虚函数成员指针，大小"), MB_ICONSTOP);
	
	QWORD* pVtAddr = (QWORD*)*(QWORD*)pRootFolder;
	//证明360没有Inline Hook这个虚函数，该函数指令未变。
	wsprintf(abc, L"%p", *((QWORD*)pVtAddr[17]));
	//MessageBox(NULL, abc, TEXT("函数前8个字节"), MB_ICONSTOP);
	auto address = reinterpret_cast<HRESULT(*)(ITaskFolder*,BSTR, ITaskDefinition*, LONG,
		VARIANT, VARIANT, TASK_LOGON_TYPE, VARIANT, IRegisteredTask **)>(pVtAddr[17]);
	
	hr = address(pRootFolder,
		_bstr_t(wszTaskName),
		pTask,
		TASK_CREATE_OR_UPDATE,
		_variant_t(),
		_variant_t(),
		TASK_LOGON_INTERACTIVE_TOKEN,
		_variant_t(L""),
		&pRegisteredTask);
	if (FAILED(hr))
	{
		wsprintf(abc, L"%p", hr);
		MessageBox(NULL, abc, TEXT("T.T失败了原因如下:"), MB_ICONSTOP);

		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
	MessageBox(NULL, L"\\o/成功了！不敢信啊", TEXT("DLL消息"), MB_ICONSTOP);
	//  Clean up.
	pRootFolder->Release();
	pTask->Release();
	pRegisteredTask->Release();
	CoUninitialize();
	return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, PVOID pvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		
		if (Load() && Init())
		{
			TCHAR szAppName[MAX_PATH] = TEXT("Notepad++.exe");//请修改宿主进程名
			TCHAR szCurName[MAX_PATH];

			GetModuleFileName(NULL, szCurName, MAX_PATH);
			PathStripPath(szCurName);

			//是否判断宿主进程名
			if (StrCmpI(szCurName, szAppName) == 0)
			{
				
				//启动补丁线程或者其他操作
				HANDLE hThread = CreateThread(NULL, NULL, ThreadProc, NULL, NULL, NULL);
				if (hThread)
				{
					CloseHandle(hThread);
				}
			}
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		Free();
	}

	return TRUE;
}

