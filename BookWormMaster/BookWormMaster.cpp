// BookWormMaster.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "wordCalculator.h"

typedef struct {
	HANDLE hProcess;
	HANDLE hShareMem;
	HWND hWnd;
	//DWORD dwBase;
} myParam, *pMyParam;

#define MEM_SIZE 2 * sizeof(DWORD) // 申请的共享内存空间长度
// Hot keys
#define hotkeyStart 1000 // 热键 切换启动/停止
#define hotkeyExit 1001 // 热键 退出程序
// 方块相关属性
#define FIRST_TILE 0xA8 // struct + 0xA8 * n
#define BONUS_WORD 0x2C0 // struct + 0x2C0
#define TILE_CHAR 0xA4 // tile + 0xA4
#define TILE_COLOR 0xE8 // tile + 0xE8
#define MAX_STRING_SIZE 50
// 主程序运行标志
BOOL g_start = FALSE, g_exit = FALSE;
// 窗口尺寸偏移
#define X1 230
#define X2 630
#define Y1 37 // 长列
#define Y2 493
// 自动点击延迟
#define CLICK_DELAY 100
// user define
//#define TARGET_PROC_NAME  L"Bookworm.exe" // 目标进程名称
#define TARGET_WINDOW_NAME L"BookWorm Deluxe 1.13" // 目标窗口名称
#define DLL_PATH          L"bkInject.dll" // 用户注入dll文件路径

#define HOOKSTART_NAME    "HookStart"
#define HOOKSTOP_NAME     "HookStop"
typedef void (*PFN_HOOKSTART)(DWORD dwTid);
typedef void (*PFN_HOOKSTOP)();

/* ---------------- Functions ---------------- */
using namespace std;

// 求解函数
solution startSolve(HANDLE hProcess, DWORD dwBase)
{
	int i;
	//BYTE buf[10];
	DWORD pTile;
	CHAR c; // 字母
	int color; // 颜色
	char bonusBuf[MAX_STRING_SIZE] = { 0 }; // 奖励文字
	bkGraph graph;

	// 读取版面所有tile数据
	for (i = 0; i < ROWS* COLUMNS; i++)
	{
		// 空位跳过
		if (i == 0 || i == 2 || i == 4 || i == 6)
		{
			graph.tiles += "0";
			graph.colors[i] = -1;
			continue;
		}
		// 读取tile数据
		ReadProcessMemory(hProcess, (LPCVOID)(dwBase + FIRST_TILE + 4 * i), &pTile, sizeof(DWORD), NULL); // 当前tile结构体地址
		ReadProcessMemory(hProcess, (LPCVOID)(dwBase + BONUS_WORD), bonusBuf, MAX_STRING_SIZE, NULL); // 读取奖励字符串
		ReadProcessMemory(hProcess, (LPCVOID)(pTile + TILE_CHAR), &c, sizeof(CHAR), NULL);
		ReadProcessMemory(hProcess, (LPCVOID)(pTile + TILE_COLOR), &color, sizeof(int), NULL);
		graph.tiles += c;
		graph.colors[i] = color;
	}
	// 求解
	string bonus = bonusBuf;
	solution s = solveGraphAndBonus(graph, bonus);
	cout << s.word << "  bonus : " << bonus << endl;
	return s;
}

// 点击方块
void click(int coX, int coY, int delay)
{
	//return;
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	// dx,dy代表的是进行点击的坐标。下面显示的是（coX,coY）
	input.mi.dx = static_cast<long>(65535.0f / (GetSystemMetrics(SM_CXSCREEN) - 1) * coX);
	input.mi.dy = static_cast<long>(65535.0f / (GetSystemMetrics(SM_CYSCREEN) - 1) * coY);
	input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;

	// 实施鼠标点击的操作：即在坐标（coX,coY）的位置，鼠标左击一下
	SendInput(1, &input, sizeof(INPUT));
	Sleep(delay);
}

void clickTiles(list<int> path, LONG baseX, LONG baseY)
{
	int row, col;
	int coX = 0, coY = 0;
	double xoffset, yoffset;
	list<int>::iterator iter;

	xoffset = (X2 - X1) / COLUMNS;
	yoffset = (Y2 - Y1) / ROWS;
	click(baseX + 100, baseY + 30, CLICK_DELAY);
	for (iter = path.begin(); iter != path.end(); iter++)
	{
		row = (*iter) / COLUMNS;
		col = (*iter) % COLUMNS;
		coX = (int)(baseX + X1 + (col+0.5)*xoffset + 0.5);
		if (col % 2 == 0)
		{
			// 短列
			coY = (int)(baseY + Y1 + (row) * yoffset + 0.5);
		}
		else
		{
			// 长列
			coY = (int)(baseY + Y1 + (row + 0.5) * yoffset + 0.5);
		}
		click(coX, coY, CLICK_DELAY);
	}
	click(coX, coY, CLICK_DELAY);
}

