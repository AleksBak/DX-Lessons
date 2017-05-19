
#include <new>
#include "main.h"
#include "SpaceStars.h"

CSpaceStars::CSpaceStars(ID3D11DeviceContext* pImmediateContext, ID3D11Device *pd3dDevice, ID3D11Buffer* pBufferWorldMatrix, UINT nStarsInBlock, UINT nBlocksCount, UINT nBlockWidth)
{
	m_pIContext = pImmediateContext;
	m_pd3dDevice = pd3dDevice;
	m_pCBWorldMatrix = pBufferWorldMatrix;
	m_nStarsInBlock = nStarsInBlock;
	m_nBlocksCount = nBlocksCount;
	m_nBlockWidth = nBlockWidth;
	m_fFovAngle = 0.0f;
	m_fAngle1 = m_fAngle2 = 0.0f;
	m_vCameraPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_vCameraDir = XMFLOAT3(0.0f, 0.0f, 1.0f);

	try {
		m_pStars = new ISingleStar[m_nStarsInBlock * m_nBlocksCount * m_nBlocksCount];
		GenerateStars();
	}catch (std::bad_alloc &a) {
		MessageBoxA(NULL, a.what(), "Error", 0);
	}

	// Создание буферов вершин и индексов
    // Создаем квадрат, на который будет натягиваться текстура звезды
	HRESULT hr = S_OK;
    SimpleVertex vertices[] =
    {	/* координаты X, Y, Z				координаты текстры tu, tv	нормаль X, Y, Z			 */
        { XMFLOAT3( -1.0f,  1.0f,  0.0f ),	XMFLOAT2( 0.0f, 0.0f ),		XMFLOAT3( 0.0f, 0.0f,-1.0f ) },
        { XMFLOAT3(  1.0f,  1.0f,  0.0f ),	XMFLOAT2( 1.0f, 0.0f ),		XMFLOAT3( 0.0f, 0.0f,-1.0f ) },
        { XMFLOAT3(  1.0f, -1.0f,  0.0f ),	XMFLOAT2( 1.0f, 1.0f ),		XMFLOAT3( 0.0f, 0.0f,-1.0f ) },
        { XMFLOAT3( -1.0f, -1.0f,  0.0f ),	XMFLOAT2( 0.0f, 1.0f ),		XMFLOAT3( 0.0f, 0.0f,-1.0f ) }
    };

	D3D11_BUFFER_DESC bd;						// Структура, описывающая создаваемый буфер
	ZeroMemory( &bd, sizeof(bd) );				// очищаем ее
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * 4;	// размер буфера
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	// тип буфера - буфер вершин
	bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;			// Структура, содержащая данные буфера
	ZeroMemory( &InitData, sizeof(InitData) );	// очищаем ее
    InitData.pSysMem = vertices;
    hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pVBStar );
	if (FAILED(hr)) m_pVBStar = NULL;

    // Создание буфера индексов
    WORD indices[] =
    {
        3,1,0,
        2,1,3
    };
	bd.Usage = D3D11_USAGE_DEFAULT;			// Структура, описывающая создаваемый буфер
	bd.ByteWidth = sizeof( WORD ) * 6;		// 6 вершин для 2 треугольников
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER; // тип - буфер индексов
	bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;				// указатель на наш массив индексов
    hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pIBStar );
    if (FAILED(hr)) m_pIBStar = NULL;
}


CSpaceStars::~CSpaceStars(void)
{
	m_pIContext = NULL;
	SAFE_RELEASE(m_pVBStar);
	SAFE_RELEASE(m_pIBStar);
	SAFE_RELEASE(m_pCBWorldMatrix);
	
	delete [] m_pStars;
}

