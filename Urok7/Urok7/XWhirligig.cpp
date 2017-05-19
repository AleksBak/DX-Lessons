
#include "main.h"

#include "XObject.h"
#include "XWhirligig.h"

CXWhirligig::CXWhirligig(ID3D11DeviceContext *pImmediateContext, ID3D11Device *pd3dDevice,
							  ID3D11PixelShader** pPSPlanet)
	: CXObject(pImmediateContext, pd3dDevice, pPSPlanet)
{
	m_fRadius = 0.0f;
	m_bTextureIndex = 2;
}


CXWhirligig::~CXWhirligig(void)
{
}

HRESULT CXWhirligig::CreateModel(float nRadius, UINT nSegments, BOOL bNormals /*TRUE = outside, FALSE = inside*/, UINT nTexturesCount)
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
			currrad = fGaussFunction(curry, 0.0f, nRadius / 3.0f, nRadius / 2.0f);

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

	delete [] vertices;
	delete [] indices;

	if(m_pIB == NULL || m_pVB == NULL) return E_FAIL; else return S_OK;
}

FLOAT CXWhirligig::fGaussFunction(FLOAT x, FLOAT mo, FLOAT disp, FLOAT max)
{
	FLOAT retval, zero, mul;
	zero = expf(-(mo*mo) / (2*disp*disp)) / sqrtf(XM_2PI * disp * disp);
	if (zero == 0.0f)
		mul = 1.0f;
	else
		mul = max / zero;

	retval = mul * expf(-(x - mo)*(x - mo) / (2*disp*disp)) / sqrtf(XM_2PI * disp * disp);
	return retval;
}