#pragma once

// Класс-космический объект: Планета
class CXPlanet: public CXObject
{
protected:
	ID3D11Buffer*               m_pVBLight;		// Дополнительный буфер для света вокруг планеты
	ID3D11Buffer*               m_pIBLight;		// Дополнительный буфер для света вокруг планеты
	ID3D11PixelShader**			m_pPSLight;		// Дополнительный шейдер для света вокруг планеты

	XMFLOAT3					m_vCameraPos;	// Положение камеры (чтобы круг света всегда был повернут лицевой стороной)
	FLOAT						m_fRadius;		// Радиус планеты

	XMMATRIX GetLightMatrix();					// Функция возвращает подготовленную матрицу мира для светового круга

public:
	CXPlanet(ID3D11DeviceContext *pImmediateContext, ID3D11Device *pd3dDevice, ID3D11PixelShader** pPSPlanet, ID3D11PixelShader** pPSLight);
	~CXPlanet(void);

	// Переопределяем функции
	HRESULT CreateModel(float nRadius, UINT nSegments, BOOL bNormals = TRUE, UINT nTexturesCount = 1);
	void Render();
	// Функция загрузки положения камеры
	void SetCameraPos(XMFLOAT3 vNewPos) { m_vCameraPos = vNewPos; };
};

