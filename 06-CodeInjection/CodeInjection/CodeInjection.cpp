#include <windows.h>
#include <stdio.h>
//定义远程线程参数类型
typedef struct _THREAD_PARAM 
{
	FARPROC pFunc[2];    //长度为2的函数指针数组，本例为LoadLibraryA，GetProcAddress函数指针。
	CHAR sbBuf[2][128];  //两个长度为128的ASCII字符串，本例为"user32.dll","MessageBoxW"。
	WCHAR szBuf[2][128]; //两个长度为128的宽字符串,本例为"你好"，"请点赞唐老师编程"。
}THREAD_PARAM,*PTHREAD_PARAM;

//定义PFLOADLIBRARY为LoadLibraryA函数指针类型
typedef HMODULE(WINAPI *PFLOADLIBRARY) 
(
	LPCSTR lpLibFileName
);
//定义PFGETPROCADDRESS为GetProcAddress函数指针类型
typedef FARPROC(WINAPI *PFGETPROCADDRESS)
(
	HMODULE hModule,
	LPCSTR lpProcName
);
//定义PFMESSAGEBOX为MessageBoxW函数指针类型
typedef int(WINAPI *PFMESSAGEBOX)(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType);

DWORD WINAPI ThreadProc(LPVOID lParam) 
{
	
	PTHREAD_PARAM pParam = (PTHREAD_PARAM)lParam;
	HMODULE       hMod = NULL;
	FARPROC       pFunc = NULL;
	//pParam->pFunc[0] ->  kernel32!LoadLibraryA
	//pParam->sbBuf[0] ->  "user32.dll"
	hMod = ((PFLOADLIBRARY)pParam->pFunc[0])(pParam->sbBuf[0]);//调用LoadLibrary加载user32.dll
	//pParam->pFunc[1] ->  kernel32!GetProcAddress
	//pParam->sbBuf[1] ->  "MessageBoxW"
	pFunc = (FARPROC)((PFGETPROCADDRESS)pParam->pFunc[1])(hMod, pParam->sbBuf[1]);//获取MessageBox地址
	//pParam->szBuf[0] -> L"你好"
	//pParam->szBuf[1] -> L"请点赞唐老师编程"
	((PFMESSAGEBOX)pFunc)(NULL, pParam->szBuf[0], pParam->szBuf[1], MB_ICONWARNING);//调用MessageBox

	return 0;
}

BOOL InjectCode(DWORD dwPID) 
{

	THREAD_PARAM param = { 0, };
	//大部分进程都会加载kernel32.dll，但有些系统进程不会加载kernel32.dll，请事前确认。
	//像kernel32.dll这样的系统库，在Windows系统中，所有进程都会将其加载到相同的地址。
	//但是注意若OS版本不同，或系统重启后，即使相同模块，加载地址也会变化。
	
	HMODULE hMod = GetModuleHandle(L"kernel32.dll");
	param.pFunc[0] = GetProcAddress(hMod, "LoadLibraryA");//可以直接传递kernel32.dll两函数地址。
	param.pFunc[1] = GetProcAddress(hMod, "GetProcAddress");
	strcpy_s(param.sbBuf[0], "user32.dll");//传递需要加载dll名称以及需要调用的函数名称。
	strcpy_s(param.sbBuf[1], "MessageBoxW");
	wcscpy_s(param.szBuf[0], L"请点赞唐老师编程！！");//传递messagebox需要的参数。
	wcscpy_s(param.szBuf[1], L"你好，新年快乐");

	HANDLE hProcess = NULL;
	LPVOID pRemoteBuf[2] = { 0, };
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	if (hProcess == 0) {
		printf("Can't open process\n");
		return FALSE;
	}
	//开辟内存，并写入参数。
	DWORD dwSize = sizeof(THREAD_PARAM);
	pRemoteBuf[0] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory(hProcess, pRemoteBuf[0], (LPVOID)&param, dwSize, NULL);
	//计算shellcode大小。
	//Virtual C++ Release模式编译程序代码后，源代码中函数顺序与二进制代码中的顺序是一致的。
	//有意按照ThreadProc，InjectCode的顺序编写程序，所以生成exe中这两个函数也会以相同顺序排列。
	//又因为函数名就是函数地址，所以(DWORD64)InjectCode-(DWORD64)ThreadProc就是ThreadProc函数大小。
	dwSize = (DWORD64)InjectCode - (DWORD64)ThreadProc;
	//使用如下代码可以输出shellcode二进制形式可用于Python注入。
	for (DWORD i = 0; i < dwSize; i++) 
	{
		printf("\\x%02x", *(reinterpret_cast<unsigned char *>((DWORD64)ThreadProc) + i));
	}
	//写入shellcode。
	pRemoteBuf[1] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(hProcess, pRemoteBuf[1], (LPVOID)ThreadProc, dwSize, NULL);
	//创建远程线程并执行。
	HANDLE hThread = CreateRemoteThread(hProcess,NULL,0,(LPTHREAD_START_ROUTINE)pRemoteBuf[1],
				pRemoteBuf[0],0,NULL);

	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hProcess);
	return TRUE;
}
int main(int argc,char* argv[])
{
	//提取调试权限，这样可以注入到System进程，但需要管理权限来运行。
	HANDLE hToken;
	LUID DebugNameValue;
	TOKEN_PRIVILEGES Privileges;
	DWORD dwRet;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, L"SeDebugPrivilege", &DebugNameValue);
	Privileges.PrivilegeCount = 1;
	Privileges.Privileges[0].Luid = DebugNameValue;
	Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &Privileges, sizeof(Privileges), NULL, &dwRet);
	CloseHandle(hToken);
	//指定pid注入。
	DWORD dwPID = 0;
	if (argc != 2)
	{
		printf("\n USAGE : %s pid\n", argv[0]);
		return 1;
	}
	dwPID = (DWORD)atol(argv[1]);
	InjectCode(dwPID);
	return 0;
}

