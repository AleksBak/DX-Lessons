//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
#include <vld.h>

#include "DirectXAppUtil.h"
#include "resource.h"

#pragma region Оконные функции потоков

/* 'Main' thread WndProc. Вызывается тогда когда приложение потока получает системное сообщение */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (Msg)
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
	}

	return 0;
}

/* 'Slave' thread WndProc. Вызывается тогда когда приложение потока получает системное сообщение */
LRESULT CALLBACK Slave1WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (Msg)
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
	}

	return 0;
}

/* 'Slave' thread WndProc. Вызывается тогда когда приложение потока получает системное сообщение */
LRESULT CALLBACK Slave2WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (Msg)
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
	}

	return 0;
}

/* 'Slave' thread WndProc. Вызывается тогда когда приложение потока получает системное сообщение */
LRESULT CALLBACK Slave3WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (Msg)
	{
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
	}

	return 0;
}

WNDPROC g_mWndProcs[4] = { MainWndProc, Slave1WndProc, Slave2WndProc, Slave3WndProc };

#pragma endregion

#pragma region Главные функции потоков

unsigned __stdcall MainThread(void* pParams)
{
	AppClass* app = new AppClass();
	AppData* d = static_cast<AppData*>(pParams);

	// Создание окна приложения, объектов DirectX, шейдеров и буфера вершин, инициализация матриц
	if (FAILED(app->InitWindow(d->hInst, d->sWndName, d->sWndTitle, d->uAppNumber, d->wWndProc)) ||
		FAILED(app->InitDevice(d->width, d->height)) ||
		FAILED(app->InitGeometry()) ||
		FAILED(app->InitMatrixes()))
	{
		app->CleanupDevice();
		_endthreadex(E_FAIL);
		return 0;
	}

	// Главный цикл сообщений
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Свободное время используется для отрисовки сцены:
		else
		{
			app->Render();
			Sleep(10);
		}
	}

	// Освобождаем объекты DirectX
	app->CleanupDevice();

	_endthreadex(S_OK);
	return (int)msg.wParam;
}

unsigned __stdcall Slave1Thread(void* pParams)
{
	AppClass* app = new AppClass();
	AppData* d = static_cast<AppData*>(pParams);

	// Создание окна приложения, объектов DirectX, шейдеров и буфера вершин, инициализация матриц
	if (FAILED(app->InitWindow(d->hInst, d->sWndName, d->sWndTitle, d->uAppNumber, d->wWndProc)) ||
		FAILED(app->InitDevice(d->width, d->height)) ||
		FAILED(app->InitGeometry()) ||
		FAILED(app->InitMatrixes()))
	{
		app->CleanupDevice();
		_endthreadex(E_FAIL);
		return 0;
	}

	// Главный цикл сообщений
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Свободное время используется для отрисовки сцены:
		else
		{
			app->Render();
			Sleep(10);
		}
	}

	// Освобождаем объекты DirectX
	app->CleanupDevice();

	_endthreadex(S_OK);
	return (int)msg.wParam;
}

unsigned __stdcall Slave2Thread(void* pParams)
{
	AppClass* app = new AppClass();
	AppData* d = static_cast<AppData*>(pParams);

	// Создание окна приложения, объектов DirectX, шейдеров и буфера вершин, инициализация матриц
	if (FAILED(app->InitWindow(d->hInst, d->sWndName, d->sWndTitle, d->uAppNumber, d->wWndProc)) ||
		FAILED(app->InitDevice(d->width, d->height)) ||
		FAILED(app->InitGeometry()) ||
		FAILED(app->InitMatrixes()))
	{
		app->CleanupDevice();
		_endthreadex(E_FAIL);
		return 0;
	}

	// Главный цикл сообщений
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Свободное время используется для отрисовки сцены:
		else
		{
			app->Render();
			Sleep(10);
		}
	}

	// Освобождаем объекты DirectX
	app->CleanupDevice();

	_endthreadex(S_OK);
	return (int)msg.wParam;
}