// 线程函数
DWORD CALLBACK solveProc(LPVOID lParam)
{
	pMyParam param = (pMyParam)lParam;
	HANDLE hProcess = param->hProcess;
	HANDLE hSharedMem = param->hShareMem;
	HWND hWnd = param->hWnd;
	//DWORD dwBase = param->dwBase;

	MSG msg = { 0 };
	string oldMaxWord = "";
	solution s;
	RECT rect;
	LPDWORD pBuf;
	DWORD dwBase;
	// 获取HOTKEY消息，实现程序的快捷键启动、停止、退出等功能
	while (!g_exit)
	{
		// 获取当前基地址
		pBuf = (LPDWORD)MapViewOfFile(hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, MEM_SIZE);
		dwBase = *pBuf;
		UnmapViewOfFile(pBuf);
		// 进行一次求解与点击
		if (g_start)
		{
			s = startSolve(hProcess, dwBase);
			GetWindowRect(hWnd, &rect);
			if (oldMaxWord == s.word)
			{
				// 当最长词没有变化时，可能是已经过关，点击继续按钮
				click((rect.left + rect.right) / 2, rect.bottom - 60, 2*CLICK_DELAY);
			}
			clickTiles(s.path, rect.left, rect.top);
			oldMaxWord = s.word;
			Sleep(200);
			//system("pause");
		}
	}
	return 0;
}

// 主函数
int main()
{
	// 注册快捷键
	RegisterHotKey(
		NULL,
		hotkeyStart,
		MOD_ALT | MOD_NOREPEAT,
		VK_F1
	);
	RegisterHotKey(
		NULL,
		hotkeyExit,
		MOD_ALT | MOD_NOREPEAT,
		VK_F2
	);
	// 初始化共享内存
	LPDWORD pBuf;
	HANDLE hSharedMem = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MEM_SIZE, L"sharedMem");
	pBuf = (LPDWORD)MapViewOfFile(hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, MEM_SIZE);
	memset((void*)pBuf, 0x0, MEM_SIZE);
	UnmapViewOfFile(pBuf);
	
	HMODULE hDll = LoadLibraryW(DLL_PATH);
	if (!hDll)
	{
		MessageBoxW(NULL, L"Failed to load dll", NULL, NULL);
		return -1;
	}

	PFN_HOOKSTART HookStart = (PFN_HOOKSTART)GetProcAddress(hDll, HOOKSTART_NAME);
	PFN_HOOKSTOP HookStop = (PFN_HOOKSTOP)GetProcAddress(hDll, HOOKSTOP_NAME);

	// get target process id
	HWND hWnd = FindWindowW(NULL, TARGET_WINDOW_NAME);
	DWORD dwPid = 0;
	DWORD dwThreadId = GetWindowThreadProcessId(hWnd, &dwPid);
	if (!dwPid)
	{
		MessageBoxW(NULL, L"Failed to get target pid", L"Error", MB_OK);
		return -1;
	}
	HookStart(dwThreadId);

	DWORD dwBase = NULL;
	do
	{
		Sleep(500);
		pBuf = (LPDWORD)MapViewOfFile(hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, MEM_SIZE);
		dwBase = *pBuf;
		UnmapViewOfFile(pBuf);
	} while (!dwBase);
	printf("struct base at 0x%X\n", dwBase);
	printf("Start hooking process %d, thread id %d\n\n", dwPid, dwThreadId);
	printf("HELP:\nstart/stop: alt+F1\nexit: alt+F2\n\n");

	// 读入字典
	getDict();
	// 打开目标进程
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	// 启动线程 求解文字循环
	myParam param;
	param.hProcess = hProcess;
	param.hShareMem = hSharedMem;
	param.hWnd = hWnd;
	//param.dwBase = dwBase;
	HANDLE hThread = CreateThread(NULL, 0, solveProc, &param, 0, NULL);
	// 获取热键
	MSG msg = { 0 };
	while (!g_exit)
	{	
		// 获取HOTKEY消息，实现程序的快捷键启动、停止、退出等功能
		GetMessageW(&msg, NULL, 0, 0);
		if (msg.message == WM_HOTKEY)
		{
			switch (msg.wParam)
			{
			case hotkeyStart:
			{
				g_start = ~g_start; // 切换启动/停止状态
				if (g_start)
				{
					printf("start solving\n");
				}
				else
				{
					printf("paused\n");
				}
				break;
			}
			case hotkeyExit:
			{
				g_exit = TRUE;
				break;
			}
			default:
				break;
			}
		}
	}
	// 退出
	HookStop();
	CloseHandle(hSharedMem);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	UnregisterHotKey(NULL, hotkeyStart);
	UnregisterHotKey(NULL, hotkeyExit);
	return 0;
}
