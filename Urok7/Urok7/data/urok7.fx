//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Константные буферы
//--------------------------------------------------------------------------------------
Texture2D txSky : register( t0 );			// Буфер текстуры
Texture2D txStar : register( t1 );			// Буфер текстуры
Texture2D txPlanet : register( t2 );		// Буфер текстуры
Texture2D txMoon : register( t3 );			// Буфер текстуры
Texture2D txHello : register( t4 );			// Буфер текстуры
// Слишком много текстур хранится в памяти одновременно, но памяти
// у меня много, а видеокарты мало

SamplerState samLinear : register( s0 );	// Буфер образца

// Буфер с информацией о матрицах
cbuffer CB_Matrixes : register( b0 )
{
	matrix View;			// Матрица вида
	matrix Projection;		// Матрица проекции
}

// Буфер с информацией о матрице мира
cbuffer CB_WorldMatrix : register( b1 )
{
	matrix World;			// Матрица мира
	float fObjectType;		// Дополнительная информация для шейдеров
}

// Буфер с информацией о свете
cbuffer CB_Ligth : register( b2 )
{
	float4 vLightPos;		// Координаты источника света
	float4 vLightColor;		// Цвет источника света
}

//--------------------------------------------------------------------------------------
struct VS_INPUT					// Входящие данные вершинного шейдера
{
    float4 Pos : POSITION;		// Позиция по X, Y, Z
	float2 Tex : TEXCOORD0;		// Координаты текстуры по tu, tv
    float3 Norm : NORMAL;		// Нормаль по X, Y, Z
};

struct PS_INPUT					// Входящие данные пиксельного шейдера
{
    float4 Pos : SV_POSITION;	// Позиция пикселя в проекции (экранная)
	float2 Tex : TEXCOORD0;		// Координаты текстуры по tu, tv
    float3 Norm : TEXCOORD1;	// Относительная нормаль пикселя по tu, tv
};

//--------------------------------------------------------------------------------------
// Вершинный шейдер
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
// Пиксельный шейдер для планеты
//--------------------------------------------------------------------------------------
float4 PS_Planet( PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;
	float4 vLightDir = vLightPos;
	vLightDir.x = - vLightPos.x;
	vLightDir.y = - vLightPos.y;
	vLightDir.z = - vLightPos.z;
    
    // складываем освещенность пикселя от всех источников света
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
// Пиксельный шейдер для неба
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
// Пиксельный шейдер для звезды
//--------------------------------------------------------------------------------------
float4 PS_Star( PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;
	float dist = 190.0f;
    
    // складываем освещенность пикселя от всех источников света
	finalColor = txStar.Sample( samLinear, input.Tex );

	// fObjectType = расстояние от камеры до звезды
	// при приближении превращаем звезду в подобие нашего солнца (по цвету)
	if (fObjectType < dist) {
		finalColor.b -= (1.0f - 1.0f * fObjectType / dist);
		finalColor.g -= (0.3f - 0.3f * fObjectType / dist);
	}
    //finalColor.a = 1.0f; //1 - (dist1 / dist0);
	if (finalColor.a < 0.0f) finalColor.a = 0.0f;
	
    return finalColor;
}
