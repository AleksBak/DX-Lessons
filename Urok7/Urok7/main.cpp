//--------------------------------------------------------------------------------------
// Урок 7. Космическая заставка со звездами, планетами и спутниками
//--------------------------------------------------------------------------------------
#include "main.h"
#include <stdio.h>

#include "SpaceStars.h"
#include "XObject.h"
#include "XPlanet.h"
#include "XWhirligig.h"
#include "XACTInterface.h"
#include "resource.h"

// Объявления глобальных переменных, объектов, интерфейсов
#include "main_declares.h"
// Предварительные объявления глобальных функций
#include "main_functions.h"

//--------------------------------------------------------------------------------------
// Точка входа в программу. Инициализация всех объектов и вход в цикл сообщений.
// Свободное время используется для отрисовки сцены.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Проверяем, не была ли запущена другая копия программы
	g_hmxProgramWorks = CreateMutex(NULL, FALSE, L"ykas_universe");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return 0;
	}

	// Загрузка настроек программы
	LoadOptions();

	/* Проверка параметров командной строки, "/p 12345678" - режим
	   предварительного просмотра в окне hWnd = 12345678           */
	WCHAR szParamName;
	UINT m_SrcHWND = 0;
	if (wcslen(lpCmdLine) > 4)
	{
		lpCmdLine[2] = L' ';
		swscanf(lpCmdLine, L"/%c %d", &szParamName, &m_SrcHWND);

		if (szParamName == L'p')
		{ // Запуск превью в окне настроек экрана
			g_bPreviewMode = TRUE;
			g_hParentWnd = (HWND)m_SrcHWND;
		}
		else if (szParamName == L'c')
		{ // Запуск настроек
			WCHAR szFileName[FILENAME_MAX];
			GetFullFileName(szFileName, L"settings.exe");
			ShellExecute(NULL, L"open", szFileName, NULL, NULL, SW_SHOWNORMAL);
			return 0;
		}
	}

	// Создание окна приложения
	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	// Создание объектов DirectX
	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	// Создание шейдеров
	if (FAILED(InitShaders()))
	{
		CleanupDevice();
		return 0;
	}

	// Создание параметров блендинга, текстур, космических объектов, аудиосистемы
	if (FAILED(InitGeometry()))
	{
		CleanupDevice();
		return 0;
	}

	// Инициализация матриц
	if (FAILED(InitMatrixes()))
	{
		CleanupDevice();
		return 0;
	}

	// Главный цикл сообщений
	BOOL bRunning = TRUE;
	MSG msg = { 0 };
	while ((WM_QUIT != msg.message) && bRunning)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			// Проверяем, не были ли нажаты/отпущены клавиши управления камерой
			if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP)
			{
				// Если клавиша нажата, надо установить флаг, отпущена - снять
				BOOL setval = (BOOL)(msg.message == WM_KEYDOWN);

				if (msg.wParam == VK_LEFT)
					g_RotateLeft = setval;
				else if (msg.wParam == VK_RIGHT)
					g_RotateRight = setval;
				else if (msg.wParam == VK_UP)
					g_MoveForward = setval;
				else if (msg.wParam == VK_DOWN)
					g_MoveBack = setval;
				else if (msg.wParam == VK_ESCAPE)
					bRunning = FALSE;
			}
		}
		else
		{
			// Рисуем сцену
			Render();
			// Проигрываем музыку
			if (g_Options.bSound && (!g_bPreviewMode)) g_XACTAudio.DoWork();
		}
	}

	// Освобождаем объекты DirectX
	CleanupDevice();

	// Сохраняем настройки в файл: отключено, т. к. теперь настройки меняются в отдельной программе
	//SaveOptions();

	// Удаляем мьютекс
	ReleaseMutex(g_hmxProgramWorks);
	CloseHandle(g_hmxProgramWorks);

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Регистрация класса и создание окна
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Регистрация класса
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_ICON1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"Urok7WindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_ICON1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Создание окна
	g_hInst = hInstance;
	RECT rc = { 0, 0, 800, 600 };
	if (g_bPreviewMode)
	{
		RECT rcParent;
		GetWindowRect(g_hParentWnd, &rcParent);
		rc.right = rcParent.right - rcParent.left;
		rc.bottom = rcParent.bottom - rcParent.top;
		g_hWnd = CreateWindow(L"Urok7WindowClass", L"Урок 7. Screensaver", WS_CHILD | WS_MAXIMIZE,
							  0, 0, rc.right - rc.left, rc.bottom - rc.top, g_hParentWnd, NULL, hInstance, NULL);
	}
	else
	{
#ifdef REGIME_FULLSCREEN
		rc.right = rc.left + GetSystemMetrics(SM_CXSCREEN);
		rc.bottom = rc.top + GetSystemMetrics(SM_CYSCREEN);
#endif
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		g_hWnd = CreateWindow(L"Urok7WindowClass", L"Урок 7. Screensaver", WS_OVERLAPPEDWINDOW,
							  CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
							  NULL);
		if (!g_hWnd) return E_FAIL;
	}

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Вызывается каждый раз, когда приложение получает системное сообщение
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_TIMER:
		g_nFPS = g_nFPSCounter;
		g_nFPSCounter = 0;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Вспомогательная функция для компиляции шейдеров в D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
							   dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
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


