
#define COMPILESHADER_ERROR(x) if (FAILED(x)) { MessageBox( NULL, L"���������� �������������� ���� FX. ����������, �������������� ���������.", L"������", MB_OK ); return x; }

// ��������� ���������
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
ID3D11Device*           g_pd3dDevice = NULL;		// ���������� (��� �������� ��������)
ID3D11DeviceContext*    g_pImmediateContext = NULL;	// �������� (���������� ���������)
IDXGISwapChain*         g_pSwapChain = NULL;		// ���� ����� (������ � �������)
ID3D11RenderTargetView* g_pRenderTargetView = NULL;	// ������ ����, ������ �����
ID3D11Texture2D*        g_pDepthStencil = NULL;		// �������� ������ ������
ID3D11DepthStencilView* g_pDepthStencilView = NULL;	// ������ ����, ����� ������

ID3D11VertexShader*     g_pVertexShader = NULL;		// ��������� ������
ID3D11PixelShader*      g_pPSSkySphere = NULL;		// ���������� ������: ����� �����
ID3D11PixelShader*      g_pPSPlanetSphere = NULL;	// ���������� ������: �������
ID3D11PixelShader*      g_pPSStarSphere = NULL;		// ���������� ������: ��������� ������
ID3D11BlendState*		g_pBSDefault = NULL;		// ��������: ��������
ID3D11BlendState*		g_pBSStar = NULL;			// ��������: ������
ID3D11BlendState*		g_pBSPlanetLight = NULL;	// ��������: �������� �������
ID3D11InputLayout*      g_pVertexLayout = NULL;		// �������� ������� ������
ID3D11Buffer*           g_pVBSkyShpere = NULL;		// ����� ������: ����� ����������� �����
ID3D11Buffer*           g_pVBPlanetShpere = NULL;	// ����� ������: �������
ID3D11Buffer*           g_pIBSkyShpere = NULL;		// ����� ��������: ����� ����������� �����
ID3D11Buffer*           g_pIBPlanetShpere = NULL;	// ����� ��������: �������
CSpaceStars*			g_pStars = NULL;

XMMATRIX                g_World;					// ������� ����
XMMATRIX                g_View;						// ������� ����
XMMATRIX                g_Projection;				// ������� ��������
ICamera					g_Camera;

ID3D11Buffer*           g_pCBMatrixes = NULL;		// ����������� ����� � ����������� � ��������
ID3D11Buffer*           g_pCBWorldMatrix = NULL;	// ����������� ����� � ����������� � ��������
ID3D11Buffer*           g_pCBLight = NULL;			// ����������� ����� � ����������� � �����

XMFLOAT4				g_vLightPos;				// ����������� ����� (������� ���������)
XMFLOAT4				g_vLightColors;				// ���� ���������

ID3D11ShaderResourceView* g_pTexRVStars = NULL;		// ������ ��������
ID3D11ShaderResourceView* g_pTexRVSun = NULL;		// ������ ��������
ID3D11ShaderResourceView* g_pTexRVPlanet = NULL;	// ������ ��������
ID3D11ShaderResourceView* g_pTexRVStarMask = NULL;	// ������ ��������
ID3D11ShaderResourceView* g_pTexRVMoon = NULL;		// ������ ��������
ID3D11ShaderResourceView* g_pTexRVHello = NULL;		// ������ ��������
ID3D11SamplerState*       g_pSamplerLinear = NULL;	// ��������� ��������� ��������

CXPlanet*				g_pSky = NULL;				// ����������� �������
CXPlanet*				g_pPlanet = NULL;
CXPlanet*				g_pMoon = NULL;
CXWhirligig*			g_pNukki = NULL;
CXACTInterface			g_XACTAudio;				// ������ �����������
ISun					g_PlanetPos;				// ��������� �������� �������

BOOL					g_RotateLeft = FALSE;		// ����� ���������� ������� (�� ������������)
BOOL					g_RotateRight = FALSE;
BOOL					g_MoveForward = FALSE;
BOOL					g_MoveBack = FALSE;

UINT					g_nFPS = 0;					// ������� ������ � �������
UINT					g_nFPSCounter = 0;			// ������� ������ � ������� (��������������� ����������
FLOAT					g_fMoveNukkiAngleX = 0.0f;	// ���� �������� ������� g_pNukki
FLOAT					g_fMoveNukkiAngleY = 0.0f;

BOOL					g_bPreviewMode = FALSE;		// ���� ������ ��������� � ���� "��������� ������ - ��������"
HWND					g_hParentWnd = NULL;		// ������������ ���� ("��������� ������ - ��������")
HANDLE					g_hmxProgramWorks = NULL;	// ������� ��� ����������� ��� ���������� ����� ���������