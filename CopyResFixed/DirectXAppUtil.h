#pragma once

#pragma region include, VALIDATE, AppData, SimpleVertex, ConstantBuffer, Release(), CompileShaderFromFile()

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <cstdint>
#include <stdio.h>
#include <new>
#include <memory>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <process.h>
#include <atlcomcli.h>
#include <Shlobj.h>
#include <comutil.h>
#include <string>				// std::to_string

//#define new		new( _NORMAL_BLOCK, __FILE__, __LINE__)

#include <d3d11.h>				// #include <d3d11_1.h> (#include <wrl/client.h> для COM)
#include <d3dcompiler.h>
#if _MSC_VER > 1600				// версия больше MSVC из Visual Studio 2010
#include <DirectXMath.h>		// DirectX::XMFLOAT3, DirectX::XMMATRIX и пр.
using namespace DirectX;		// это также нужно (из DirectXMath.h)
#else
#include <xnamath.h>
#endif //_MSC_VER > 1600
#include "DDSTextureLoader.h"

// здесь подключил библиотеки (не в настройках проекта)
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTK.lib")


#ifndef VALIDATE
#define VALIDATE(x, msg, title)		if (!(x)) \
									{ \
										MessageBoxA(NULL, (msg), title, MB_ICONERROR | MB_OK); \
										exit(-1); \
									}
#endif

struct AppData
{
	HINSTANCE hInst;
	std::string sWndName;
	std::string sWndTitle;
	unsigned uAppNumber;			// просто номер данного экземпляра
	UINT width;
	UINT height;
	WNDPROC wWndProc;				// оконная функция данного экземпляра
};

/* Clean up member COM pointers */
template<typename T> void Release(T* &obj)
{
	if (!obj) return;
	obj->Release();
	obj = nullptr;
}

/* Структура вершины */
struct SimpleVertex
{
	XMFLOAT3 Pos;	 // Координаты точки в пространстве
	XMFLOAT2 Tex;	 // Координаты текстуры
	XMFLOAT3 Normal; // Нормаль вершины
};

/* Структура константного буфера (совпадает со структурой в шейдере) */
struct ConstantBuffer
{
	XMMATRIX mWorld;		// Матрица мира
	XMMATRIX mView;			// Матрица вида
	XMMATRIX mProjection;	// Матрица проекции
	XMFLOAT4 vLightDir[2];	// Направление света
	XMFLOAT4 vLightColor[2];// Цвет источника
	XMFLOAT4 vOutputColor;	// Активный цвет (для второго PSSolid)
};

/* Вспомогательная функция для компиляции шейдеров в D3DX11 */
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
							  ID3DBlob** ppBlobOut);

#define MX_SETWORLD			0x101

#pragma endregion

/* Force 16 byte alignment to avoid C4316 warning.. not gauranteeing alignment on stack.
 * DirectX needs it for XMMATRIX use */
__declspec(align(16)) class AppClass
{
public:
	AppClass();
	~AppClass();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	/* Регистрация класса и создание окна */
	HRESULT InitWindow(HINSTANCE hInst, std::string sWndName, std::string sWndTitle,
					   unsigned uAppNumber, WNDPROC wWndProc);

	/* Инициализация устройств DirectX */
	HRESULT InitDevice(UINT width, UINT height);

	/* Создание буфера вершин, шейдеров (shaders) и описания формата вершин (input layout) */
	HRESULT InitGeometry();

	/* Инициализация матриц */
	HRESULT InitMatrixes();

	/* Вычисляем направление света */
	void UpdateLight();

	/* Устанавливаем матрицы для текущего источника света (0-1) или мира (MX_SETWORLD) */
	void UpdateMatrix(UINT nLightIndex);

	/* Рендеринг кадра */
	void Render();

	/* Освобождение всех созданных объектов */
	void CleanupDevice();

	void CloseWindow();

private:
	HINSTANCE					g_hInst;
	HWND						g_hWnd;
	std::string					g_sWndName;
	std::string					g_sWndTitle;
	unsigned					g_uAppNumber;			// Просто номер данного экземпляра

	D3D_DRIVER_TYPE				g_driverType;
	D3D_FEATURE_LEVEL			g_featureLevel;
	ID3D11Device*				g_pd3dDevice;			// Устройство (для создания объектов)
	ID3D11DeviceContext*		g_pImmediateContext;	// Контекст (устройство рисования)
	IDXGISwapChain*				g_pSwapChain;			// Цепь связи (буфера с экраном)
	ID3D11RenderTargetView*		g_pRenderTargetView;	// Объект вида, задний буфер
	ID3D11Texture2D*			g_pDepthStencil;		// Текстура буфера глубин
	ID3D11DepthStencilView*		g_pDepthStencilView;	// Объект вида, буфер глубин

	ID3D11VertexShader*			g_pVertexShader;		// Вершинный шейдер
	ID3D11PixelShader*			g_pPixelShader;			// Пиксельный шейдер
	ID3D11PixelShader*			g_pPixelShaderSolid;	// Пиксельный шейдер для источников света
	ID3D11InputLayout*			g_pVertexLayout;		// Описание формата вершин

	ID3D11Buffer*				g_pVertexBuffer;		// Буфер вершин
	ID3D11Buffer*				g_pIndexBuffer;			// Буфер индексов вершин
	ID3D11Buffer*				g_pConstantBuffer;		// Константный буфер

	XMMATRIX					g_World;				// Матрица мира
	XMMATRIX					g_View;					// Матрица вида
	XMMATRIX					g_Projection;			// Матрица проекции
	FLOAT						t;						// Переменная-время

	XMFLOAT4					vLightDirs[2];			// Направление света (позиция источников)
	XMFLOAT4					vLightColors[2];		// Цвет источников

	ID3D11ShaderResourceView*	g_pTextureRV;			// Объект текстуры
	ID3D11SamplerState*			g_pSamplerLinear;		// Параметры наложения текстуры

	bool						Running;
	bool						Key[256];
	int							WinSizeW;
	int							WinSizeH;

};