//--------------------------------------------------------------------------------------
// Создание устройства Direct3D (D3D Device), связующей цепи (Swap Chain) и
// контекста устройства (Immediate Context).
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;	// получаем ширину
	UINT height = rc.bottom - rc.top;	// и высоту окна

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

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
	DXGI_SWAP_CHAIN_DESC sd;			// Структура, описывающая цепь связи (Swap Chain)
	ZeroMemory(&sd, sizeof(sd));	// очищаем ее
	sd.BufferCount = 1;					// у нас один буфер
	sd.BufferDesc.Width = width;		// ширина буфера
	sd.BufferDesc.Height = height;		// высота буфера
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	// формат пикселя в буфере
	sd.BufferDesc.RefreshRate.Numerator = 0;			// частота обновления экрана
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// назначение буфера - задний буфер
	sd.OutputWindow = g_hWnd;							// привязываем к нашему окну
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
#ifdef REGIME_FULLSCREEN
	sd.Windowed = g_bPreviewMode;
#else
	sd.Windowed = TRUE;
#endif

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
										   D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))  // Если устройства созданы успешно, то выходим из цикла
			break;
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
	D3D11_TEXTURE2D_DESC descDepth;	// Структура с параметрами
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;		// ширина и
	descDepth.Height = height;		// высота текстуры
	descDepth.MipLevels = 1;		// уровень интерполяции
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
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;	// Структура с параметрами
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;		// формат как в текстуре
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

	// Инициализация генератора псевдослучайных чисел
	srand((UINT)timeGetTime());

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Создание шейдеров и шаблона вершин
//--------------------------------------------------------------------------------------
HRESULT InitShaders()
{
	HRESULT hr = S_OK;
	ID3DBlob* pVSBlob = NULL; // Вспомогательный объект - просто место в оперативной памяти
	WCHAR strFileName[FILENAME_MAX];
	GetFullFileName(strFileName, L"data\\urok7.fx");

	// Компиляция вершинного шейдера из файла
	hr = CompileShaderFromFile(strFileName, "VS", "vs_4_0", &pVSBlob);
	COMPILESHADER_ERROR(hr);
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr)) { pVSBlob->Release(); return hr; }

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

	// Объект - буфер в памяти
	ID3DBlob* pPSBlob = NULL;

	//////////////////////////////
	// Компиляция пиксельного шейдера для сферы неподвижных звезд
	hr = CompileShaderFromFile(strFileName, "PS_SkySphere", "ps_4_0", &pPSBlob);
	COMPILESHADER_ERROR(hr);
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPSSkySphere);
	pPSBlob->Release();
	if (FAILED(hr)) { return hr; }

	//////////////////////////////
	// Компиляция пиксельного шейдера для планеты
	hr = CompileShaderFromFile(strFileName, "PS_Planet", "ps_4_0", &pPSBlob);
	COMPILESHADER_ERROR(hr);
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPSPlanetSphere);
	pPSBlob->Release();
	if (FAILED(hr)) { return hr; }

	//////////////////////////////
	// Компиляция пиксельного шейдера для звезды
	hr = CompileShaderFromFile(strFileName, "PS_Star", "ps_4_0", &pPSBlob);
	COMPILESHADER_ERROR(hr);
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPSStarSphere);
	pPSBlob->Release();
	if (FAILED(hr)) { return hr; }

	pVSBlob = NULL;
	pPSBlob = NULL;

	return hr;
}