void CSpaceStars::GenerateStars()
{
	UINT nBlock;
	float nBorderX, nBorderZ;

	// Заполняем m_nBlocksCount*m_nBlocksCount блоков в пространстве звездами со случайными координатами

	nBorderX = -((float)m_nBlocksCount / 2) * m_nBlockWidth;
	nBorderZ = -((float)m_nBlocksCount / 2) * m_nBlockWidth;
	for (nBlock = 0; nBlock < m_nBlocksCount*m_nBlocksCount; nBlock++) {
		for (UINT i=0; i<m_nStarsInBlock; i++) {
			m_pStars[nBlock * m_nStarsInBlock + i].pos.x = nBorderX + ((float)rand() / (RAND_MAX + 1)) * m_nBlockWidth;
			m_pStars[nBlock * m_nStarsInBlock + i].pos.z = nBorderZ + ((float)rand() / (RAND_MAX + 1)) * m_nBlockWidth;
			m_pStars[nBlock * m_nStarsInBlock + i].pos.y = ((float)rand() / (RAND_MAX + 1)) * (float)(4 * m_nBlockWidth) - 
				(4 * m_nBlockWidth) / 2;
			m_pStars[nBlock * m_nStarsInBlock + i].size = rand() % 4 + 1;
			m_pStars[nBlock * m_nStarsInBlock + i].color.x = m_pStars[nBlock * m_nStarsInBlock + i].color.y = 
				m_pStars[nBlock * m_nStarsInBlock + i].color.z = m_pStars[nBlock * m_nStarsInBlock + i].color.w = 1.0f;
		}
		if (nBlock % m_nBlocksCount == 0) {
			nBorderX = -((float)m_nBlocksCount / 2) * m_nBlockWidth;
			nBorderZ += (float)m_nBlockWidth;
		} else {
			nBorderX += (float)m_nBlockWidth;
		}
	}
}

void CSpaceStars::UpdateCameraLineEquations()
{
	float defx, defz; // dx, dz
	float alpha; // угол оси взгляда; углы двух прямых, ограничивающих область обзора
	defx = m_vCameraDir.x - m_vCameraPos.x;
	defz = m_vCameraDir.z - m_vCameraPos.z;
	alpha = GetAngle(defx, defz);
	
	m_fAngle1 = alpha - m_fFovAngle / 2;
	m_fAngle2 = alpha + m_fFovAngle / 2;
}

inline float CSpaceStars::GetAngle(float dx, float dz)
{
	float tga, alpha;
	if (dx == 0.0f && dz == 0.0f) return m_fAngle1;
    if (dx == 0) {
		if (dz > 0) alpha = XM_PIDIV2; else alpha = 3 * XM_PIDIV2;
        return alpha;
	}
    if (dz == 0) {
		if (dx > 0) alpha = 0; else alpha = XM_PI;
        return alpha;
	}

	tga = abs(dz) / abs(dx);
	alpha = atanf(tga);
	if (dx > 0 && dz < 0)
		alpha = XM_2PI - alpha;
	else if (dx < 0 && dz < 0)
		alpha = XM_PI + alpha;
	else if (dx < 0 && dz > 0)
		alpha = XM_PI - alpha;
	if (alpha < 0.0f) alpha += XM_2PI;
	return alpha;
}

inline void CSpaceStars::GetLineEquation(float &retA, float &retB, float x1, float z1, float angle)
{
	float x2, z2;
	x2 = x1 + cosf(angle);
	z2 = z1 + sinf(angle);
	retA = -(z2 - z1) / (x2 - x1);
	retB = z1 + retA * x1;
}

inline BOOL CSpaceStars::IsPointInCamera(float x, float z)
{
	BOOL bRet = FALSE;
	float linez;

	linez = GetAngle(x - m_vCameraPos.x, z - m_vCameraPos.z);
	if (linez >= m_fAngle1 && linez <= m_fAngle2)
        bRet = TRUE;
	if (m_fAngle1 < XM_2PI && m_fAngle2 > XM_2PI)
		if ((linez+XM_2PI) >= m_fAngle1 && (linez+XM_2PI) <= m_fAngle2)
			bRet = TRUE;

	return bRet;
}

