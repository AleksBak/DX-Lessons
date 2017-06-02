#include "DirectXAppUtil.h"


static CRITICAL_SECTION cs;

/* ������������� ����� � ����� ��������� (� ��� ��������� ���� D3D11_RESOURCE_MISC_SHARED) */
//CComPtr<ID3D11Texture2D> g_pSharedTexture = nullptr;
ID3D11Texture2D* g_pSharedTexture = nullptr;	//static 

HANDLE g_pSharedHandle = nullptr;				//static 

IDXGIKeyedMutex* g_pDXGIKeyedMutex = nullptr;

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
							  ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
	ID3DBlob* pErrorBlob;
	hr = D3DCompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel, dwShaderFlags, 0,
							ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

AppClass::AppClass() :
	g_hInst(nullptr),
	g_hWnd(nullptr),
	g_driverType(D3D_DRIVER_TYPE_NULL),
	g_featureLevel(D3D_FEATURE_LEVEL_11_0),
	g_pd3dDevice(nullptr),
	g_pImmediateContext(nullptr),
	g_pSwapChain(nullptr),
	g_pRenderTargetView(nullptr),
	g_pDepthStencil(nullptr),
	g_pDepthStencilView(nullptr),
	g_pVertexShader(nullptr),
	g_pPixelShader(nullptr),
	g_pPixelShaderSolid(nullptr),
	g_pVertexLayout(nullptr),
	g_pVertexBuffer(nullptr),
	g_pIndexBuffer(nullptr),
	g_pConstantBuffer(nullptr),
	t(0.0f),
	g_pTextureRV(nullptr),
	g_pSamplerLinear(nullptr),
	Running(false),
	WinSizeW(0),
	WinSizeH(0)
{
	// Clear input
	for (int i = 0; i < sizeof(Key) / sizeof(Key[0]); ++i)
		Key[i] = false;
}

AppClass::~AppClass()
{
	CleanupDevice();
	CloseWindow();
}

HRESULT AppClass::InitWindow(HINSTANCE hInst, std::string sWndName, std::string sWndTitle,
							 unsigned uAppNumber, WNDPROC wWndProc)
{
	InitializeCriticalSection(&cs);

	g_hInst = hInst;
	g_sWndName = sWndName;
	g_sWndTitle = sWndTitle;
	g_uAppNumber = uAppNumber;

	Running = true;

	// ����������� ������
	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.lpszClassName = _T(g_sWndName.c_str());
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = wWndProc;
	wc.cbWndExtra = sizeof(this);
	RegisterClass(&wc);

	// adjust the window size and show at InitDevice time
	g_hWnd = CreateWindowA(wc.lpszClassName, g_sWndTitle.c_str(), WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
						   0, 0, hInst, 0);
	if (!g_hWnd) 
		return E_FAIL;

	// ������ g_hWnd ��� ��������� �� ��� ��������� ����� ������:
	SetWindowLongPtr(g_hWnd, 0, LONG_PTR(this));

	return S_OK;

	//// ����������� ������
	//WNDCLASS wcex;
	//wcex.cbSize = sizeof(WNDCLASS);
	//wcex.style = CS_HREDRAW | CS_VREDRAW;
	//wcex.lpfnWndProc = wWndProc;
	//wcex.cbClsExtra = 0;
	//wcex.cbWndExtra = 0;
	//wcex.hInstance = hInst;
	//wcex.hIcon = LoadIcon(hInst, (LPCTSTR)IDI_ICON1);
	//wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcex.lpszMenuName = NULL;
	//wcex.lpszClassName = L"Urok6WindowClass";
	//wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_ICON1);
	//if (!RegisterClassEx(&wcex))
	//	return E_FAIL;

	//// �������� ����
	//g_hInst = hInst;
	//RECT rc = { 0, 0, 400, 300 };
	//AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	//g_hWnd = CreateWindow(L"Urok6WindowClass", L"���� 6. ��������� �������", WS_OVERLAPPEDWINDOW,
	//					  CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst,
	//					  NULL);
	//if (!g_hWnd)
	//	return E_FAIL;

	//ShowWindow(g_hWnd, nCmdShow);

	//return S_OK;
}