//--------------------------------------------------------------------------------------
// Создание буфера вершин, шейдеров (shaders) и описания формата вершин (input layout)
//--------------------------------------------------------------------------------------
HRESULT InitGeometry()
{
	HRESULT hr = S_OK;

	// -------------------------- Инициализация блендинга --------------------------
	D3D11_BLEND_DESC abdesc;
	abdesc.AlphaToCoverageEnable = TRUE;
	abdesc.IndependentBlendEnable = FALSE;
	abdesc.RenderTarget[0].BlendEnable = TRUE;
	abdesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR; // D3D11_BLEND_ONE;
	abdesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE; // D3D11_BLEND_DEST_COLOR;
	abdesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	abdesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	abdesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
	abdesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	abdesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = g_pd3dDevice->CreateBlendState(&abdesc, &g_pBSStar);
	hr = g_pd3dDevice->CreateBlendState(&abdesc, &g_pBSPlanetLight);
	if (FAILED(hr)) return hr;

	abdesc.AlphaToCoverageEnable = FALSE;
	abdesc.IndependentBlendEnable = FALSE;
	abdesc.RenderTarget[0].BlendEnable = FALSE;
	hr = g_pd3dDevice->CreateBlendState(&abdesc, &g_pBSDefault);
	if (FAILED(hr)) return hr;

	// ------------------------ Создание константных буферов -------------------------
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBufferMatrixes);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBMatrixes);
	if (FAILED(hr)) return hr;

	bd.ByteWidth = sizeof(ConstantBufferLight);
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBLight);
	if (FAILED(hr)) return hr;

	bd.ByteWidth = sizeof(ConstantBufferWorldMatrix);
	hr = g_pd3dDevice->CreateBuffer(&bd, NULL, &g_pCBWorldMatrix);
	if (FAILED(hr)) return hr;

	// ---------------------------- Создание текстур ---------------------------------
	hr = InitGeometry_Textures();
	if (FAILED(hr)) return hr;

	// ---------------------- Создание космических объектов --------------------------
	// Звезды
	g_pStars = new CSpaceStars(g_pImmediateContext, g_pd3dDevice, g_pCBWorldMatrix, g_Options.nStarsCountInBlock, BLOCKS_MAX, (UINT)BLOCK_SIZE);

	// Звездное небо - сфера Аристотеля
	g_pSky = new CXPlanet(g_pImmediateContext, g_pd3dDevice, &g_pPSSkySphere, NULL);
	hr = g_pSky->CreateModel(2.0f, 8, FALSE, 2);
	if (FAILED(hr)) return hr;
	g_pSky->SetScaling(XMFLOAT3(3500.0f, 3500.0f, 3500.0f));

	// Планета
	g_pPlanet = new CXPlanet(g_pImmediateContext, g_pd3dDevice, &g_pPSPlanetSphere, &g_pPSStarSphere);
	hr = g_pPlanet->CreateModel(0.9f, 32, TRUE, 1);
	if (FAILED(hr)) return hr;

	// Спутник
	g_pMoon = new CXPlanet(g_pImmediateContext, g_pd3dDevice, &g_pPSPlanetSphere, &g_pPSStarSphere);
	hr = g_pMoon->CreateModel(0.2f, 16, TRUE, 1);
	g_pMoon->m_bTextureIndex = 1;
	if (FAILED(hr)) return hr;

	g_pNukki = new CXWhirligig(g_pImmediateContext, g_pd3dDevice, &g_pPSPlanetSphere);
	hr = g_pNukki->CreateModel(2.0f, 40, TRUE, 1);
	if (FAILED(hr)) return hr;
	g_pNukki->m_bTextureIndex = 2;

	// Инициализация объекта управления положением планеты
	g_PlanetPos.Init();
	g_PlanetPos.Shift(2);
	g_pPlanet->m_bTextureIndex = g_PlanetPos.nTextureIndex[1];
	if (g_pPlanet->m_bTextureIndex == TEXIND_SUN)
	{
		g_pImmediateContext->PSSetShaderResources(2, 1, &g_pTexRVSun);
		g_pPlanet->SetScaling(XMFLOAT3(2.5f, 2.5f, 2.5f));
	}
	else
	{
		g_pImmediateContext->PSSetShaderResources(2, 1, &g_pTexRVPlanet);
		g_pPlanet->SetScaling(XMFLOAT3(1.0f, 1.0f, 1.0f));
	}
	// --------------------------------- Аудиосистема --------------------------------
	InitGeometry_Audio();

	return S_OK;
}

VOID InitGeometry_Audio()
{
	WCHAR strWaveBank[FILENAME_MAX];
	WCHAR strSoundBank[FILENAME_MAX];
	GetFullFileName(strWaveBank, L"music\\audio.xwb");
	GetFullFileName(strSoundBank, L"music\\audio.xsb");

	if (g_Options.bSound && (!g_bPreviewMode))
	{
		// Загружаем банки данных из файлов
		g_XACTAudio.LoadBank(strWaveBank, strSoundBank);
		// Запускаем циклическое проигрывание последовательности
		g_XACTAudio.PlayCue("cue_sound1", TRUE);
	}
}