void CSpaceStars::Render()
{
	UINT nBlock;
	float nBorderX, nBorderZ;
	if (m_pVBStar == NULL || m_pIBStar == NULL) return;

	// Установка буфера вершин
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    m_pIContext->IASetVertexBuffers( 0, 1, &m_pVBStar, &stride, &offset );
    m_pIContext->IASetIndexBuffer( m_pIBStar, DXGI_FORMAT_R16_UINT, 0 );
    m_pIContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	UpdateCameraLineEquations();

	nBorderX = -((float)m_nBlocksCount / 2) * m_nBlockWidth;
	nBorderZ = -((float)m_nBlocksCount / 2) * m_nBlockWidth;
	for (nBlock = 0; nBlock < m_nBlocksCount*m_nBlocksCount; nBlock++) {
		// ПРОВЕРКА: если блок не виден в камере, переходим к следующему;
		if (IsPointInCamera(nBorderX, nBorderZ) || IsPointInCamera(nBorderX+m_nBlockWidth, nBorderZ) ||
				IsPointInCamera(nBorderX, nBorderZ+m_nBlockWidth) || 
				IsPointInCamera(nBorderX+m_nBlockWidth, nBorderZ+m_nBlockWidth)) {

			for (UINT i=0; i<m_nStarsInBlock; i++) {
				// ПРОВЕРКА: если звезда не видна в камере, переходим к следующей;
				/*if (!IsPointInCamera(m_pStars[nBlock * m_nStarsInBlock + i].pos.x, m_pStars[nBlock * m_nStarsInBlock + i].pos.z))
					continue;*/
	
				// Устанавливаем матрицу мира и рисуем звезду
				SetMatrix(m_pStars[nBlock * m_nStarsInBlock + i].pos);
				m_pIContext->DrawIndexed( 6, 0, 0 );
			}
		}
		if (nBlock % m_nBlocksCount == 0) {
			nBorderX = -((float)m_nBlocksCount / 2) * m_nBlockWidth;
			nBorderZ += (float)m_nBlockWidth;
		} else {
			nBorderX += (float)m_nBlockWidth;
		}
	}
}

void CSpaceStars::SetMatrix(XMFLOAT3 f3Position)
{
	float angleX, angleY;
	XMMATRIX m_World, m_Pos, m_RotateX, m_RotateY, m_RotateZ, m_Rotate, m_Scale;

	angleX = GetAngle(m_vCameraDir.y - m_vCameraPos.y, m_vCameraDir.z - m_vCameraPos.z);
	angleY = GetAngle(m_vCameraDir.x - m_vCameraPos.x, m_vCameraDir.z - m_vCameraPos.z) + XM_PIDIV4;
	m_Pos = XMMatrixTranslation(f3Position.x, f3Position.y, f3Position.z);

	// Звезда всегда должна быть повернута лицом к камере. Для этого вычисляем вектор от камеры к звезде,
	// а потом его угол.
	XMFLOAT3 f3Dir(m_vCameraPos.x - m_vCameraDir.x, m_vCameraPos.y - m_vCameraDir.y, m_vCameraPos.z - m_vCameraDir.z);
	if (f3Dir.x > 0.0f)
		angleY = -atanf(f3Dir.z / f3Dir.x) + XM_PIDIV2;
	else if (f3Dir.x < 0.0f)
		angleY = -atanf(f3Dir.z / f3Dir.x) - XM_PIDIV2;
	else
		angleY = 0;

	// Умножаем матрицы вращения и трансляции
	m_RotateY = XMMatrixRotationY(angleY);
	m_World = m_RotateY * m_Pos;

	//m_Pos = XMMatrixTranslation(0.0f, 0.0f, 0.0f);

	//angleX = GetAngle(f3Position.y - m_vCameraPos.y, f3Position.z - m_vCameraPos.z);
	//angleZ = GetAngle(f3Position.x - m_vCameraPos.x, f3Position.y - m_vCameraPos.y);
	//m_RotateX = XMMatrixRotationX(angleX);
	//m_RotateZ = XMMatrixRotationZ(angleZ);
	//m_Rotate = /*m_RotateZ **/ m_RotateX * m_RotateY;

    // Обновление содержимого константного буфера
    ConstantBufferWorldMatrix cb1;	// временный контейнер для первого буферв
	cb1.mWorld = XMMatrixTranspose( m_World );	// загружаем в него матрицы
	cb1.fObjectType = GetDistance(m_vCameraPos, f3Position);
	m_pIContext->UpdateSubresource( m_pCBWorldMatrix, 0, NULL, &cb1, 0, 0 );	
	m_pIContext->VSSetConstantBuffers( 1, 1, &m_pCBWorldMatrix );
	m_pIContext->PSSetConstantBuffers( 1, 1, &m_pCBWorldMatrix );

}