HRESULT AppClass::InitDevice(UINT width, UINT height)
{
	HRESULT hr = S_OK;
	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	WinSizeW = width;
	WinSizeH = height;

	RECT size = { 0, 0, width, height };
	AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, false);
	const UINT flags = SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW;
	if (!SetWindowPos(g_hWnd, nullptr, 0, 0, size.right - size.left, size.bottom - size.top,
					  flags))
	{
		return E_FAIL;
	}

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	// ��� �� ������� ������ �������������� ������ DirectX
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// ������ �� �������� ���������� DirectX. ��� ������ �������� ���������,
	// ������� ��������� �������� ��������� ������ � ����������� ��� � ������ ����.
	DXGI_SWAP_CHAIN_DESC sd;						// ���������, ����������� ���� ����� (Swap Chain)
	ZeroMemory(&sd, sizeof(sd));							// ������� ��
	sd.BufferCount = 1;										// � ��� ���� �����
	sd.BufferDesc.Width = width;							// ������ ������
	sd.BufferDesc.Height = height;							// ������ ������
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		// ������ ������� � ������
	sd.BufferDesc.RefreshRate.Numerator = 75;				// ������� ���������� ������
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		// ���������� ������ - ������ �����
	sd.OutputWindow = g_hWnd;								// ����������� � ������ ����
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;										// �� ������������� �����

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];

		// ���� ���������� ������� �������, �� ������� �� �����
		if (SUCCEEDED(hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags,
														 featureLevels, numFeatureLevels,
														 D3D11_SDK_VERSION, &sd, &g_pSwapChain,
														 &g_pd3dDevice, &g_featureLevel,
														 &g_pImmediateContext)))
		{
			break;
		}
	}
	if (FAILED(hr)) return hr;

	// ������ ������� ������ �����. �������� ��������, � SDK
	// RenderTargetOutput - ��� �������� �����, � RenderTargetView - ������.
	// ��������� �������� ������� ������
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr)) return hr;

	// �� ����������� �������� ������� ����������� ���������
	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr)) return hr;

	// ��������� � �������� ������ ������
	// ������� ��������-�������� ������ ������
	D3D11_TEXTURE2D_DESC descDepth;						// ��������� � �����������
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;							// ������ �
	descDepth.Height = height;							// ������ ��������
	descDepth.MipLevels = 1;							// ������� ������������
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	// ������ (������ �������)
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;		// ��� - ����� ������
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	// ��� ������ ����������� ���������-�������� ������� ������ ��������
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr)) return hr;

	// ������ ���� ������� ��� ������ ������ ������
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;				// ��������� � �����������
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;					// ������ ��� � ��������
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	// ��� ������ ����������� ���������-�������� � �������� ������� ������ ������ ������
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr)) return hr;

	// ���������� ������ ������� ������ � ������ ������ ������ � ��������� ����������
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// ��������� �������� (������� � ������� ���������). � ���������� ������� �� ����������
	// �������������, ���� �� ��� ����� ����.
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	return S_OK;
}

