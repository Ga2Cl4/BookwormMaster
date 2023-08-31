// dllmain.cpp : 定义 DLL 应用程序的入口点。
#pragma comment(lib, "shlwapi")

#include <Windows.h>
#include <Shlwapi.h>

#define ASM_LEN 5
#define MEM_SIZE 2 * sizeof(DWORD) // 申请的共享内存空间长度

HHOOK g_hHook = NULL;
HINSTANCE g_hModule = NULL;

BYTE g_oldCode[ASM_LEN] = { 0 };
DWORD g_retAddr = NULL;
DWORD g_structBase = NULL; // 结构体基址，每局新游戏时会改变
DWORD g_dwImageBase = NULL;

BOOL g_attachFlag = FALSE;

// user define
#define TARGET_PROC_NAME  L"Bookworm.exe"
#define TARGET_WINDOW_NAME L"BookWorm Deluxe 1.13"



void parseData()
{
    HANDLE hSharedMem = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MEM_SIZE, L"sharedMem");
    LPDWORD pBuf = (LPDWORD)MapViewOfFile(hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, MEM_SIZE);
    *pBuf = g_structBase;
    UnmapViewOfFile(pBuf);
    CloseHandle(hSharedMem);
}

void __declspec(naked) GetStructBase()
{
    // 全寄存器压栈
    __asm
    {
        pushad
        pushfd
    }
    
    __asm
    {
        mov g_structBase, ecx
    }
    //TODO
    parseData();

    // 全寄存器出栈
    __asm
    {
        popfd
        popad
    }

    __asm
    {
        push ebp
        mov ebp, esp
        push -1
        jmp g_retAddr
    }
}

void AttachCode(DWORD rvaDstAddr, DWORD rvaUsrcodeAddr, DWORD dwImageBase)
{
    // 注意为了后面HOOK代码的稳定，retAddr是一个全局变量，没有作为参数
    g_retAddr = dwImageBase + rvaDstAddr + ASM_LEN; // 注意跳转的长度要与汇编代码长度匹配，否则会出问题

    BYTE code[ASM_LEN] = { 0xE9, 0x90, 0x90, 0x90, 0x90 };

    DWORD jmplen = rvaUsrcodeAddr - rvaDstAddr - 0x5;

    DWORD oldProtect = 0;
    VirtualProtect((LPVOID)(rvaDstAddr + dwImageBase), ASM_LEN, PAGE_EXECUTE_READWRITE, &oldProtect); // 一定要记得修改内存读写属性
    // 内存操作也可以使用ReadProcessMemory
    memcpy(g_oldCode, (LPVOID)(rvaDstAddr + dwImageBase), ASM_LEN);
    memcpy(&code[1], &jmplen, 0x4); // 将需要跳转的地址写入汇编指令中
    memcpy((void*)(rvaDstAddr + dwImageBase), code, ASM_LEN); // 将跳转指令写入到准备HOOK的地址

    VirtualProtect((LPVOID)(rvaDstAddr + dwImageBase), ASM_LEN, oldProtect, &oldProtect);
}

void DetachCode(DWORD rvaDstAddr, DWORD dwImageBase)
{
    DWORD oldProtect = 0;
    VirtualProtect((LPVOID)(rvaDstAddr + dwImageBase), ASM_LEN, PAGE_EXECUTE_READWRITE, &oldProtect); // 一定要记得修改内存读写属性
    memcpy((void*)(rvaDstAddr + dwImageBase), g_oldCode, ASM_LEN); // 将跳转指令写入到准备HOOK的地址
    VirtualProtect((LPVOID)(rvaDstAddr + dwImageBase), ASM_LEN, oldProtect, &oldProtect);
}

// WH_CALLWNDPROC 回调函数，看似没有操作，实际是钩取了窗口调用，此时会将dll加载到目标进程上，然后dllmain里的注入函数会执行
LRESULT CALLBACK HookMessage(int nCode, WPARAM wParam, LPARAM lParam)
{
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    DWORD rvaHookAddr = 0xDCFD0; // 准备HOOK的函数地址

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        g_hModule = hModule;
        // 只有找到目标进程时才进行代码注入
        DWORD dwpid = GetCurrentProcessId();
        WCHAR name[_MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, name, _MAX_PATH);
        PCWSTR subname = StrRChrW(name, NULL, L'\\');
        if (!lstrcmpW(subname + 1, TARGET_PROC_NAME))
        {
            //MessageBoxW(NULL, L"get it", NULL, NULL); // test code
            // 获取窗口句柄
            HWND hWnd = FindWindowW(NULL, TARGET_WINDOW_NAME);
            if (!hWnd)
            {
                MessageBoxW(NULL, L"未找到目标窗口", NULL, NULL);
                break;
            }

            g_dwImageBase = (DWORD)GetModuleHandleW(TARGET_PROC_NAME);
            // 钩取目标代码
            AttachCode(rvaHookAddr, (DWORD)GetStructBase - g_dwImageBase, g_dwImageBase);
            g_attachFlag = TRUE;
        }
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if (g_attachFlag)
        {
            DetachCode(rvaHookAddr, g_dwImageBase);
            g_attachFlag = FALSE;
        }
        break;
    }
    return TRUE;
}

// 消息钩取，钩取窗口消息
#ifdef __cplusplus
extern "C" {
#endif

    __declspec(dllexport) void HookStart(DWORD dwThreadID)
    {
        g_hHook = SetWindowsHookExW(WH_CALLWNDPROC, HookMessage, g_hModule, dwThreadID);
        if (!g_hHook)
        {
            WCHAR buf[30] = { 0 };
            wsprintfW(buf, L"set hook error : 0x%X", GetLastError());
            MessageBoxW(NULL, buf, L"Error", MB_OK);
        }
    }

    __declspec(dllexport) void HookStop()
    {
        if (g_hHook)
        {
            UnhookWindowsHookEx(g_hHook);
            g_hHook = NULL;
        }
    }

#ifdef __cplusplus
}
#endif