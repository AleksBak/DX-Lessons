#pragma once

class CXObject
{
protected:
	ID3D11DeviceContext*		m_pIContext;		// Указатели на основные интерфейсы Direct3D
	ID3D11Device*				m_pd3dDevice;
	ID3D11RasterizerState*		m_pRasterState;

	ID3D11Buffer*               m_pVB;				// Буфер вершин
	ID3D11Buffer*               m_pIB;				// Буфер индексов
	ID3D11Buffer*				m_pCBWorldMatrix;	// Константный буфер для матрицы мира
	ID3D11PixelShader**			m_pPSPlanet;		// Указатель на используемый пиксельный шейдер

	XMFLOAT3					m_vPos;				// Положение объекта в пространстве
	XMFLOAT3					m_vRotate;			// Данные о повороте вокруг своих осей
	XMFLOAT3					m_vScale;			// Масштаб объекта

	UINT						m_nIndexesCount;	// Количество рисуемых вершин (для рендеринга)
	
	XMMATRIX GetMatrix();							// Функция возвращает готовую матрицу мира для объекта
	SimpleVertex sv(XMFLOAT3 pos, XMFLOAT2 tex, XMFLOAT3 norm);	// вспомогательная функция
	float GetAngle(float dx, float dz);
public:
	UINT						m_bTextureIndex;	// Номер текстуры для объекта (передается в пиксельный шейдер)

public:
	CXObject(ID3D11DeviceContext *pImmediateContext, ID3D11Device *pd3dDevice, ID3D11PixelShader** pPSPlanet);
	~CXObject(void);

	// виртуальная функция, реализована только в потомках класса
	HRESULT CreateModel(float nRadius, UINT nSegments, BOOL bNormals = TRUE, UINT nTexturesCount = 1);

	void Render();

	void SetPosition(XMFLOAT3 vNewPosition) { m_vPos = vNewPosition; };
	XMFLOAT3 GetPosition() { return m_vPos; };
	void SetRotation(XMFLOAT3 vNewRotationInfo) { m_vRotate = vNewRotationInfo; };
	XMFLOAT3 GetRotation() { return m_vRotate; };
	void SetScaling(XMFLOAT3 vNewScaling) { m_vScale = vNewScaling; };
	XMFLOAT3 GetScaling() { return m_vScale; };
};