HRESULT AppClass::InitGeometry()
{
	HRESULT hr = S_OK;

	// ���������� ���������� ������� �� �����
	ID3DBlob* pVSBlob = NULL; // ��������������� ������ - ������ ����� � ����������� ������
	hr = CompileShaderFromFile(L"urok6.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBoxA(NULL, _T("���������� �������������� ���� FX. ����������, ��������� ������ \
					��������� �� �����, ���������� ���� FX."), _T("������"), MB_OK);
		return hr;
	}

	// �������� ���������� �������
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
										  NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// ����������� ������� ������
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// �������� ������� ������
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
										 pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr)) return hr;

	// ����������� ������� ������
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// ���������� ����������� ������� ��� ��������� �������� ���� �� �����
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"urok6.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBoxA(NULL, _T("���������� �������������� ���� FX. ����������, ��������� ������ \
					��������� �� �����, ���������� ���� FX."), _T("������"), MB_OK);
		return hr;
	}

	// �������� ����������� �������
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
										 NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr)) return hr;

	// ���������� ����������� ������� ��� ���������� ����� �� �����
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"urok6.fx", "PSSolid", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBoxA(NULL, _T("���������� �������������� ���� FX. ����������, ��������� ������ \
					��������� �� �����, ���������� ���� FX."), _T("������"), MB_OK);
		return hr;
	}

	// �������� ����������� �������
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
										 NULL, &g_pPixelShaderSolid);
	pPSBlob->Release();
	if (FAILED(hr)) return hr;

	// �������� ������ ������ (�� 4 ����� �� ������ ������� ����, ����� 24 �������)
	SimpleVertex vertices[] =
	{
		/* ���������� X, Y, Z			���������� �������� tu, tv	������� X, Y, Z			 */
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f),	XMFLOAT2(0.0f, 0.0f),		XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f),	XMFLOAT2(1.0f, 0.0f),		XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f),	XMFLOAT2(1.0f, 1.0f),		XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f),	XMFLOAT2(0.0f, 1.0f),		XMFLOAT3(0.0f, 1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT2(0.0f, 0.0f),		XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f),	XMFLOAT2(1.0f, 0.0f),		XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f),	XMFLOAT2(1.0f, 1.0f),		XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f),	XMFLOAT2(0.0f, 1.0f),		XMFLOAT3(0.0f, -1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f),	XMFLOAT2(0.0f, 0.0f),		XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT2(1.0f, 0.0f),		XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f),	XMFLOAT2(1.0f, 1.0f),		XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f),	XMFLOAT2(0.0f, 1.0f),		XMFLOAT3(-1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(1.0f, -1.0f, 1.0f),	XMFLOAT2(0.0f, 0.0f),		XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f),	XMFLOAT2(1.0f, 0.0f),		XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f),	XMFLOAT2(1.0f, 1.0f),		XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f),	XMFLOAT2(0.0f, 1.0f),		XMFLOAT3(1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT2(0.0f, 0.0f),		XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f),	XMFLOAT2(1.0f, 0.0f),		XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f),	XMFLOAT2(1.0f, 1.0f),		XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f),	XMFLOAT2(0.0f, 1.0f),		XMFLOAT3(0.0f, 0.0f, -1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f),	XMFLOAT2(0.0f, 0.0f),		XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f),	XMFLOAT2(1.0f, 0.0f),		XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f),	XMFLOAT2(1.0f, 1.0f),		XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f),	XMFLOAT2(0.0f, 1.0f),		XMFLOAT3(0.0f, 0.0f, 1.0f) },
	};

	D3D11_BUFFER_DESC bd;						// ���������, ����������� ����������� �����
	ZeroMemory(&bd, sizeof(bd));				// ������� ��
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 24;	// ������ ������
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	// ��� ������ - ����� ������
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;			// ���������, ���������� ������ ������
	ZeroMemory(&InitData, sizeof(InitData));	// ������� ��
	InitData.pSysMem = vertices;				// ��������� �� ���� 8 ������
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr)) return hr;

	// �������� ������ ��������

	// 1) c������� ������� � �������
	WORD indices[] =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};

	// 2) c������� ������� ������
	bd.Usage = D3D11_USAGE_DEFAULT;				// ���������, ����������� ����������� �����
	bd.ByteWidth = sizeof(WORD) * 36;			// 36 ������ ��� 12 ������������� (6 ������)
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;		// ��� - ����� ��������
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;					// ��������� �� ��� ������ ��������
											
	// ����� ������ g_pd3dDevice ������� ������ ������ ��������
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
	if (FAILED(hr)) return hr;

	// ��������� ������ ������
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	// ��������� ������ ��������
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	// ��������� ������� ��������� ������ � ������
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// �������� ������������ ������
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);		// ������ ������ = ������� ���������
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	// ��� - ����������� �����
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pConstantBuffer);
	if (FAILED(hr)) return hr;

	// �������� �������� �� �����
	//hr = CreateDDSTextureFromFile(g_pd3dDevice, L"seafloor.dds", nullptr, &g_pTextureRV);
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Afterburner_Eng_T.dds", nullptr, &g_pTextureRV);
	if (FAILED(hr)) return hr;

	// �������� ������ (��������) ��������
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	// ��� ����������
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;		// ������ ����������
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	// ������� ��������� ������ ���������������
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr)) return hr;

	return S_OK;
}

HRESULT AppClass::InitMatrixes()
{
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;						// �������� ������
	UINT height = rc.bottom - rc.top;						// � ������ ����

	// ������������� ������� ����
	g_World = XMMatrixIdentity();

	// ������������� ������� ����
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// ������ �������
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// ���� �������
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// ����������� �����
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	// ������������� ������� ��������:
	g_Projection = XMMatrixPerspectiveFovLH
	(
		XM_PIDIV4,											// ������ ���� ���������
		width / (FLOAT)height,								// "������������" �������
		0.01f,												// ����� ������� ������� ����������
		100.0f												// ����� ������� ������� ����������
	);

	return S_OK;
}

