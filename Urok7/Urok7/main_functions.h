
/*------------------------------- FUNCTIONS --------------------------------*/
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );  // Создание окна
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );	  // Функция окна
HRESULT InitDevice();			// Инициализация устройств DirectX
HRESULT InitShaders();			// Инициализация шаблона ввода и буфера вершин
HRESULT InitGeometry();			// Инициализация шаблона ввода и буфера вершин
HRESULT InitGeometry_Textures();
VOID    InitGeometry_Audio();
HRESULT InitMatrixes();			// Инициализация матриц
void CleanupDevice();			// Удаление созданнных устройств DirectX

void UpdateLight();				// Обновление параметров света
void UpdateMatrix();			// Обновление матрицы мира
void ResetWorldMatrix();		// Сброс матрицы мира в положение Identity();
void MoveCamera(float coef);	// Функция перемещения камеры (каждый кадр)
void MovePlanets(float coef);	// Функция перемещения планеты (каждый кадр)
void MoveNukki(float coef);		// Функция перемещения объекта g_pNukki (каждый кадр)

void Render();					// Функция рисования
inline void RenderPlanet();		// Функция рисования планеты [и спутника]

void LoadOptions();				// Загрузка настроек программы из файла
void SaveOptions();				// Сохранение настроек программы в файл
void GetFullFileName(LPWSTR strFullFileName, LPWSTR strFile); // strFullFileName = Рабочая папка + strFile