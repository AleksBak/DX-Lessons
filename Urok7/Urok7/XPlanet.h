#pragma once

// �����-����������� ������: �������
class CXPlanet: public CXObject
{
protected:
	ID3D11Buffer*               m_pVBLight;		// �������������� ����� ��� ����� ������ �������
	ID3D11Buffer*               m_pIBLight;		// �������������� ����� ��� ����� ������ �������
	ID3D11PixelShader**			m_pPSLight;		// �������������� ������ ��� ����� ������ �������

	XMFLOAT3					m_vCameraPos;	// ��������� ������ (����� ���� ����� ������ ��� �������� ������� ��������)
	FLOAT						m_fRadius;		// ������ �������

	XMMATRIX GetLightMatrix();					// ������� ���������� �������������� ������� ���� ��� ��������� �����

public:
	CXPlanet(ID3D11DeviceContext *pImmediateContext, ID3D11Device *pd3dDevice, ID3D11PixelShader** pPSPlanet, ID3D11PixelShader** pPSLight);
	~CXPlanet(void);

	// �������������� �������
	HRESULT CreateModel(float nRadius, UINT nSegments, BOOL bNormals = TRUE, UINT nTexturesCount = 1);
	void Render();
	// ������� �������� ��������� ������
	void SetCameraPos(XMFLOAT3 vNewPos) { m_vCameraPos = vNewPos; };
};

