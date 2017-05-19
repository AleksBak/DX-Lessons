
#include "main.h"

#include "XObject.h"
#include "XPlanet.h"

CXPlanet::CXPlanet(ID3D11DeviceContext *pImmediateContext, ID3D11Device *pd3dDevice,
							  ID3D11PixelShader** pPSPlanet, ID3D11PixelShader** pPSLight)
	: CXObject(pImmediateContext, pd3dDevice, pPSPlanet)
{
	m_pVBLight = m_pIBLight = NULL;
	m_pPSLight = pPSLight;
}


CXPlanet::~CXPlanet(void)
{
	m_pPSLight = NULL;
	SAFE_RELEASE(m_pVBLight);
	SAFE_RELEASE(m_pIBLight);
}

HRESULT CXPlanet::CreateModel(float nRadius, UINT nSegments, BOOL bNormals /*TRUE = outside, FALSE = inside*/, UINT nTexturesCount)
{
	HRESULT hr = S_OK;
	float dec = (float)2 / nSegments;
	float curry, currangle, currrad, texturey;
	int pntr = 0, nrm;
	m_fRadius = nRadius;

    SimpleVertex *vertices = new SimpleVertex[(nSegments+1) * (nSegments+1)];
	(bNormals) ? nrm = 1 : nrm = -1;

	for (UINT i=0; i<=nSegments; i++) {
		currangle = XM_2PI * i / nSegments;

		for (UINT n=0; n<=nSegments; n++) {
			curry = (1.0f - 2 * (FLOAT)n / nSegments) * nRadius;
			currrad = sqrtf(nRadius*nRadius - curry*curry);

			if (nTexturesCount==1)
				texturey = (FLOAT)nTexturesCount * n / nSegments;
			else {
				texturey = ((FLOAT)nTexturesCount * n / nSegments) * 0.8f + 0.1f;
			}

			if ( n==0 || n==nSegments) {
				vertices[pntr++] = sv(XMFLOAT3(0.0f, curry, 0.0f), 
									  XMFLOAT2((FLOAT)nTexturesCount*i / nSegments, texturey),
									  XMFLOAT3(0.0f, curry*nrm/nRadius, 0.0f));
				continue;
			}

			vertices[pntr++] = sv(XMFLOAT3(cosf(currangle)*currrad, curry, sinf(currangle)*currrad),
								  XMFLOAT2((FLOAT)nTexturesCount*i / nSegments, texturey), 
								  XMFLOAT3(10.0f, (FLOAT)nrm, 0.0f));
		}
	}
	
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * (nSegments+1) * (nSegments+1);	// размер буфера
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pVB );
	if (FAILED(hr)) m_pVB = NULL;

	vertices[0] = sv(XMFLOAT3( -1.0f,  1.0f,  0.0f ), XMFLOAT2( 0.0f, 0.0f ), XMFLOAT3( 0.0f, 0.0f,-1.0f ));
	vertices[1] = sv(XMFLOAT3(  1.0f,  1.0f,  0.0f ), XMFLOAT2( 1.0f, 0.0f ), XMFLOAT3( 0.0f, 0.0f,-1.0f ));
	vertices[2] = sv(XMFLOAT3(  1.0f, -1.0f,  0.0f ), XMFLOAT2( 1.0f, 1.0f ), XMFLOAT3( 0.0f, 0.0f,-1.0f ));
	vertices[3] = sv(XMFLOAT3( -1.0f, -1.0f,  0.0f ), XMFLOAT2( 0.0f, 1.0f ), XMFLOAT3( 0.0f, 0.0f,-1.0f ));
	bd.ByteWidth = sizeof( SimpleVertex ) * 4;
    InitData.pSysMem = vertices;
    hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pVBLight );
	if (FAILED(hr)) m_pVBLight = NULL;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	m_nIndexesCount = nSegments * (nSegments-1)*2*3;
    WORD *indices = new WORD[m_nIndexesCount];
	UINT arcpntr;
	pntr = 0;

	for (UINT i=0; i<nSegments; i++) {
		//(i==(nSegments-1)) ? arcpntr = 0 : arcpntr = i+1;
		arcpntr = i+1;
		for (UINT n=0; n<nSegments; n++) {
			if (n==0) {
				indices[pntr++] = (nSegments+1)*i+n;
				indices[pntr++] = (nSegments+1)*arcpntr+n+1;
				indices[pntr++] = (nSegments+1)*i+n+1;
				continue;
			} else if (n==(nSegments-1)) {
				indices[pntr++] = (nSegments+1)*i+n;
				indices[pntr++] = (nSegments+1)*arcpntr+n;
				indices[pntr++] = (nSegments+1)*i+n+1;
				continue;
			}
			indices[pntr++] = (nSegments+1)*i+n;
			indices[pntr++] = (nSegments+1)*arcpntr+n;
			indices[pntr++] = (nSegments+1)*arcpntr+n+1;
			indices[pntr++] = (nSegments+1)*i+n;
			indices[pntr++] = (nSegments+1)*arcpntr+n+1;
			indices[pntr++] = (nSegments+1)*i+n+1;
		}
	}

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( WORD ) * m_nIndexesCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pIB );
    if (FAILED(hr)) m_pIB = NULL;

	indices[0] = 3; indices[1] = 1; indices[2] = 0;
	indices[3] = 2; indices[4] = 1; indices[5] = 3;
	bd.ByteWidth = sizeof( WORD ) * 6;
	InitData.pSysMem = indices;
    hr = m_pd3dDevice->CreateBuffer( &bd, &InitData, &m_pIBLight );
    if (FAILED(hr)) m_pIBLight = NULL;

	delete [] vertices;
	delete [] indices;

	if(m_pIB == NULL || m_pVB == NULL) return E_FAIL; else return S_OK;
}