HRESULT InitGeometry_Textures()
{
	HRESULT hr = S_OK;
	WCHAR strFileName[FILENAME_MAX];
	// Загрузка текстур из файл
	GetFullFileName(strFileName, L"data\\star.tex");
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, strFileName, NULL, NULL, &g_pTexRVStarMask, NULL);
	if (FAILED(hr)) return hr;
	GetFullFileName(strFileName, L"data\\sphere.tex");
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, strFileName, NULL, NULL, &g_pTexRVStars, NULL);
	if (FAILED(hr)) return hr;
	GetFullFileName(strFileName, L"data\\mars.tex");
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, strFileName, NULL, NULL, &g_pTexRVPlanet, NULL);
	if (FAILED(hr)) return hr;
	GetFullFileName(strFileName, L"data\\moon.tex");
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, strFileName, NULL, NULL, &g_pTexRVMoon, NULL);
	if (FAILED(hr)) return hr;
	GetFullFileName(strFileName, L"data\\interference.tex");
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, strFileName, NULL, NULL, &g_pTexRVHello, NULL);
	if (FAILED(hr)) return hr;
	GetFullFileName(strFileName, L"data\\sun.tex");
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, strFileName, NULL, NULL, &g_pTexRVSun, NULL);
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

	// Загружаем текстуры в шейдеры
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTexRVStars);
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_pTexRVStarMask);
	g_pImmediateContext->PSSetShaderResources(2, 1, &g_pTexRVPlanet);
	g_pImmediateContext->PSSetShaderResources(3, 1, &g_pTexRVMoon);
	g_pImmediateContext->PSSetShaderResources(4, 1, &g_pTexRVHello);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Инициализация матриц
//--------------------------------------------------------------------------------------
HRESULT InitMatrixes()
{
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;	// получаем ширину
	UINT height = rc.bottom - rc.top;	// и высоту окна

	// Инициализация матрицы мира
	g_World = XMMatrixIdentity();
	g_MoveForward = g_MoveBack = g_RotateLeft = g_RotateRight = FALSE;

	// Инициализация матрицы вида
	g_Camera.Pos = XMFLOAT3(GAME_RADIUS + GAME_ORBIT, 1.0f, 0.0f);
	g_Camera.AngleY = 0.0f;

	// Запуск таймера подсчета FPS
	SetTimer(g_hWnd, 1, 1000, NULL);

	XMVECTOR Eye = XMVectorSet(g_Camera.Pos.x, g_Camera.Pos.y, g_Camera.Pos.z, 0.0f);	// Откуда смотрим
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);	// Куда смотрим
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);	// Направление верха
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	// ОБъекту-массиву звезд необходимо знать положение камеры
	g_pStars->SetCamera(g_Camera.Pos, XMFLOAT3(0.0f, 0.0f, 0.0f), XM_PIDIV4);

	// Инициализация матрицы проекции
	// Параметры: 1) ширина угла объектива 2) "квадратность" пикселя
	// 3) самое ближнее видимое расстояние 4) самое дальнее видимое расстояние
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 10000.0f);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Вычисляем направление света
//--------------------------------------------------------------------------------------
void UpdateLight()
{
	// Задаем начальные координаты источников света
	// Задаем цвет источников света, у нас он не будет меняться
	g_vLightColors = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT3 pos = g_PlanetPos.Position[1];
	// Тут на самом деле необходимо вычислять направление света
	g_vLightPos = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);

	// Загрузка данных в константный буфер
	ConstantBufferLight cb3;
	cb3.vLightPos = g_vLightPos;
	cb3.vLightColor = g_vLightColors;
	g_pImmediateContext->UpdateSubresource(g_pCBLight, 0, NULL, &cb3, 0, 0);
}


