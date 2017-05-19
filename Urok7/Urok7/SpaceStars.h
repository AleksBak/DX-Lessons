
struct ISingleStar {
	XMFLOAT3 pos;	// ���������� ������
	BYTE size;		// ������ ������ 1..4 (�� ������������)
	XMFLOAT4 color;	// ���� ������ (�� ������������)
};

#pragma once
class CSpaceStars
{
public:
	CSpaceStars(ID3D11DeviceContext* pImmediateContext, ID3D11Device *pd3dDevice, ID3D11Buffer* pBufferWorldMatrix, UINT nStarsInBlock, UINT nBlocksCount, UINT nBlockWidth);
	~CSpaceStars(void);

	// �������� ��������� ������. ���������� ��� �������� ������ � ���������� ������
	// � ��� ��������� ����������� ����������.
	void SetCamera(XMFLOAT3 f3Eye, XMFLOAT3 f3Direction, FLOAT fFovAngle) { m_vCameraPos = f3Eye ; 
																			m_vCameraDir = f3Direction;
																			m_fFovAngle = fFovAngle; }
	void Render();
protected:
	ID3D11DeviceContext* m_pIContext;	// ��������� �� �������� ���������� Direct3D
	ID3D11Device *m_pd3dDevice;
	ID3D11Buffer* m_pCBWorldMatrix;
	ID3D11Buffer* m_pVBStar;			// ����� ������: ��������� ������
	ID3D11Buffer* m_pIBStar;			// ����� ��������: ��������� ������

	ISingleStar* m_pStars;				// ������ � ������� � �������
	UINT m_nStarsInBlock;				// ���������� ����� � ����� �����
	UINT m_nBlocksCount;				// ���������� ������ (�� ����� �������)
	UINT m_nBlockWidth;					// ������ ������ �����
	XMFLOAT3 m_vCameraPos;				// ��������� ������
	XMFLOAT3 m_vCameraDir;				// �����, � ������� ������ �������
	FLOAT m_fFovAngle;

	float m_fAngle1, m_fAngle2;			// ����, �������������� ����� ������ ������ ������������ ���

	void GenerateStars();
	void UpdateCameraLineEquations();	 // ���������� ����� ������ ������
	void SetMatrix(XMFLOAT3 f3Position); // ��������� ������� ���� ��� ��������� ������
	inline void GetLineEquation(float &retA, float &retB, float x1, float z1, float angle);
	inline BOOL IsPointInCamera(float x, float z);
	inline float GetAngle(float dx, float dz); // ���� ������� ������� (0,0)-(dx, dz)
};

