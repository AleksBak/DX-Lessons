
#define COMPILESHADER_ERROR(x) if (FAILED(x)) { MessageBox( NULL, L"Невозможно скомпилировать файл FX. Пожалуйста, переустановите программу.", L"Ошибка", MB_OK ); return x; }

// Настройки программы
struct SCREENSAVEROPTIONS {
	WCHAR strMediaPath[256];
	BOOL  bSound;
	UINT  nStarsCountInBlock;
	FLOAT fSpeed;
} g_Options;

/*--------------------------- GLOBAL VARIABLES -----------------------------*/
HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = NULL;		// Устройство (для создания объектов)
ID3D11DeviceContext*    g_pImmediateContext = NULL;	// Контекст (устройство рисования)
IDXGISwapChain*         g_pSwapChain = NULL;		// Цепь связи (буфера с экраном)
ID3D11RenderTargetView* g_pRenderTargetView = NULL;	// Объект вида, задний буфер
ID3D11Texture2D*        g_pDepthStencil = NULL;		// Текстура буфера глубин
ID3D11DepthStencilView* g_pDepthStencilView = NULL;	// Объект вида, буфер глубин

ID3D11VertexShader*     g_pVertexShader = NULL;		// Вершинный шейдер
ID3D11PixelShader*      g_pPSSkySphere = NULL;		// Пиксельный шейдер: сфера звезд
ID3D11PixelShader*      g_pPSPlanetSphere = NULL;	// Пиксельный шейдер: планета
ID3D11PixelShader*      g_pPSStarSphere = NULL;		// Пиксельный шейдер: одиночная звезда
ID3D11BlendState*		g_pBSDefault = NULL;		// Блендинг: отключен
ID3D11BlendState*		g_pBSStar = NULL;			// Блендинг: звезда
ID3D11BlendState*		g_pBSPlanetLight = NULL;	// Блендинг: свечение планеты
ID3D11InputLayout*      g_pVertexLayout = NULL;		// Описание формата вершин
ID3D11Buffer*           g_pVBSkyShpere = NULL;		// Буфер вершин: сфера неподвижных звезд
ID3D11Buffer*           g_pVBPlanetShpere = NULL;	// Буфер вершин: планета
ID3D11Buffer*           g_pIBSkyShpere = NULL;		// Буфер индексов: сфера неподвижных звезд
ID3D11Buffer*           g_pIBPlanetShpere = NULL;	// Буфер индексов: планета
CSpaceStars*			g_pStars = NULL;

XMMATRIX                g_World;					// Матрица мира
XMMATRIX                g_View;						// Матрица вида
XMMATRIX                g_Projection;				// Матрица проекции
ICamera					g_Camera;

ID3D11Buffer*           g_pCBMatrixes = NULL;		// Константный буфер с информацией о матрицах
ID3D11Buffer*           g_pCBWorldMatrix = NULL;	// Константный буфер с информацией о матрицах
ID3D11Buffer*           g_pCBLight = NULL;			// Константный буфер с информацией о свете

XMFLOAT4				g_vLightPos;				// Направление света (позиция источника)
XMFLOAT4				g_vLightColors;				// Цвет источника

ID3D11ShaderResourceView* g_pTexRVStars = NULL;		// Объект текстуры
ID3D11ShaderResourceView* g_pTexRVSun = NULL;		// Объект текстуры
ID3D11ShaderResourceView* g_pTexRVPlanet = NULL;	// Объект текстуры
ID3D11ShaderResourceView* g_pTexRVStarMask = NULL;	// Объект текстуры
ID3D11ShaderResourceView* g_pTexRVMoon = NULL;		// Объект текстуры
ID3D11ShaderResourceView* g_pTexRVHello = NULL;		// Объект текстуры
ID3D11SamplerState*       g_pSamplerLinear = NULL;	// Параметры наложения текстуры

CXPlanet*				g_pSky = NULL;				// Космические объекты
CXPlanet*				g_pPlanet = NULL;
CXPlanet*				g_pMoon = NULL;
CXWhirligig*			g_pNukki = NULL;
CXACTInterface			g_XACTAudio;				// Объект аудиоплеера
ISun					g_PlanetPos;				// Положение активной планеты

BOOL					g_RotateLeft = FALSE;		// Флаги управления камерой (не используются)
BOOL					g_RotateRight = FALSE;
BOOL					g_MoveForward = FALSE;
BOOL					g_MoveBack = FALSE;

UINT					g_nFPS = 0;					// Счетчик кадров в секунду
UINT					g_nFPSCounter = 0;			// Счетчик кадров в секунду (вспомогательная переменная
FLOAT					g_fMoveNukkiAngleX = 0.0f;	// Углы движения объекта g_pNukki
FLOAT					g_fMoveNukkiAngleY = 0.0f;

BOOL					g_bPreviewMode = FALSE;		// Флаг режима просмотра в окне "Параметры экрана - Заставка"
HWND					g_hParentWnd = NULL;		// Родительское окно ("Параметры экрана - Заставка")
HANDLE					g_hmxProgramWorks = NULL;	// Мьютекс для определения уже запущенной копии программы