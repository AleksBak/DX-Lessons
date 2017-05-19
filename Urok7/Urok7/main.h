#define _CRT_SECURE_NO_WARNINGS 1

/*-------------------------------- DEFINES --------------------------------*/
#define BLOCKS_MAX 12
#define BLOCK_SIZE 200.0f
#define GAME_RADIUS 850.0f
#define GAME_ORBIT 1.35f

#define TEXIND_PLANET 0
#define TEXIND_MOON 1
#define TEXIND_NUKKI 2
#define TEXIND_SUN 3

#define REGIME_FULLSCREEN

/*-------------------------------- INCLUDES --------------------------------*/
#ifndef __DX_INCLEDES
#define __DX_INCLEDES

#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>

/*--------------------------------- MACROS ---------------------------------*/
#define V_RETURN(x)     { hr = (x); if( FAILED(hr) ) { return hr; } }
#define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL; }

/*------------------------------- STRUCTURES -------------------------------*/
struct SimpleVertex // Структура вершины
{
	XMFLOAT3 Pos;	 // Координаты точки в пространстве
	XMFLOAT2 Tex;	 // Координаты текстуры
	XMFLOAT3 Normal; // Нормаль вершины
};


// Структура константного буфера (совпадает со структурой в шейдере)
struct ConstantBufferMatrixes
{
	XMMATRIX mView;			// Матрица вида
	XMMATRIX mProjection;	// Матрица проекции
};

// Структура константного буфера (совпадает со структурой в шейдере)
struct ConstantBufferWorldMatrix
{
	XMMATRIX mWorld;		// Матрица мира
	FLOAT fObjectType;
};

// Структура константного буфера (совпадает со структурой в шейдере)
struct ConstantBufferLight
{
	XMFLOAT4 vLightPos;		// Направление света
	XMFLOAT4 vLightColor;	// Цвет источника
};

// Структура простой камеры
struct ICamera {
	XMFLOAT3 Pos;
	FLOAT AngleY;
	FLOAT AngleX;
	void Init()
	{
		Pos.x = Pos.y = Pos.z = 0.0f;
		AngleY = AngleX = 0.0f;
	}
	void Move(float value)
	{
		Pos.x += (value * cosf(AngleY));
		Pos.z += (value * sinf(AngleY));
	}
	void RotateY(float value)
	{
		AngleY += value;
	}
};

// Структура положения планеты: она запоминает предыдущее положение и заранее вычисляет
// следующее, чтобы можно было их отрисосывать (не используется)
struct ISun {
	XMFLOAT3 Position[3];	// 0 - предыдущая, 1 - текущая, 2 - следующая
	UINT nTextureIndex[3];	// 0 - предыдущая, 1 - текущая, 2 - следующая
	void Init()
	{
		Position[1] = Position[2] = XMFLOAT3(2.0f, 0.0f, 0.0f);
		nTextureIndex[0] = nTextureIndex[2] = nTextureIndex[2] = 0;
		fAngle = 0.0f;
	}
	void Shift(int iterations = 1)
	{
		for (int i = 0; i < iterations; i++)
		{
			float angle, inc; int n;
			// Отодвигаемся от нуля, чтобы избежать неопределенностей при сравнении углов около 2pi-0pi
			fAngle += XM_2PI * 2;
			do
			{
				angle = fAngle - 1.8f * XM_PI * ((float)rand() / RAND_MAX);
			} while ((angle - fAngle < 1.1f) && (angle - fAngle > 0.0f));
			while (angle > XM_2PI) { angle -= XM_2PI; };
			fAngle = angle;

			Position[0] = Position[1];
			Position[1] = Position[2];
			nTextureIndex[0] = nTextureIndex[1];
			nTextureIndex[1] = nTextureIndex[2];

			//n = rand() % 15;
			if (nTextureIndex[2] == TEXIND_SUN) n = 7; else if (nTextureIndex[2] == TEXIND_MOON) n = 12; else n = 2;
			if (n < 5)
			{
				nTextureIndex[2] = TEXIND_SUN;
				inc = GAME_ORBIT;
				Position[2].y = -1.7f;
			}
			else
			{
				if (n < 10) nTextureIndex[2] = TEXIND_MOON; else nTextureIndex[2] = TEXIND_PLANET;
				if (rand() % 10 < 5) inc = GAME_ORBIT * 2.0f; else inc = 0.0f;
				Position[2].y = 1.0f;
			}
			Position[2].x = cosf(angle) * (GAME_RADIUS + inc);
			Position[2].z = sinf(angle) * (GAME_RADIUS + inc);
		}
	}
private:
	float fAngle;
};

// Предварительное объявление глобальной фукнции
void SetBlendEnable(BOOL bEnable = TRUE);

// Часто использующаяся функция: вычисление расстояния между двумя точками в пространстве
inline float GetDistance(XMFLOAT3 v1, XMFLOAT3 v2)
{
	return sqrtf((v1.x - v2.x)*(v1.x - v2.x) + (v1.y - v2.y)*(v1.y - v2.y) + (v1.z - v2.z)*(v1.z - v2.z));
}

// Часто использующаяся функция: вычисления угла между нулем координат (0, 0), точкой (dx, dz) и нулевой осью
inline float fGetAngle(float dx, float dz)
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

#endif
