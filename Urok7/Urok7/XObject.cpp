#include "main.h"
#include "XObject.h"

CXObject::CXObject(ID3D11DeviceContext *pImmediateContext, ID3D11Device *pd3dDevice, ID3D11PixelShader** pPSPlanet)
{
	// Инициализация указателей и переменных
	m_pIContext = pImmediateContext;
	m_pd3dDevice = pd3dDevice;
	m_pVB = m_pIB = m_pCBWorldMatrix = NULL;
	m_pRasterState = NULL;
	m_pPSPlanet = pPSPlanet;
	m_bTextureIndex = 0;
	m_vPos = m_vRotate = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_vScale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	// Создаем константный буфер для загрузки матрицы мира в шейдер
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBufferWorldMatrix);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HRESULT hr = m_pd3dDevice->CreateBuffer(&bd, NULL, &m_pCBWorldMatrix);

	// Растровые параметры рисования: отключаем CULLING (чтобы видеть обратную сторону треугольников)
	// Значения по умолчанию кроме CullMode
	D3D11_RASTERIZER_DESC pRasterDesc;
	ZeroMemory(&pRasterDesc, sizeof(D3D11_RASTERIZER_DESC));
	pRasterDesc.FillMode = D3D11_FILL_SOLID;
	pRasterDesc.CullMode = D3D11_CULL_NONE;
	m_pd3dDevice->CreateRasterizerState(&pRasterDesc, &m_pRasterState);
}


CXObject::~CXObject(void)
{
	m_pIContext = NULL;
	m_pd3dDevice = NULL;
	SAFE_RELEASE(m_pCBWorldMatrix);
	SAFE_RELEASE(m_pRasterState);
	SAFE_RELEASE(m_pVB);
	SAFE_RELEASE(m_pIB);
}

SimpleVertex CXObject::sv(XMFLOAT3 pos, XMFLOAT2 tex, XMFLOAT3 norm)
{
	SimpleVertex v;
	v.Pos = pos; v.Normal = norm; v.Tex = tex;

	// Особый случай: необходимо подсчитать нормаль самостоятельно
	if (norm.x == 10.0f && norm.z == 0.0f)
	{
		// Чтобы найти нормаль, вычисляем вектор из (0, 0, 0) в точку pos,
		// а затем сокращаем до длины = 1. Получилось немного коряво.
		BOOL a1 = FALSE, b1 = FALSE;
		float a = 1.0f, b = 1.0f;
		(pos.y == 0.0f) ? a1 = TRUE : a = pos.x / pos.y;
		(pos.z == 0.0f) ? b1 = TRUE : b = pos.x / pos.z;
		float mul = norm.y;
		// Нормализация позиции = превращение в нормаль
		float q = 1.0f + (1.0f - (int)a1)*1.0f / (a*a) + (1.0f - (int)b1)*1.0f / (b*b);
		norm.x = mul * sqrtf(1.0f / q);
		if (a1) norm.y = 0; else norm.y = norm.x / a;
		if (b1) norm.z = 0; else norm.z = norm.x / b;
		if ((norm.x > 0.0f && pos.x < 0.0f) || (norm.x < 0.0f && pos.x > 0.0f)) norm.x = -norm.x;
		if ((norm.y > 0.0f && pos.y < 0.0f) || (norm.y < 0.0f && pos.y > 0.0f)) norm.y = -norm.y;
		if ((norm.z > 0.0f && pos.z < 0.0f) || (norm.z < 0.0f && pos.z > 0.0f)) norm.z = -norm.z;
		v.Normal = norm;
	}
	return v;
}

void CXObject::Render()
{
	if (m_pVB == NULL || m_pIB == NULL) return;

	// Установить пиксельный шейдер
	m_pIContext->PSSetShader(*m_pPSPlanet, NULL, 0);

	// Установить буфера вершин
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	m_pIContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
	m_pIContext->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0);
	m_pIContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pIContext->RSSetState(m_pRasterState); // CULLMODE = OFF

	// Установить матрицу
	XMMATRIX mWorld = GetMatrix();
	ConstantBufferWorldMatrix cb1;	// временный контейнер для первого буферв
	cb1.mWorld = XMMatrixTranspose(mWorld);	// загружаем в него матрицы
	cb1.fObjectType = (float)m_bTextureIndex;
	m_pIContext->UpdateSubresource(m_pCBWorldMatrix, 0, NULL, &cb1, 0, 0);
	m_pIContext->VSSetConstantBuffers(1, 1, &m_pCBWorldMatrix);
	m_pIContext->PSSetConstantBuffers(1, 1, &m_pCBWorldMatrix);

	// Нарисовать объект
	m_pIContext->DrawIndexed(m_nIndexesCount, 0, 0);
}

XMMATRIX CXObject::GetMatrix()
{
	// Тут все очевидно
	XMMATRIX mWorld, mTranslate, mScale, mRotate, mRotateX, mRotateY, mRotateZ;
	mTranslate = XMMatrixTranslation(m_vPos.x, m_vPos.y, m_vPos.z);
	mScale = XMMatrixScaling(m_vScale.x, m_vScale.y, m_vScale.z);
	mRotateX = XMMatrixRotationX(m_vRotate.x);
	mRotateY = XMMatrixRotationY(m_vRotate.y);
	mRotateZ = XMMatrixRotationZ(m_vRotate.z);
	mRotate = mRotateY * mRotateX * mRotateZ;
	mWorld = mScale * mRotate * mTranslate;
	return mWorld;
}

float CXObject::GetAngle(float dx, float dz)
{
	float tga, alpha;
	if (dx == 0.0f && dz == 0.0f) return 0.0f;
	if (dx == 0)
	{
		if (dz > 0) alpha = XM_PIDIV2; else alpha = 3 * XM_PIDIV2;
		return alpha;
	}
	if (dz == 0)
	{
		if (dx > 0) alpha = 0; else alpha = XM_PI;
		return alpha;
	}

	tga = fabs(dz) / fabs(dx);		// XXX!
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