/* ���������� ������� ��� �������������� */
void AppClass::UpdateLight()
{
	// ���������� ����������-������� (� �������� ������� ����������� ����� ������ � 4 ���� �������):
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		// ��� � ������� ��������� ��������� - (1 + 3 * (g_uAppNumber % 2))
		t += (float)XM_PI * 0.0125f * (1 + 3 * (g_uAppNumber % 2));
	}
	// � ��� ������ ����� - g_driverType = D3D_DRIVER_TYPE_HARDWARE:
	else
	{
		static DWORD dwTimeStart = 0;
		DWORD dwTimeCur = GetTickCount();
		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;
		// ��� � ������� ��������� ��������� - (1 + 3 * (g_uAppNumber % 2))
		t = ((dwTimeCur - dwTimeStart) / 1000.0f) * (1 + 3 * (g_uAppNumber % 2));
	}

	// ������ ��������� ���������� ���������� �����
	vLightDirs[0] = XMFLOAT4(-0.577f, 0.577f, -0.577f, 1.0f);
	vLightDirs[1] = XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
	// ������ ���� ���������� �����, � ��� �� �� ����� ��������
	vLightColors[0] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vLightColors[1] = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

	// ��� ������ ������������� ������������ ������ �������� �����
	XMMATRIX mRotate = XMMatrixRotationY(-2.0f * t);
	XMVECTOR vLightDir = XMLoadFloat4(&vLightDirs[1]);
	vLightDir = XMVector3Transform(vLightDir, mRotate);
	XMStoreFloat4(&vLightDirs[1], vLightDir);

	// ��� ������ ������������� ������������ ������ �������� �����
	mRotate = XMMatrixRotationY(0.5f * t);
	vLightDir = XMLoadFloat4(&vLightDirs[0]);
	vLightDir = XMVector3Transform(vLightDir, mRotate);
	XMStoreFloat4(&vLightDirs[0], vLightDir);
}

void AppClass::UpdateMatrix(UINT nLightIndex)
{
	// ��������� �������� �������
	if (nLightIndex == MX_SETWORLD)
	{
		// ���� ������ ����������� ���: ��� ���� ������ �������� �������
		g_World = XMMatrixRotationAxis(XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), t);
		nLightIndex = 0;
	}
	else if (nLightIndex < 2)
	{
		// ���� ������ ��������� �����: ���������� ������� � ����� � ��������� � 5 ���
		g_World = XMMatrixTranslationFromVector(5.0f * XMLoadFloat4(&vLightDirs[nLightIndex]));
		XMMATRIX mLightScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
		g_World = mLightScale * g_World;
	}
	else
	{
		nLightIndex = 0;
	}

	// ���������� ����������� ������������ ������
	ConstantBuffer cb1;	// ��������� ���������
	cb1.mWorld = XMMatrixTranspose(g_World);	// ��������� � ���� �������
	cb1.mView = XMMatrixTranspose(g_View);
	cb1.mProjection = XMMatrixTranspose(g_Projection);
	cb1.vLightDir[0] = vLightDirs[0];			// ��������� ������ � �����
	cb1.vLightDir[1] = vLightDirs[1];
	cb1.vLightColor[0] = vLightColors[0];
	cb1.vLightColor[1] = vLightColors[1];
	cb1.vOutputColor = vLightColors[nLightIndex];
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, NULL, &cb1, 0, 0);
}

UINT acqKey = 0;
UINT relKey = 1;
DWORD timeout = 30;