//--------------------------------------------------------------------------------------
// Перемещение камеры по кругу с радиусом (GAME_RADIUS+GAME_ORBIT) и с замедлением при
// приближении к планете
//--------------------------------------------------------------------------------------
void MoveCamera(float coef)
{
	float fMoveAngle, fDist, planet_coef;
	static float fPrevPlanetDist = 0.0f;
	static BOOL bShiftPlanet = FALSE;
	XMFLOAT3 vNextPos;

	fMoveAngle = fGetAngle(g_Camera.Pos.x, g_Camera.Pos.z) + XM_PIDIV2;
	vNextPos.x = g_Camera.Pos.x + cosf(fMoveAngle) * g_Options.fSpeed * coef * 10;
	vNextPos.y = g_Camera.Pos.y;
	vNextPos.z = g_Camera.Pos.z + sinf(fMoveAngle) * g_Options.fSpeed * coef * 10;

	fDist = GetDistance(XMFLOAT3(0.0f, 0.0f, 0.0f), vNextPos);
	if (fDist > (GAME_RADIUS + GAME_ORBIT))
		fMoveAngle += 0.001;
	else if (fDist > (GAME_RADIUS + GAME_ORBIT))
		fMoveAngle -= 0.001;

	fDist = GetDistance(g_Camera.Pos, g_PlanetPos.Position[1]);
	if (fDist < 60.0f)
	{
		if (fDist < fPrevPlanetDist && fDist >= 20.0f)
			planet_coef = (fDist - 20.0f) / 40.0f;
		else if (fDist < fPrevPlanetDist && fDist < 20.0f)
			planet_coef = 0.03f;
		else if (fDist >= fPrevPlanetDist)
			planet_coef = sqrtf(fDist / 60.0f);

		if (planet_coef < 0.03f) planet_coef = 0.03f;
		bShiftPlanet = TRUE;
	}
	else
	{
		planet_coef = 1.0f;
		if (bShiftPlanet)
		{
			// Если прошли мимо планеты, генерируем новую
			bShiftPlanet = FALSE;
			g_PlanetPos.Shift();
			g_pPlanet->m_bTextureIndex = g_PlanetPos.nTextureIndex[1];
			if (g_pPlanet->m_bTextureIndex == TEXIND_SUN)
			{
				g_pImmediateContext->PSSetShaderResources(2, 1, &g_pTexRVSun);
				g_pPlanet->SetScaling(XMFLOAT3(2.1f, 2.1f, 2.1f));
			}
			else
			{
				g_pImmediateContext->PSSetShaderResources(2, 1, &g_pTexRVPlanet);
				g_pPlanet->SetScaling(XMFLOAT3(1.0f, 1.0f, 1.0f));
			}
		}
	}
	fPrevPlanetDist = fDist;

	g_Camera.Pos.x += cosf(fMoveAngle) * g_Options.fSpeed * coef * planet_coef * 1000.0f;
	g_Camera.Pos.z += sinf(fMoveAngle) * g_Options.fSpeed * coef * planet_coef * 1000.0f;
	g_Camera.AngleY = fMoveAngle;
	/*
	static double fMoveAngle = 0.0f;
	static float fPrevPlanetDist = 0.0f;
	static BOOL bShiftPlanet = FALSE;
	XMFLOAT3 v;
	double incval, minval;
	float dist;

	(g_pPlanet->m_bTextureIndex == TEXIND_SUN) ? minval = 0.00006 : minval = 0.0001;
	dist = GetDistance(g_Camera.Pos, g_PlanetPos.Position[1]);
	if (dist < 60.0f) {
		// Если приблизились к планете, надо замедлить скорость
		if (dist < fPrevPlanetDist && dist >= 20.0f)
			incval = coef * (minval + g_Options.fSpeed * ((dist-20.0f) / 40.0f));
		else if (dist < fPrevPlanetDist && dist < 20.0f)
			incval = coef * minval;
		else if (dist >= fPrevPlanetDist)
			incval = coef * (minval + g_Options.fSpeed * sqrtf(dist / 60.0f));

		// Флаг, что мы проходим рядом с планетой
		bShiftPlanet = TRUE;
	} else {
		incval = g_Options.fSpeed * coef;
		if (bShiftPlanet) {
			// Если прошли мимо планеты, генерируем новую
			bShiftPlanet = FALSE;
			g_PlanetPos.Shift();
			g_pPlanet->m_bTextureIndex = g_PlanetPos.nTextureIndex[1];
			if (g_pPlanet->m_bTextureIndex == TEXIND_SUN) {
				g_pImmediateContext->PSSetShaderResources( 2, 1, &g_pTexRVSun );
				g_pPlanet->SetScaling(XMFLOAT3(2.1f, 2.1f, 2.1f));
			} else {
				g_pImmediateContext->PSSetShaderResources( 2, 1, &g_pTexRVPlanet );
				g_pPlanet->SetScaling(XMFLOAT3(1.0f, 1.0f, 1.0f));
			}
		}
	}
	fPrevPlanetDist = dist;

	// Перемещаем камеру
	fMoveAngle += incval;
	v = XMFLOAT3(cos(fMoveAngle+incval) * (GAME_RADIUS+GAME_ORBIT), g_Camera.Pos.y, sin(fMoveAngle+incval) * (GAME_RADIUS+GAME_ORBIT));
	g_Camera.Pos = XMFLOAT3(cos(fMoveAngle) * (GAME_RADIUS+GAME_ORBIT), g_Camera.Pos.y, sin(fMoveAngle) * (GAME_RADIUS+GAME_ORBIT));
	g_Camera.AngleY = fGetAngle(v.x - g_Camera.Pos.x, v.z - g_Camera.Pos.z);
	*/
}

