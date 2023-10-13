//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
//�ٸ��� ǥ���ϴ� ����� �ִ� �� �̴� 258p ����.
cbuffer cbPerObject : register(b0) // cbPerObject��� cbuffer ��ü(��� ����)�� �����Ѵ�.
{
	float4x4 gWorldViewProj; // �� ���� ���� �������� ���� ���� �������� ��ȯ�ϴ� �� ���̴� ���� ���, �þ� ���, ���� ����� �ϳ��� ������ ���̴�.
};

struct VertexIn // �Է� �Ű�����
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut // ��� �Ű�����
{
	float4 PosH  : SV_POSITION;	// ���� ���̴��� ������� �ʴ´ٸ�, ���� ���̴��� ����� �ݵ�� �ǹ̼Ұ� SV_POSITION��, (SV: system value,�ý��� ��)
    float4 Color : COLOR;		// ���� ���� ���������� ���� ��ġ�̾���Ѵ�. ���ϼ��̴��� ���� �� �ϵ����� ���� ���̴���
								// ���� �������� ���� ���� ������ �ִٰ� �����ϱ� �����̴�.
};								// ���� ���̴��� ����ϴ� ��쿡�� ���� ���� ���� ��ġ�� ����� ���� ���̴��� �̷� �� �ִ�.
								// ���� ����������� �������� ���ƾ��Ѵ�. ���� ����� ���ϴ� �κи� �ϸ�ȴ�. ���� ������� �ϵ��� �˾Ƽ�
VertexOut VS(VertexIn vin) //vertex shader ���� ���̴�
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	// ���� ���� �������� ��ȯ�Ѵ�.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj); // mul(x,y): float4 x * float4x4 y, ���� �� ��İ���(���� ũ���� ��� ���� ����), 
															 // float4()��� ���� �κ��� w = 1��, ���Ͱ� �ƴ� �� ������.
															 // gWorldViewProj�� ��� ���ۿ� ����ִ� ��
	
	// Just pass vertex color into the pixel shader.
	// ���� ������ �״�� �ȼ� ���̴��� �����Ѵ�.
    vout.Color = vin.Color;
    
    return vout;
}
// �ȼ� ���̴��� �Է��� ���� ���̴��� ��°� ��Ȯ�� ��ġ��.
float4 PS(VertexOut pin) : SV_Target //pixel shader
{
    return pin.Color;
}

// �Ʒ��� ���� �����̰� ���ĸ� ���� �ٸ���.
/*
	cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
};

// ����� ����ü�� ������� �ʴ´�.

VertexOut VS(float3 iPos : GetRenderTargetSamplePosition, float4 iColor : COLOR, out float4 oPosH : SV_POSITION, out float4 oColor : COLOR) //vertex shader ���� ���̴�
{
	
	// Transform to homogeneous clip space.
	// ���� ���� �������� ��ȯ�Ѵ�.
	oPosH = mul(float4(iPos, 1.0f), gWorldViewProj); // mul(x,y): float4 x * float4x4 y, ���� �� ��İ���(���� ũ���� ��� ���� ����), 
															 // float4()��� ���� �κ��� w = 1��, ���Ͱ� �ƴ� �� ������.
															 // gWorldViewProj�� ��� ���ۿ� ����ִ� ��
	
	// Just pass vertex color into the pixel shader.
	// ���� ������ �״�� �ȼ� ���̴��� �����Ѵ�.
    oColor = iColor;
}
// �ȼ� ���̴��� �Է��� ���� ���̴��� ��°� ��Ȯ�� ��ġ��.
float4 PS(float4 posH : SV_POSITION, float4 color : COLOR) : SV_Target //pixel shader
{
    return color;
}
*/