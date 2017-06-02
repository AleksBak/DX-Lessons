#include "DirectXAppUtil.h"


static CRITICAL_SECTION cs;

/* Промежуточный буфер с нашей текстурой (у нее добавляем флаг D3D11_RESOURCE_MISC_SHARED) */
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

	// Регистрация класса
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

	// теперь g_hWnd уже указывает на сам экземпляр этого класса:
	SetWindowLongPtr(g_hWnd, 0, LONG_PTR(this));

	return S_OK;

	//// Регистрация класса
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

	//// Создание окна
	//g_hInst = hInst;
	//RECT rc = { 0, 0, 400, 300 };
	//AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	//g_hWnd = CreateWindow(L"Urok6WindowClass", L"Урок 6. Наложение текстур", WS_OVERLAPPEDWINDOW,
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

	// Тут мы создаем список поддерживаемых версий DirectX
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	// Сейчас мы создадим устройства DirectX. Для начала заполним структуру,
	// которая описывает свойства переднего буфера и привязывает его к нашему окну.
	DXGI_SWAP_CHAIN_DESC sd;						// Структура, описывающая цепь связи (Swap Chain)
	ZeroMemory(&sd, sizeof(sd));							// очищаем ее
	sd.BufferCount = 1;										// у нас один буфер
	sd.BufferDesc.Width = width;							// ширина буфера
	sd.BufferDesc.Height = height;							// высота буфера
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		// формат пикселя в буфере
	sd.BufferDesc.RefreshRate.Numerator = 75;				// частота обновления экрана
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		// назначение буфера - задний буфер
	sd.OutputWindow = g_hWnd;								// привязываем к нашему окну
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;										// не полноэкранный режим

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];

		// Если устройства созданы успешно, то выходим из цикла
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

	// Теперь создаем задний буфер. Обратите внимание, в SDK
	// RenderTargetOutput - это передний буфер, а RenderTargetView - задний.
	// Извлекаем описание заднего буфера
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr)) return hr;

	// По полученному описанию создаем поверхность рисования
	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr)) return hr;

	// Переходим к созданию буфера глубин
	// Создаем текстуру-описание буфера глубин
	D3D11_TEXTURE2D_DESC descDepth;						// Структура с параметрами
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;							// ширина и
	descDepth.Height = height;							// высота текстуры
	descDepth.MipLevels = 1;							// уровень интерполяции
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	// формат (размер пикселя)
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;		// вид - буфер глубин
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	// При помощи заполненной структуры-описания создаем объект текстуры
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr)) return hr;

	// Теперь надо создать сам объект буфера глубин
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;				// Структура с параметрами
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;					// формат как в текстуре
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	// При помощи заполненной структуры-описания и текстуры создаем объект буфера глубин
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr)) return hr;

	// Подключаем объект заднего буфера и объект буфера глубин к контексту устройства
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Установки вьюпорта (масштаб и система координат). В предыдущих версиях он создавался
	// автоматически, если не был задан явно.
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

	// Компиляция вершинного шейдера из файла
	ID3DBlob* pVSBlob = NULL; // Вспомогательный объект - просто место в оперативной памяти
	hr = CompileShaderFromFile(L"urok6.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBoxA(NULL, _T("Невозможно скомпилировать файл FX. Пожалуйста, запустите данную \
					программу из папки, содержащей файл FX."), _T("Ошибка"), MB_OK);
		return hr;
	}

	// Создание вершинного шейдера
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
										  NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Определение шаблона вершин
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Создание шаблона вершин
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
										 pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr)) return hr;

	// Подключение шаблона вершин
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Компиляция пиксельного шейдера для основного большого куба из файла
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"urok6.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBoxA(NULL, _T("Невозможно скомпилировать файл FX. Пожалуйста, запустите данную \
					программу из папки, содержащей файл FX."), _T("Ошибка"), MB_OK);
		return hr;
	}

	// Создание пиксельного шейдера
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
										 NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr)) return hr;

	// Компиляция пиксельного шейдера для источников света из файла
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"urok6.fx", "PSSolid", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBoxA(NULL, _T("Невозможно скомпилировать файл FX. Пожалуйста, запустите данную \
					программу из папки, содержащей файл FX."), _T("Ошибка"), MB_OK);
		return hr;
	}

	// Создание пиксельного шейдера
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
										 NULL, &g_pPixelShaderSolid);
	pPSBlob->Release();
	if (FAILED(hr)) return hr;

	// Создание буфера вершин (по 4 точки на каждую сторону куба, всего 24 вершины)
	SimpleVertex vertices[] =
	{
		/* координаты X, Y, Z			координаты текстуры tu, tv	нормаль X, Y, Z			 */
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

	D3D11_BUFFER_DESC bd;						// Структура, описывающая создаваемый буфер
	ZeroMemory(&bd, sizeof(bd));				// очищаем ее
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 24;	// размер буфера
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	// тип буфера - буфер вершин
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;			// Структура, содержащая данные буфера
	ZeroMemory(&InitData, sizeof(InitData));	// очищаем ее
	InitData.pSysMem = vertices;				// указатель на наши 8 вершин
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr)) return hr;

	// Создание буфера индексов

	// 1) cоздание массива с данными
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

	// 2) cоздание объекта буфера
	bd.Usage = D3D11_USAGE_DEFAULT;				// Структура, описывающая создаваемый буфер
	bd.ByteWidth = sizeof(WORD) * 36;			// 36 вершин для 12 треугольников (6 сторон)
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;		// тип - буфер индексов
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;					// указатель на наш массив индексов
											
	// Вызов метода g_pd3dDevice создаст объект буфера индексов
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
	if (FAILED(hr)) return hr;

	// Установка буфера вершин
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	// Установка буфера индексов
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	// Установка способа отрисовки вершин в буфере
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Создание константного буфера
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);		// размер буфера = размеру структуры
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	// тип - константный буфер
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pConstantBuffer);
	if (FAILED(hr)) return hr;

	// Загрузка текстуры из файла
	//hr = CreateDDSTextureFromFile(g_pd3dDevice, L"seafloor.dds", nullptr, &g_pTextureRV);
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Afterburner_Eng_T.dds", nullptr, &g_pTextureRV);
	if (FAILED(hr)) return hr;

	// Создание сэмпла (описания) текстуры
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	// Тип фильтрации
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;		// Задаем координаты
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	// Создаем интерфейс сэмпла текстурирования
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr)) return hr;

	return S_OK;
}

