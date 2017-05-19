#pragma once

class CXWhirligig: public CXObject
{
protected:
	FLOAT						m_fRadius;

protected:
	FLOAT						fGaussFunction(FLOAT x, FLOAT mo, FLOAT disp, FLOAT max);

public:
	CXWhirligig(ID3D11DeviceContext *pImmediateContext, ID3D11Device *pd3dDevice, ID3D11PixelShader** pPSPlanet);
	~CXWhirligig(void);

	HRESULT CreateModel(float nRadius, UINT nSegments, BOOL bNormals = TRUE, UINT nTexturesCount = 1);
};