//--------------------------------------------------------------------------------------
// Устанавливаем координаты планеты, спутника, загружаем данные в объект звезд
//--------------------------------------------------------------------------------------
void MovePlanets(float coef)
{
	static float t = 0;
	const float orbit = 3.0f;
	t += 0.01f * coef;
	if (t > XM_2PI) t -= XM_2PI;

	XMFLOAT3 rt = g_pPlanet->GetRotation();
	rt.y = t;
	rt.x = XM_PI / 5;
	// Задаем положение планеты
	g_pPlanet->SetPosition(g_PlanetPos.Position[1]);
	// Задаем вращение планеты
	g_pPlanet->SetRotation(rt);
	// Задаем положение камеры
	g_pPlanet->SetCameraPos(g_Camera.Pos);


	// Задаем положение и вращение спутника, если он есть
	if (g_pPlanet->m_bTextureIndex == 0)
	{
		// Задаем положение спутника
		g_pMoon->SetPosition(XMFLOAT3(g_PlanetPos.Position[1].x + orbit*cosf(t * 3.0f),
									  g_PlanetPos.Position[1].y - 1.0f*sinf(t * 3.0f),
									  g_PlanetPos.Position[1].z + orbit*sinf(t * 3.0f)));
		// Задаем вращение спутника
		rt.y = t * 0.5f;
		rt.x = XM_PI / 7.0f;
		g_pMoon->SetRotation(rt);
		// Задаем положение камеры для спутника
		g_pMoon->SetCameraPos(g_Camera.Pos);

	}

	// Задаем положение камеры для звезд
	g_pStars->SetCamera(g_Camera.Pos, XMFLOAT3(g_Camera.Pos.x + cosf(g_Camera.AngleY),
											   g_Camera.Pos.y + sinf(g_Camera.AngleX),
											   g_Camera.Pos.z + sinf(g_Camera.AngleY)),
						XM_PIDIV4 * 4 / 3);
}

//--------------------------------------------------------------------------------------
// Устанавливаем координаты объекта g_pNukki
//--------------------------------------------------------------------------------------
void MoveNukki(float coef)
{
	static float t = 0;
	t += 0.04f * coef;

	g_fMoveNukkiAngleY += 0.005f * coef;

	g_pNukki->SetPosition(XMFLOAT3(g_Camera.Pos.x + cosf(g_fMoveNukkiAngleY)*15.0f,
								   g_Camera.Pos.y + sinf(g_fMoveNukkiAngleY)*g_fMoveNukkiAngleX * 1.0f,
								   g_Camera.Pos.z + sinf(g_fMoveNukkiAngleY)*15.0f));
	XMFLOAT3 rt = g_pPlanet->GetRotation();
	rt.x = 0.0f;
	rt.y = t;
	g_pNukki->SetRotation(rt);

	if (g_fMoveNukkiAngleY > XM_2PI)
	{
		g_fMoveNukkiAngleY = 0.0f;
		g_fMoveNukkiAngleX = 0.0f;
	}
}

//--------------------------------------------------------------------------------------
// Сброс матрицы мира и сохранение константного буфера
//--------------------------------------------------------------------------------------
void ResetWorldMatrix()
{
	g_World = XMMatrixIdentity();

	// Загрузка данных в константный буфер
	ConstantBufferWorldMatrix cb2;
	cb2.mWorld = XMMatrixTranspose(g_World);
	cb2.fObjectType = 0;
	g_pImmediateContext->UpdateSubresource(g_pCBWorldMatrix, 0, NULL, &cb2, 0, 0);
}


