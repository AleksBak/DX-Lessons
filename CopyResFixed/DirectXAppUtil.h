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

#include <d3d11.h>				// #include <d3d11_1.h> (#include <wrl/client.h> ��� COM)
#include <d3dcompiler.h>
#if _MSC_VER > 1600				// ������ ������ MSVC �� Visual Studio 2010
#include <DirectXMath.h>		// DirectX::XMFLOAT3, DirectX::XMMATRIX � ��.
using namespace DirectX;		// ��� ����� ����� (�� DirectXMath.h)
#else
#include <xnamath.h>
#endif //_MSC_VER > 1600
#include "DDSTextureLoader.h"

// ����� ��������� ���������� (�� � ���������� �������)
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
	unsigned uAppNumber;			// ������ ����� ������� ����������
	UINT width;
	UINT height;
	WNDPROC wWndProc;				// ������� ������� ������� ����������
};

/* Clean up member COM pointers */
template<typename T> void Release(T* &obj)
{
	if (!obj) return;
	obj->Release();
	obj = nullptr;
}

/* ��������� ������� */
struct SimpleVertex
{
	XMFLOAT3 Pos;	 // ���������� ����� � ������������
	XMFLOAT2 Tex;	 // ���������� ��������
	XMFLOAT3 Normal; // ������� �������
};

/* ��������� ������������ ������ (��������� �� ���������� � �������) */
struct ConstantBuffer
{
	XMMATRIX mWorld;		// ������� ����
	XMMATRIX mView;			// ������� ����
	XMMATRIX mProjection;	// ������� ��������
	XMFLOAT4 vLightDir[2];	// ����������� �����
	XMFLOAT4 vLightColor[2];// ���� ���������
	XMFLOAT4 vOutputColor;	// �������� ���� (��� ������� PSSolid)
};

/* ��������������� ������� ��� ���������� �������� � D3DX11 */
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

	/* ����������� ������ � �������� ���� */
	HRESULT InitWindow(HINSTANCE hInst, std::string sWndName, std::string sWndTitle,
					   unsigned uAppNumber, WNDPROC wWndProc);

	/* ������������� ��������� DirectX */
	HRESULT InitDevice(UINT width, UINT height);

	/* �������� ������ ������, �������� (shaders) � �������� ������� ������ (input layout) */
	HRESULT InitGeometry();

	/* ������������� ������ */
	HRESULT InitMatrixes();

	/* ��������� ����������� ����� */
	void UpdateLight();

	/* ������������� ������� ��� �������� ��������� ����� (0-1) ��� ���� (MX_SETWORLD) */
	void UpdateMatrix(UINT nLightIndex);

	/* ��������� ����� */
	void Render();

	/* ������������ ���� ��������� �������� */
	void CleanupDevice();

	void CloseWindow();

private:
	HINSTANCE					g_hInst;
	HWND						g_hWnd;
	std::string					g_sWndName;
	std::string					g_sWndTitle;
	unsigned					g_uAppNumber;			// ������ ����� ������� ����������

	D3D_DRIVER_TYPE				g_driverType;
	D3D_FEATURE_LEVEL			g_featureLevel;
	ID3D11Device*				g_pd3dDevice;			// ���������� (��� �������� ��������)
	ID3D11DeviceContext*		g_pImmediateContext;	// �������� (���������� ���������)
	IDXGISwapChain*				g_pSwapChain;			// ���� ����� (������ � �������)
	ID3D11RenderTargetView*		g_pRenderTargetView;	// ������ ����, ������ �����
	ID3D11Texture2D*			g_pDepthStencil;		// �������� ������ ������
	ID3D11DepthStencilView*		g_pDepthStencilView;	// ������ ����, ����� ������

	ID3D11VertexShader*			g_pVertexShader;		// ��������� ������
	ID3D11PixelShader*			g_pPixelShader;			// ���������� ������
	ID3D11PixelShader*			g_pPixelShaderSolid;	// ���������� ������ ��� ���������� �����
	ID3D11InputLayout*			g_pVertexLayout;		// �������� ������� ������

	ID3D11Buffer*				g_pVertexBuffer;		// ����� ������
	ID3D11Buffer*				g_pIndexBuffer;			// ����� �������� ������
	ID3D11Buffer*				g_pConstantBuffer;		// ����������� �����

	XMMATRIX					g_World;				// ������� ����
	XMMATRIX					g_View;					// ������� ����
	XMMATRIX					g_Projection;			// ������� ��������
	FLOAT						t;						// ����������-�����

	XMFLOAT4					vLightDirs[2];			// ����������� ����� (������� ����������)
	XMFLOAT4					vLightColors[2];		// ���� ����������

	ID3D11ShaderResourceView*	g_pTextureRV;			// ������ ��������
	ID3D11SamplerState*			g_pSamplerLinear;		// ��������� ��������� ��������

	bool						Running;
	bool						Key[256];
	int							WinSizeW;
	int							WinSizeH;

};
