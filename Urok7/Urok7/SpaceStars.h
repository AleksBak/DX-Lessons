
struct ISingleStar {
	XMFLOAT3 pos;	// Координаты звезды
	BYTE size;		// Размер звезды 1..4 (не используется)
	XMFLOAT4 color;	// Цвет звезды (не используется)
};

#pragma once
class CSpaceStars
{
public:
	CSpaceStars(ID3D11DeviceContext* pImmediateContext, ID3D11Device *pd3dDevice, ID3D11Buffer* pBufferWorldMatrix, UINT nStarsInBlock, UINT nBlocksCount, UINT nBlockWidth);
	~CSpaceStars(void);

	// Загрузка положения камеры. Необходимо для передачи данных в пиксельный шейдер
	// и для алгоритма оптимизации рендеринга.
	void SetCamera(XMFLOAT3 f3Eye, XMFLOAT3 f3Direction, FLOAT fFovAngle) { m_vCameraPos = f3Eye ; 
																			m_vCameraDir = f3Direction;
																			m_fFovAngle = fFovAngle; }
	void Render();
protected:
	ID3D11DeviceContext* m_pIContext;	// Указатели на основные интерфейсы Direct3D
	ID3D11Device *m_pd3dDevice;
	ID3D11Buffer* m_pCBWorldMatrix;
	ID3D11Buffer* m_pVBStar;			// Буфер вершин: одиночная звезда
	ID3D11Buffer* m_pIBStar;			// Буфер индексов: одиночная звезда

	ISingleStar* m_pStars;				// Массив с данными о звездах
	UINT m_nStarsInBlock;				// Количество звезд в одном блоке
	UINT m_nBlocksCount;				// Количество блоков (по одной стороне)
	UINT m_nBlockWidth;					// Ширина одного блока
	XMFLOAT3 m_vCameraPos;				// Положение камеры
	XMFLOAT3 m_vCameraDir;				// Точка, в которую камера смотрит
	FLOAT m_fFovAngle;

	float m_fAngle1, m_fAngle2;			// Углы, ограничивающие обзор камеры вокруг вертикальной оси

	void GenerateStars();
	void UpdateCameraLineEquations();	 // Вычисление углов обзора камеры
	void SetMatrix(XMFLOAT3 f3Position); // Установка матрицы мира для отдельной звезды
	inline void GetLineEquation(float &retA, float &retB, float x1, float z1, float angle);
	inline BOOL IsPointInCamera(float x, float z);
	inline float GetAngle(float dx, float dz); // угол наклона вектора (0,0)-(dx, dz)
};