HRESULT AppClass::InitMatrixes()
{
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;						// получаем ширину
	UINT height = rc.bottom - rc.top;						// и высоту окна

	// Инициализация матрицы мира
	g_World = XMMatrixIdentity();

	// Инициализация матрицы вида
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// Откуда смотрим
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// Куда смотрим
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// Направление верха
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	// Инициализация матрицы проекции:
	g_Projection = XMMatrixPerspectiveFovLH
	(
		XM_PIDIV4,											// ширина угла объектива
		width / (FLOAT)height,								// "квадратность" пикселя
		0.01f,												// самое ближнее видимое расстояние
		100.0f												// самое дальнее видимое расстояние
	);

	return S_OK;
}

/* Переменная времени тут рассчитывается */
void AppClass::UpdateLight()
{
	// Обновление переменной-времени (у нечетных номеров экземпляров этого класса в 4 раза быстрее):
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		// тут я добавил последний множитель - (1 + 3 * (g_uAppNumber % 2))
		t += (float)XM_PI * 0.0125f * (1 + 3 * (g_uAppNumber % 2));
	}
	// у нас скорее всего - g_driverType = D3D_DRIVER_TYPE_HARDWARE:
	else
	{
		static DWORD dwTimeStart = 0;
		DWORD dwTimeCur = GetTickCount();
		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;
		// тут я добавил последний множитель - (1 + 3 * (g_uAppNumber % 2))
		t = ((dwTimeCur - dwTimeStart) / 1000.0f) * (1 + 3 * (g_uAppNumber % 2));
	}

	// Задаем начальные координаты источников света
	vLightDirs[0] = XMFLOAT4(-0.577f, 0.577f, -0.577f, 1.0f);
	vLightDirs[1] = XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
	// Задаем цвет источников света, у нас он не будет меняться
	vLightColors[0] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vLightColors[1] = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

	// При помощи трансформаций поворачиваем второй источник света
	XMMATRIX mRotate = XMMatrixRotationY(-2.0f * t);
	XMVECTOR vLightDir = XMLoadFloat4(&vLightDirs[1]);
	vLightDir = XMVector3Transform(vLightDir, mRotate);
	XMStoreFloat4(&vLightDirs[1], vLightDir);

	// При помощи трансформаций поворачиваем первый источник света
	mRotate = XMMatrixRotationY(0.5f * t);
	vLightDir = XMLoadFloat4(&vLightDirs[0]);
	vLightDir = XMVector3Transform(vLightDir, mRotate);
	XMStoreFloat4(&vLightDirs[0], vLightDir);
}