//--------------------------------------------------------------------------------------
// Передвигаем объекты вселенной и устанавливаем матрицы
//--------------------------------------------------------------------------------------
void UpdateMatrix()
{
	float coef;
	// вычисляем коэффициент для поддержания одинаковой
	// скорости на компьютерах с разной производительностью
	if (g_nFPS != 0)
	{
		coef = 30.0f / (float)g_nFPS;

		// Показывать FPS в заголовке главного окна, если мы не в полноэкранном режиме
		// и не в режиме предварительного просмотра в окне "Экран-Заставка"
#ifndef REGIME_FULLSCREEN
		if (!g_bPreviewMode)
		{
			wchar_t caption[32];
			swprintf(caption, L"FPS = %d", g_nFPS);
			SetWindowText(g_hWnd, caption);
		}
#endif
	}
	else { coef = 1.0f; }

	/* Упраление камерой с клавиатуры: отключено
	if (g_RotateLeft)
		g_Camera.RotateY(0.05f);
	else if (g_RotateRight)
		g_Camera.RotateY(-0.05f);
	if (g_MoveForward)
		g_Camera.Move(2.0f);
	else if (g_MoveBack)
		g_Camera.Move(-2.0f);
	*/
	g_World = XMMatrixIdentity();

	// Перемещаем камеру
	MoveCamera(coef);
	// Перемещаем планету и спутник, если он есть
	MovePlanets(coef);

	// Устаревшаяя конструкция
	if (g_fMoveNukkiAngleY != 0.0f)
	{
		// Перемещаем объект g_pNukki
		MoveNukki(coef);
	}
	else if (rand() % 10000 < 5000)
	{
		g_fMoveNukkiAngleY = 0.01f;
		g_fMoveNukkiAngleX = XM_2PI * ((float)rand() / RAND_MAX);
	}

	// Устанавливем матрицу вида, используя параметры камеры
	XMVECTOR Eye = XMVectorSet(g_Camera.Pos.x, g_Camera.Pos.y, g_Camera.Pos.z, 0.0f);	// Откуда смотрим
	XMVECTOR At = XMVectorSet(g_Camera.Pos.x + cosf(g_Camera.AngleY),
							  g_Camera.Pos.y + sinf(g_Camera.AngleX),
							  g_Camera.Pos.z + sinf(g_Camera.AngleY), 0.0f);	// Куда смотрим
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);	// Направление верха
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	// Обновление содержимого константного буфера
	ConstantBufferMatrixes cb1;
	cb1.mView = XMMatrixTranspose(g_View);
	cb1.mProjection = XMMatrixTranspose(g_Projection);
	g_pImmediateContext->UpdateSubresource(g_pCBMatrixes, 0, NULL, &cb1, 0, 0);
}


//--------------------------------------------------------------------------------------
// Рендеринг кадра
//--------------------------------------------------------------------------------------
void Render()
{
	// Очищаем задний буфер в синий цвет
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	// Очищаем буфер глубин до едицины (максимальная глубина)
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	UpdateLight();	// Устанвока освещения

	// Устанавливаем шейдеры и константные буферы
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBMatrixes);
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCBWorldMatrix);
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBWorldMatrix);
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCBLight);

	// Перемещаем объекты и камеру, устанавливаем матрицы вида и проекции
	UpdateMatrix();

	// Рисуем сферу неподвижных звезд
	g_pSky->Render();

	// Рисуем массив звезд
	ResetWorldMatrix();
	g_pImmediateContext->PSSetShader(g_pPSStarSphere, NULL, 0);
	SetBlendEnable(TRUE);
	g_pStars->Render();
	SetBlendEnable(FALSE);

	// В порядке уменьшения расстояния до камеры рисуем космические объекты
	float dist1, dist2;
	dist1 = GetDistance(g_pPlanet->GetPosition(), g_Camera.Pos);
	dist2 = GetDistance(g_pNukki->GetPosition(), g_Camera.Pos);
	if (dist1 > dist2)
	{ /* если планета находится дальше от камеры, ее нарисовать первой */
		RenderPlanet();
		g_pNukki->Render();
	}
	else
	{
		g_pNukki->Render();
		RenderPlanet();
	}

	// Копируем задний буфер в передний (на экран)
	g_pSwapChain->Present(0, 0);
	// Инкремент счетчика отрисованных кадров
	g_nFPSCounter++;
}

//--------------------------------------------------------------------------------------
// Рендеринг планеты и спутника
//--------------------------------------------------------------------------------------
inline void RenderPlanet()
{
	// В порядке уменьшения расстояния до камеры рисуем космические объекты
	if (g_pPlanet->m_bTextureIndex == 0)
	{
		float dist1, dist2; /* Если есть спутник */
		dist1 = GetDistance(g_pPlanet->GetPosition(), g_Camera.Pos);
		dist2 = GetDistance(g_pMoon->GetPosition(), g_Camera.Pos);

		if (dist1 > dist2)
		{ /* если планета находится дальше от камеры, ее нарисовать первой */
			g_pPlanet->Render();
			g_pMoon->Render();
		}
		else
		{
			g_pMoon->Render();
			g_pPlanet->Render();
		}
	}
	else
	{ /* Если нет спутника */
		g_pPlanet->Render();
	}
}