void AppClass::Render()
{
	// ��� ����������, ����� ������ 2, ������ � ��� � ������ 1 �������� ��� �������� � �����. �����:
	if (g_uAppNumber != 2)
	{
		// ������� ������ ����� � ����� ����
		float ClearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

		// ������� ����� ������ �� ������� (������������ �������)
		g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		// ��������� ���������
		UpdateLight();

		// ������ ����������� ���

		// 1) ��������� ������� ������������ ����
		UpdateMatrix(MX_SETWORLD);

		// 2) ������������� ������� � ����������� ������
		g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
		g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

		// 3) ������ � ������ ������ 36 ������
		g_pImmediateContext->DrawIndexed(36, 0, 0);

		// ������ ��� ��������� �����

		// 1) ������������� ���������� ������
		g_pImmediateContext->PSSetShader(g_pPixelShaderSolid, NULL, 0);
		for (int m = 0; m < 2; m++)
		{
			// 2) ������������� ������� ���� ��������� �����
			UpdateMatrix(m);
			// 3) ������ � ������ ������ 36 ������ 
			g_pImmediateContext->DrawIndexed(36, 0, 0);
		}

		// � ���������� � �1 ('Main') �������� �� ��������� ��������:
		if (g_uAppNumber == 1)
		{
			ID3D11Resource* pMainResource = nullptr;
			g_pRenderTargetView->GetResource(&pMainResource);
			if (pMainResource)
			{
				// get the temp texture and the description from him to add some flags:
				ID3D11Texture2D* pTexture = nullptr;
				pMainResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);
				D3D11_TEXTURE2D_DESC desc = { 0 };
				pTexture->GetDesc(&desc);
				desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
				pTexture->Release();

				if (g_pDXGIKeyedMutex)
				{
					// ���� � ������� ����-���� ������� ������������� �������, �� ��������� ��������:
					if (g_pDXGIKeyedMutex->AcquireSync(acqKey, timeout) == S_OK)
					{
						// also QI IDXGIKeyedMutex of synchronized shared surface's resource handle:
						g_pSharedTexture = nullptr;
						g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_pSharedTexture);

						if (g_pSharedTexture)
						{
							g_pImmediateContext->CopyResource(g_pSharedTexture, pMainResource);

							// obtain handle to IDXGIResource object:
							IDXGIResource* dxgiResource = nullptr;
							g_pSharedTexture->QueryInterface(__uuidof(IDXGIResource),
															 reinterpret_cast<void**>(&dxgiResource));
							dxgiResource->GetSharedHandle(&g_pSharedHandle);
							dxgiResource->Release();
						}
					}
					g_pDXGIKeyedMutex->ReleaseSync(relKey);
				}
				// ���� ������� ������� (�������), �� ������ ����� �������� ��������� ���������:
				else
				{
					// also QI IDXGIKeyedMutex of synchronized shared surface's resource handle:
					g_pSharedTexture = nullptr;
					g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_pSharedTexture);

					if (g_pSharedTexture)
					{
						g_pImmediateContext->CopyResource(g_pSharedTexture, pMainResource);

						g_pSharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&g_pDXGIKeyedMutex);

						// obtain handle to IDXGIResource object:
						IDXGIResource* dxgiResource = nullptr;
						g_pSharedTexture->QueryInterface(__uuidof(IDXGIResource),
														 reinterpret_cast<void**>(&dxgiResource));
						dxgiResource->GetSharedHandle(&g_pSharedHandle);
						dxgiResource->Release();

						g_pDXGIKeyedMutex->ReleaseSync(relKey);
					}
				}

				//// ���� �������� ������� (�������), �� �� ����������� � ��� �������:
				//if (g_pSharedTexture)
				//{
				//	
				//}
				//else
				//{
				//	g_pSharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&g_pDXGIKeyedMutex);
				//}

				//// ���� � ������� ����-���� ������� ������������� �������, �� ��������� ��������:
				//if (g_pDXGIKeyedMutex->AcquireSync(acqKey, timeout) == S_OK)
				//{
				//	// also QI IDXGIKeyedMutex of synchronized shared surface's resource handle:
				//	g_pSharedTexture = nullptr;
				//	g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_pSharedTexture);

				//	if (g_pSharedTexture)
				//	{
				//		g_pImmediateContext->CopyResource(g_pSharedTexture, pMainResource);

				//		// obtain handle to IDXGIResource object:
				//		IDXGIResource* dxgiResource = nullptr;
				//		g_pSharedTexture->QueryInterface(__uuidof(IDXGIResource),
				//										 reinterpret_cast<void**>(&dxgiResource));
				//		dxgiResource->GetSharedHandle(&g_pSharedHandle);
				//		dxgiResource->Release();
				//	}
				//	g_pDXGIKeyedMutex->ReleaseSync(relKey);
				//}
				pMainResource->Release();
			}




			//EnterCriticalSection(&cs);

			////if (g_pSharedHandle == nullptr || g_pDXGIKeyedMutex == nullptr || g_pDXGIKeyedMutex->AcquireSync(0, 1) == S_OK)
			//if (g_pSharedHandle == nullptr || g_pDXGIKeyedMutex->AcquireSync(acqKey, timeout) == S_OK)
			//{
			//	ID3D11Resource* pMainResource = nullptr;
			//	g_pRenderTargetView->GetResource(&pMainResource);
			//	if (pMainResource)
			//	{
			//		// get the texture and the description from him to add some flags:
			//		ID3D11Texture2D* pTexture = nullptr;
			//		pMainResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);
			//		D3D11_TEXTURE2D_DESC desc = { 0 };
			//		pTexture->GetDesc(&desc);
			//		//desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
			//		desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
			//		pTexture->Release();

			//		// also QI IDXGIKeyedMutex of synchronized shared surface's resource handle:
			//		g_pSharedTexture = nullptr;
			//		g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_pSharedTexture);
			//		//if (g_pSharedTexture && SUCCEEDED(g_pSharedTexture->QueryInterface
			//		//					(__uuidof(IDXGIKeyedMutex), (void**)&g_pDXGIKeyedMutex)))
			//		if (g_pSharedTexture)
			//		{
			//			//if (g_pDXGIKeyedMutex == nullptr)
			//			//{
			//				g_pSharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&g_pDXGIKeyedMutex);
			//			//}
			//				g_pDXGIKeyedMutex->AcquireSync(acqKey, timeout);

			//			g_pImmediateContext->CopyResource(g_pSharedTexture, pMainResource);

			//			// obtain handle to IDXGIResource object:
			//			IDXGIResource* dxgiResource = nullptr;
			//			g_pSharedTexture->QueryInterface(__uuidof(IDXGIResource),
			//											 reinterpret_cast<void**>(&dxgiResource));
			//			dxgiResource->GetSharedHandle(&g_pSharedHandle);
			//			dxgiResource->Release();
			//		}

			//		pMainResource->Release();			//
			//	}
			//	g_pDXGIKeyedMutex->ReleaseSync(relKey);
			//}

			//// ����� ����� �������� ����������� �����������
			//Sleep(1);

			//LeaveCriticalSection(&cs);
		}

		// �������� ������ ����� � �������� (�� �����)
		g_pSwapChain->Present(0, 0);
	}
	// � ������ � ���������� � �2 �������� ������� �� ��������� ��������:
	else
	{
		if (g_pSharedHandle && g_pDXGIKeyedMutex)
		{
			//g_pSharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&g_pDXGIKeyedMutex);
			if (g_pDXGIKeyedMutex->AcquireSync(relKey, timeout) == S_OK)
			{
				// �������� ��������� ������, ���� ����� ����������� �����. ��������:
				ID3D11Resource* pSharedResource = nullptr;
				g_pd3dDevice->OpenSharedResource(g_pSharedHandle, __uuidof(ID3D11Resource),
					(void**)(&pSharedResource));

				// �������� 'ID3D11Resource' ��� ��������� � �������� ���� ��� ����������:
				ID3D11Resource* pSlaveResource = nullptr;
				g_pRenderTargetView->GetResource(&pSlaveResource);
				g_pImmediateContext->CopyResource(pSlaveResource, pSharedResource);
				pSlaveResource->Release();
				pSharedResource->Release();
				g_pSharedTexture->Release();
			}
			g_pDXGIKeyedMutex->ReleaseSync(acqKey);
			//EnterCriticalSection(&cs);



			//// ����� ����� �������� ����������� �����������
			//Sleep(1);

			//LeaveCriticalSection(&cs);
		}
		else
		{
			Sleep(0);		// for debug
		}

		// �������� ������ ����� � �������� (�� �����)
		g_pSwapChain->Present(0, 0);
	}
}

void AppClass::CleanupDevice()
{
	// ������� �������� �������� ����������
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	// ����� ���������/������ �������
	Release(g_pSamplerLinear);
	Release(g_pTextureRV);
	Release(g_pConstantBuffer);
	Release(g_pVertexBuffer);
	Release(g_pIndexBuffer);
	Release(g_pVertexLayout);
	Release(g_pVertexShader);
	Release(g_pPixelShaderSolid);
	Release(g_pPixelShader);
	Release(g_pDepthStencil);
	Release(g_pDepthStencilView);
	Release(g_pRenderTargetView);
	if (g_pSwapChain)
	{
		g_pSwapChain->SetFullscreenState(FALSE, NULL);
		Release(g_pSwapChain);
	}
	Release(g_pImmediateContext);
	Release(g_pd3dDevice);

	// ������������� ����� � ����� ��������� ���� �������
	//Release(g_pSharedTexture);

	MessageBoxA(NULL, "CleanupDevice()", _T(g_sWndTitle.c_str()), MB_OK);
}

void AppClass::CloseWindow()
{
	if (g_hWnd)
	{
		::DestroyWindow(g_hWnd);
		g_hWnd = nullptr;
		UnregisterClass(_T(g_sWndName.c_str()), g_hInst);
	}
}
