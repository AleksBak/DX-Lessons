
/*------------------------------- FUNCTIONS --------------------------------*/
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );  // �������� ����
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );	  // ������� ����
HRESULT InitDevice();			// ������������� ��������� DirectX
HRESULT InitShaders();			// ������������� ������� ����� � ������ ������
HRESULT InitGeometry();			// ������������� ������� ����� � ������ ������
HRESULT InitGeometry_Textures();
VOID    InitGeometry_Audio();
HRESULT InitMatrixes();			// ������������� ������
void CleanupDevice();			// �������� ���������� ��������� DirectX

void UpdateLight();				// ���������� ���������� �����
void UpdateMatrix();			// ���������� ������� ����
void ResetWorldMatrix();		// ����� ������� ���� � ��������� Identity();
void MoveCamera(float coef);	// ������� ����������� ������ (������ ����)
void MovePlanets(float coef);	// ������� ����������� ������� (������ ����)
void MoveNukki(float coef);		// ������� ����������� ������� g_pNukki (������ ����)

void Render();					// ������� ���������
inline void RenderPlanet();		// ������� ��������� ������� [� ��������]

void LoadOptions();				// �������� �������� ��������� �� �����
void SaveOptions();				// ���������� �������� ��������� � ����
void GetFullFileName(LPWSTR strFullFileName, LPWSTR strFile); // strFullFileName = ������� ����� + strFile