//--------------------------------------------------------------------------------------
// Включение и отключение прозрачности
//--------------------------------------------------------------------------------------
void SetBlendEnable(BOOL bEnable)
{
	float blendfactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	if (bEnable)
		g_pImmediateContext->OMSetBlendState(g_pBSStar, blendfactor, 0xffffffff);
	else
		g_pImmediateContext->OMSetBlendState(g_pBSDefault, blendfactor, 0xffffffff);
}

//--------------------------------------------------------------------------------------
// Загрузка настроек из файла
//--------------------------------------------------------------------------------------
void LoadOptions()
{
	HANDLE hFile;
	DWORD dwFileSize;
	DWORD dwBytesRead;

	hFile = CreateFile(L"Ykas Universe.dat", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize != -1 && dwFileSize >= sizeof(SCREENSAVEROPTIONS))
		{
			ReadFile(hFile, (void*)&g_Options, sizeof(SCREENSAVEROPTIONS), &dwBytesRead, NULL);
		}
		CloseHandle(hFile);
	}
	else
	{
		// Если файла с настройками не найдено, необходимо задать значения по умолчанию
		g_Options.nStarsCountInBlock = 10;
		g_Options.bSound = TRUE;
		g_Options.fSpeed = 0.003f;
		wcscpy(g_Options.strMediaPath, L"");
		//wcscpy(g_Options.strMediaPath, L"c:\\1\\");
	}
}

//--------------------------------------------------------------------------------------
// Сохранение настроек в файл
//--------------------------------------------------------------------------------------
void SaveOptions()
{
	HANDLE hFile;
	DWORD dwBytesWritten;

	hFile = CreateFile(L"Ykas Universe.dat", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, (void*)&g_Options, sizeof(SCREENSAVEROPTIONS), &dwBytesWritten, NULL);
		CloseHandle(hFile);
	}
}

//--------------------------------------------------------------------------------------
// Получение полного имени файла с данными программы
//--------------------------------------------------------------------------------------
void GetFullFileName(LPWSTR strFullFileName, LPWSTR strFile)
{
	wcscpy_s(strFullFileName, FILENAME_MAX, g_Options.strMediaPath);
	wcscat_s(strFullFileName, FILENAME_MAX, strFile);
}

//--------------------------------------------------------------------------------------
// Освобождение всех созданных объектов
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	// Сначала отключим контекст устройства
	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	// Потом удалим объекты
	delete g_pStars;

	if (g_Options.bSound) g_XACTAudio.Stop();

	if (g_pSky != NULL) { delete g_pSky; g_pSky = NULL; }
	if (g_pPlanet != NULL) { delete g_pPlanet; g_pPlanet = NULL; }
	if (g_pMoon != NULL) { delete g_pMoon; g_pMoon = NULL; }
	if (g_pNukki != NULL) { delete g_pNukki; g_pNukki = NULL; }

	SAFE_RELEASE(g_pBSDefault);
	SAFE_RELEASE(g_pBSStar);
	SAFE_RELEASE(g_pBSPlanetLight);
	SAFE_RELEASE(g_pSamplerLinear);
	SAFE_RELEASE(g_pTexRVStars);
	SAFE_RELEASE(g_pTexRVSun);
	SAFE_RELEASE(g_pTexRVPlanet);
	SAFE_RELEASE(g_pTexRVStarMask);
	SAFE_RELEASE(g_pTexRVHello);
	SAFE_RELEASE(g_pTexRVMoon);
	SAFE_RELEASE(g_pCBMatrixes);
	SAFE_RELEASE(g_pCBWorldMatrix);
	SAFE_RELEASE(g_pCBLight);
	SAFE_RELEASE(g_pVBSkyShpere);
	SAFE_RELEASE(g_pIBSkyShpere);
	SAFE_RELEASE(g_pVBPlanetShpere);
	SAFE_RELEASE(g_pIBPlanetShpere);
	SAFE_RELEASE(g_pVertexLayout);
	SAFE_RELEASE(g_pVertexShader);
	SAFE_RELEASE(g_pPSSkySphere);
	SAFE_RELEASE(g_pPSStarSphere);
	SAFE_RELEASE(g_pPSPlanetSphere);
	SAFE_RELEASE(g_pDepthStencil);
	SAFE_RELEASE(g_pDepthStencilView);
	SAFE_RELEASE(g_pRenderTargetView);
	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pImmediateContext);
	SAFE_RELEASE(g_pd3dDevice);
}