unsigned __stdcall Slave3Thread(void* pParams)
{
	AppClass* app = new AppClass();
	AppData* d = static_cast<AppData*>(pParams);

	// Создание окна приложения, объектов DirectX, шейдеров и буфера вершин, инициализация матриц
	if (FAILED(app->InitWindow(d->hInst, d->sWndName, d->sWndTitle, d->uAppNumber, d->wWndProc)) ||
		FAILED(app->InitDevice(d->width, d->height)) ||
		FAILED(app->InitGeometry()) ||
		FAILED(app->InitMatrixes()))
	{
		app->CleanupDevice();
		_endthreadex(E_FAIL);
		return 0;
	}

	// Главный цикл сообщений
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Свободное время используется для отрисовки сцены:
		else
		{
			app->Render();
			Sleep(10);
		}
	}

	// Освобождаем объекты DirectX
	app->CleanupDevice();

	_endthreadex(S_OK);
	return (int)msg.wParam;
}

typedef unsigned(__stdcall* THREADPROC)(void* pParams);

THREADPROC g_mThrdProcs[4] = { MainThread, Slave1Thread, Slave2Thread, Slave3Thread };

#pragma endregion

/* Нужно объявить глобально эти входные данные для каждого из потоков */
static AppData g_mThreadDatas[4];

bool CreateThreads(HINSTANCE hInstance, unsigned count, HANDLE* pHandles, unsigned startNumber)
{
	if (count > sizeof(g_mWndProcs) || count > sizeof(g_mThrdProcs))
	{
		return false;
	}

	std::string sWndName, sWndTitle;

	// Points to a 32-bit variable that receives the thread identifier. If it's NULL, it's not used:
	unsigned* thrdaddr = NULL;
	unsigned uThrdID;

	for (unsigned number = startNumber; number < (count + startNumber); number++)
	{
		sWndName = "App" + std::to_string(number);
		sWndTitle = (number == startNumber) ? "MainThread (Source)" :
			("SlaveThread" + std::to_string(number) + ((number == startNumber + 1) ? " (Copy)" : ""));

		unsigned index = number - startNumber;

		g_mThreadDatas[index].hInst = hInstance;
		g_mThreadDatas[index].sWndName = sWndName;
		g_mThreadDatas[index].sWndTitle = sWndTitle;
		g_mThreadDatas[index].uAppNumber = number;
		g_mThreadDatas[index].width = 800;
		g_mThreadDatas[index].height = 450;
		g_mThreadDatas[index].wWndProc = g_mWndProcs[index];

		if (!(*pHandles = (HANDLE)_beginthreadex(NULL, 0, g_mThrdProcs[index],
			(void*)&g_mThreadDatas[index], 0, &uThrdID)))
		{
			return false;
		}
		pHandles++;
	}

	return true;
}

bool PrintThreadsExitCodes(unsigned count, HANDLE* hThrdHandles, unsigned startNumber)
{
	if (count > sizeof(g_mWndProcs) || count > sizeof(g_mThrdProcs))
	{
		return false;
	}

	// where we get threads 'Exit code':
	DWORD dCode = E_FAIL;

	for (unsigned number = startNumber; number < (count + startNumber); number++)
	{
		unsigned index = number - startNumber;

		if (GetExitCodeThread(hThrdHandles[index], static_cast<LPDWORD>(&dCode)))
		{
			std::string outStr = (dCode == S_OK) ? "Exit code: Ok" : "Exit code: FAIL";
			MessageBox(NULL, outStr.c_str(), _T(g_mThreadDatas[index].sWndTitle.c_str()), MB_OK);
		}
	}

	return true;
}

bool CloseThreads(unsigned count, HANDLE* hThrdHandles, unsigned startNumber)
{
	if (count > sizeof(g_mWndProcs) || count > sizeof(g_mThrdProcs))
	{
		return false;
	}

	for (unsigned number = startNumber; number < (count + startNumber); number++)
	{
		unsigned index = number - startNumber;
		CloseHandle(hThrdHandles[index]);
	}

	return true;
}

/* Точка входа в программу. */
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	const unsigned THREAD_COUNT = 4;		// кол-во потоков которые хотим создать (до 4-х пока что)
	const unsigned START_NUMBER = 1;		// нумерация потоков начинается с 1 (№1 это 'Main' поток)

	HANDLE hThrdHandles[THREAD_COUNT];

	CreateThreads(hInstance, THREAD_COUNT, static_cast<HANDLE *>(&hThrdHandles[0]), START_NUMBER);

	// Wait until any thread terminates:
	MsgWaitForMultipleObjects(THREAD_COUNT, hThrdHandles, false, INFINITE, 0);

	PrintThreadsExitCodes(THREAD_COUNT, hThrdHandles, START_NUMBER);

	CloseThreads(THREAD_COUNT, hThrdHandles, START_NUMBER);

	return S_OK;
}
