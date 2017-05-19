#pragma once

class CXObject
{
protected:
	ID3D11DeviceContext*		m_pIContext;		// ��������� �� �������� ���������� Direct3D
	ID3D11Device*				m_pd3dDevice;
	ID3D11RasterizerState*		m_pRasterState;

	ID3D11Buffer*               m_pVB;				// ����� ������
	ID3D11Buffer*               m_pIB;				// ����� ��������
	ID3D11Buffer*				m_pCBWorldMatrix;	// ����������� ����� ��� ������� ����
	ID3D11PixelShader**			m_pPSPlanet;		// ��������� �� ������������ ���������� ������

	XMFLOAT3					m_vPos;				// ��������� ������� � ������������
	XMFLOAT3					m_vRotate;			// ������ � �������� ������ ����� ����
	XMFLOAT3					m_vScale;			// ������� �������

	UINT						m_nIndexesCount;	// ���������� �������� ������ (��� ����������)
	
	XMMATRIX GetMatrix();							// ������� ���������� ������� ������� ���� ��� �������
	SimpleVertex sv(XMFLOAT3 pos, XMFLOAT2 tex, XMFLOAT3 norm);	// ��������������� �������
	float GetAngle(float dx, float dz);
public:
	UINT						m_bTextureIndex;	// ����� �������� ��� ������� (���������� � ���������� ������)

public:
	CXObject(ID3D11DeviceContext *pImmediateContext, ID3D11Device *pd3dDevice, ID3D11PixelShader** pPSPlanet);
	~CXObject(void);

	// ����������� �������, ����������� ������ � �������� ������
	HRESULT CreateModel(float nRadius, UINT nSegments, BOOL bNormals = TRUE, UINT nTexturesCount = 1);

	void Render();

	void SetPosition(XMFLOAT3 vNewPosition) { m_vPos = vNewPosition; };
	XMFLOAT3 GetPosition() { return m_vPos; };
	void SetRotation(XMFLOAT3 vNewRotationInfo) { m_vRotate = vNewRotationInfo; };
	XMFLOAT3 GetRotation() { return m_vRotate; };
	void SetScaling(XMFLOAT3 vNewScaling) { m_vScale = vNewScaling; };
	XMFLOAT3 GetScaling() { return m_vScale; };
};

