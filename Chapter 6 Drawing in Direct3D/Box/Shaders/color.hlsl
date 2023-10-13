//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
//다르게 표현하는 방법도 있는 데 이는 258p 참고.
cbuffer cbPerObject : register(b0) // cbPerObject라는 cbuffer 객체(상수 버퍼)를 참조한다.
{
	float4x4 gWorldViewProj; // 한 점을 국소 공간에서 동차 절단 공간으로 변환하는 데 쓰이는 세계 행렬, 시야 행렬, 투영 행렬을 하나로 결합한 것이다.
};

struct VertexIn // 입력 매개변수
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut // 출력 매개변수
{
	float4 PosH  : SV_POSITION;	// 기하 셰이더를 사용하지 않는다면, 정점 셰이더의 출력은 반드시 의미소가 SV_POSITION인, (SV: system value,시스템 값)
    float4 Color : COLOR;		// 동차 절단 공간에서의 정점 위치이어야한다. 기하셰이더가 없을 때 하드웨어는 정점 셰이더를
								// 떠난 정점들이 동차 절단 공간에 있다고 가정하기 때문이다.
};								// 기하 셰이더를 사용하는 경우에는 동차 절단 공간 위치의 출력을 기하 셰이더에 미룰 수 있다.
								// 원근 나누기까지는 수행하지 말아야한다. 투영 행렬을 곱하는 부분만 하면된다. 원근 나누기는 하드웨어가 알아서
VertexOut VS(VertexIn vin) //vertex shader 정점 셰이더
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	// 동차 절단 공간으로 변환한다.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj); // mul(x,y): float4 x * float4x4 y, 벡터 대 행렬곱셈(여러 크기의 행렬 곱셈 가능), 
															 // float4()행렬 생성 부분은 w = 1로, 벡터가 아닌 점 생성임.
															 // gWorldViewProj은 상수 버퍼에 들어있는 것
	
	// Just pass vertex color into the pixel shader.
	// 정점 색상을 그대로 픽셀 셰이더에 전달한다.
    vout.Color = vin.Color;
    
    return vout;
}
// 픽셀 셰이더의 입력이 정점 셰이더의 출력과 정확히 일치함.
float4 PS(VertexOut pin) : SV_Target //pixel shader
{
    return pin.Color;
}

// 아래와 같은 내용이고 형식만 조금 다르다.
/*
	cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
};

// 입출력 구조체를 사용하지 않는다.

VertexOut VS(float3 iPos : GetRenderTargetSamplePosition, float4 iColor : COLOR, out float4 oPosH : SV_POSITION, out float4 oColor : COLOR) //vertex shader 정점 셰이더
{
	
	// Transform to homogeneous clip space.
	// 동차 절단 공간으로 변환한다.
	oPosH = mul(float4(iPos, 1.0f), gWorldViewProj); // mul(x,y): float4 x * float4x4 y, 벡터 대 행렬곱셈(여러 크기의 행렬 곱셈 가능), 
															 // float4()행렬 생성 부분은 w = 1로, 벡터가 아닌 점 생성임.
															 // gWorldViewProj은 상수 버퍼에 들어있는 것
	
	// Just pass vertex color into the pixel shader.
	// 정점 색상을 그대로 픽셀 셰이더에 전달한다.
    oColor = iColor;
}
// 픽셀 셰이더의 입력이 정점 셰이더의 출력과 정확히 일치함.
float4 PS(float4 posH : SV_POSITION, float4 color : COLOR) : SV_Target //pixel shader
{
    return color;
}
*/