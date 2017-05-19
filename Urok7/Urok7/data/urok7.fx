//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// ����������� ������
//--------------------------------------------------------------------------------------
Texture2D txSky : register( t0 );			// ����� ��������
Texture2D txStar : register( t1 );			// ����� ��������
Texture2D txPlanet : register( t2 );		// ����� ��������
Texture2D txMoon : register( t3 );			// ����� ��������
Texture2D txHello : register( t4 );			// ����� ��������
// ������� ����� ������� �������� � ������ ������������, �� ������
// � ���� �����, � ���������� ����

SamplerState samLinear : register( s0 );	// ����� �������

// ����� � ����������� � ��������
cbuffer CB_Matrixes : register( b0 )
{
	matrix View;			// ������� ����
	matrix Projection;		// ������� ��������
}

// ����� � ����������� � ������� ����
cbuffer CB_WorldMatrix : register( b1 )
{
	matrix World;			// ������� ����
	float fObjectType;		// �������������� ���������� ��� ��������
}

// ����� � ����������� � �����
cbuffer CB_Ligth : register( b2 )
{
	float4 vLightPos;		// ���������� ��������� �����
	float4 vLightColor;		// ���� ��������� �����
}

//--------------------------------------------------------------------------------------
struct VS_INPUT					// �������� ������ ���������� �������
{
    float4 Pos : POSITION;		// ������� �� X, Y, Z
	float2 Tex : TEXCOORD0;		// ���������� �������� �� tu, tv
    float3 Norm : NORMAL;		// ������� �� X, Y, Z
};

struct PS_INPUT					// �������� ������ ����������� �������
{
    float4 Pos : SV_POSITION;	// ������� ������� � �������� (��������)
	float2 Tex : TEXCOORD0;		// ���������� �������� �� tu, tv
    float3 Norm : TEXCOORD1;	// ������������� ������� ������� �� tu, tv
};

//--------------------------------------------------------------------------------------
// ��������� ������
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Norm = mul( input.Norm, World );
	output.Tex = input.Tex;
    
    return output;
}

//--------------------------------------------------------------------------------------
// ���������� ������ ��� �������
//--------------------------------------------------------------------------------------
float4 PS_Planet( PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;
	float4 vLightDir = vLightPos;
	vLightDir.x = - vLightPos.x;
	vLightDir.y = - vLightPos.y;
	vLightDir.z = - vLightPos.z;
    
    // ���������� ������������ ������� �� ���� ���������� �����
    finalColor = saturate( dot((float3)vLightPos, input.Norm) * vLightColor );
	if (fObjectType == 0) {
		finalColor *= txPlanet.Sample( samLinear, input.Tex );
	} else if (fObjectType == 1) {
		finalColor *= txMoon.Sample( samLinear, input.Tex );
	} else if (fObjectType == 2) {
		finalColor *= txHello.Sample( samLinear, input.Tex );
	} else if (fObjectType == 3) {
		finalColor.r = finalColor.g = finalColor.b = finalColor.a = 2.0f;
		finalColor *= txPlanet.Sample( samLinear, input.Tex );
	}
	//finalColor = txPlanet.Sample( samLinear, input.Tex );

    finalColor.a = 1.0f;
    return finalColor;
}

//--------------------------------------------------------------------------------------
// ���������� ������ ��� ����
//--------------------------------------------------------------------------------------
float4 PS_SkySphere( PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;
	finalColor.x = finalColor.y = finalColor.z = 0.5f;
	finalColor *= txSky.Sample( samLinear, input.Tex );
	//finalColor.x = finalColor.z = 1.0f;
	//finalColor.y = 0.0f;
    finalColor.a = 1.0f;
    return finalColor;
}

//--------------------------------------------------------------------------------------
// ���������� ������ ��� ������
//--------------------------------------------------------------------------------------
float4 PS_Star( PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;
	float dist = 190.0f;
    
    // ���������� ������������ ������� �� ���� ���������� �����
	finalColor = txStar.Sample( samLinear, input.Tex );

	// fObjectType = ���������� �� ������ �� ������
	// ��� ����������� ���������� ������ � ������� ������ ������ (�� �����)
	if (fObjectType < dist) {
		finalColor.b -= (1.0f - 1.0f * fObjectType / dist);
		finalColor.g -= (0.3f - 0.3f * fObjectType / dist);
	}
    //finalColor.a = 1.0f; //1 - (dist1 / dist0);
	if (finalColor.a < 0.0f) finalColor.a = 0.0f;
	
    return finalColor;
}