void CXPlanet::Render()
{
	CXObject::Render();
	// Установка буфера вершин
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;

	if (m_pPSLight != NULL) {
		m_pIContext->PSSetShader( *m_pPSLight, NULL, 0 );
		m_pIContext->IASetVertexBuffers( 0, 1, &m_pVBLight, &stride, &offset );
		m_pIContext->IASetIndexBuffer( m_pIBLight, DXGI_FORMAT_R16_UINT, 0 );
		
		XMMATRIX mWorld = GetLightMatrix();
		ConstantBufferWorldMatrix cb1;
		cb1.mWorld = XMMatrixTranspose( mWorld );	// загружаем в него матрицы
		// Напоминаем:  в пиксельном шейдере рисования звезды или светового кольца переменная fObjectType
		// определяет желтизну объекта. Чем значение меньше, тем объект (звезда или кольцо света) желтее.
		(m_bTextureIndex == TEXIND_SUN) ? cb1.fObjectType = 115.0f : cb1.fObjectType = 1000.0f;
		m_pIContext->UpdateSubresource( m_pCBWorldMatrix, 0, NULL, &cb1, 0, 0 );	
		m_pIContext->VSSetConstantBuffers( 1, 1, &m_pCBWorldMatrix );
		m_pIContext->PSSetConstantBuffers( 1, 1, &m_pCBWorldMatrix );
		SetBlendEnable(TRUE);
		m_pIContext->DrawIndexed( 6, 0, 0 );
		SetBlendEnable(FALSE);
	}
}

XMMATRIX CXPlanet::GetLightMatrix()
{
	XMMATRIX mWorld, mTranslate, mScale, mRotate, mRotateX, mRotateY, mRotateZ;
	float sc_coef;
	// Вокруг солнца орело свечения должен быть ярче
	(m_bTextureIndex == TEXIND_SUN) ? sc_coef = 1.7f : sc_coef = 1.4f;
	mTranslate = XMMatrixTranslation(m_vPos.x, m_vPos.y, m_vPos.z);
	mScale = XMMatrixScaling(m_vScale.x * m_fRadius * sc_coef, m_vScale.y * m_fRadius * sc_coef, m_vScale.z * m_fRadius * sc_coef);
	//mRotateX = XMMatrixRotationX(m_vRotate.x);
	mRotateY = XMMatrixRotationY(XM_PIDIV2 - GetAngle(m_vCameraPos.x - m_vPos.x, m_vCameraPos.z - m_vPos.z));
	//mRotateZ = XMMatrixRotationZ(m_vRotate.z);
	//mRotate = mRotateX * mRotateY * mRotateZ;
	mWorld = mScale * mRotateY * mTranslate;
	return mWorld;
}