void AppClass::UpdateMatrix(UINT nLightIndex)
{
	// Небольшая проверка индекса
	if (nLightIndex == MX_SETWORLD)
	{
		// Если рисуем центральный куб: его надо просто медленно вращать
		g_World = XMMatrixRotationAxis(XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), t);
		nLightIndex = 0;
	}
	else if (nLightIndex < 2)
	{
		// Если рисуем источники света: перемещаем матрицу в точку и уменьшаем в 5 раз
		g_World = XMMatrixTranslationFromVector(5.0f * XMLoadFloat4(&vLightDirs[nLightIndex]));
		XMMATRIX mLightScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
		g_World = mLightScale * g_World;
	}
	else
	{
		nLightIndex = 0;
	}

	// Обновление содержимого константного буфера
	ConstantBuffer cb1;	// временный контейнер
	cb1.mWorld = XMMatrixTranspose(g_World);	// загружаем в него матрицы
	cb1.mView = XMMatrixTranspose(g_View);
	cb1.mProjection = XMMatrixTranspose(g_Projection);
	cb1.vLightDir[0] = vLightDirs[0];			// загружаем данные о свете
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
	// все экземпляры, кроме номера 2, рисуем и еще у номера 1 копируем его текстуру в соотв. буфер:
	if (g_uAppNumber != 2)
	{
		// Очищаем задний буфер в синий цвет
		float ClearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

		// Очищаем буфер глубин до едицины (максимальная глубина)
		g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Установка освещения
		UpdateLight();

		// Рисуем центральный куб

		// 1) Установка матрицы центрального куба
		UpdateMatrix(MX_SETWORLD);

		// 2) Устанавливаем шейдеры и константные буферы
		g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
		g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);
		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

		// 3) Рисуем в заднем буфере 36 вершин
		g_pImmediateContext->DrawIndexed(36, 0, 0);

		// Рисуем все источники света

		// 1) Устанавливаем пиксельный шейдер
		g_pImmediateContext->PSSetShader(g_pPixelShaderSolid, NULL, 0);
		for (int m = 0; m < 2; m++)
		{
			// 2) Устанавливаем матрицу мира источника света
			UpdateMatrix(m);
			// 3) Рисуем в заднем буфере 36 вершин 
			g_pImmediateContext->DrawIndexed(36, 0, 0);
		}

		// у экземпляра с №1 ('Main') копируем во временную текстуру:
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
					// если в течении тайм-аута удалось заблокировать мьютекс, то заполняем текстуру:
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
				// если мьютекс нулевой (вначале), то значит может спокойно создавать структуру:
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

				//// если текстура нулевая (вначале), то не запрашиваем у нее мьютекс:
				//if (g_pSharedTexture)
				//{
				//	
				//}
				//else
				//{
				//	g_pSharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&g_pDXGIKeyedMutex);
				//}

				//// если в течении тайм-аута удалось заблокировать мьютекс, то заполняем текстуру:
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

			//// нужно чтобы операция копирования завершилась
			//Sleep(1);

			//LeaveCriticalSection(&cs);
		}

		// Копируем задний буфер в передний (на экран)
		g_pSwapChain->Present(0, 0);
	}
	// и только у экземпляра с №2 копируем обратно из временной текстуры:
	else
	{
		if (g_pSharedHandle && g_pDXGIKeyedMutex)
		{
			//g_pSharedTexture->QueryInterface(__uuidof(IDXGIKeyedMutex), (void**)&g_pDXGIKeyedMutex);
			if (g_pDXGIKeyedMutex->AcquireSync(relKey, timeout) == S_OK)
			{
				// получаем временный ресурс, куда ранее скопировали соотв. текстуру:
				ID3D11Resource* pSharedResource = nullptr;
				g_pd3dDevice->OpenSharedResource(g_pSharedHandle, __uuidof(ID3D11Resource),
					(void**)(&pSharedResource));

				// получаем 'ID3D11Resource' для рисования и копируем туда это полученное:
				ID3D11Resource* pSlaveResource = nullptr;
				g_pRenderTargetView->GetResource(&pSlaveResource);
				g_pImmediateContext->CopyResource(pSlaveResource, pSharedResource);
				pSlaveResource->Release();
				pSharedResource->Release();
				g_pSharedTexture->Release();
			}
			g_pDXGIKeyedMutex->ReleaseSync(acqKey);
			//EnterCriticalSection(&cs);



			//// нужно чтобы операция копирования завершилась
			//Sleep(1);

			//LeaveCriticalSection(&cs);
		}
		else
		{
			Sleep(0);		// for debug
		}

		// Копируем задний буфер в передний (на экран)
		g_pSwapChain->Present(0, 0);
	}
}

void AppClass::CleanupDevice()
{
	// Сначала отключим контекст устройства
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	// Потом освободим/удалим объекты
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

	// Промежуточный буфер с нашей текстурой тоже удаляем
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
