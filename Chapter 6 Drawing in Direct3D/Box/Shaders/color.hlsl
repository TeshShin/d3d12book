//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
};

struct VertexIn // �Է� �Ű�����
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut // ��� �Ű�����
{
	float4 PosH  : SV_POSITION;	// ���� ���̴��� ������� �ʴ´ٸ�, ���� ���̴��� ����� �ݵ�� �ǹ̼Ұ� SV_POSITION��,
    float4 Color : COLOR;		// ���� ���� ���������� ���� ��ġ�̾���Ѵ�. ���ϼ��̴��� ���� �� �ϵ����� ���� ���̴���
								// ���� �������� ���� ���� ������ �ִٰ� �����ϱ� �����̴�.
};								// ���� ���̴��� ����ϴ� ��쿡�� ���� ���� ���� ��ġ�� ����� ���� ���̴��� �̷� �� �ִ�.
								// ���� ����������� �������� ���ƾ��Ѵ�. ���� ����� ���ϴ� �κи� �ϸ�ȴ�. ���� ������� �ϵ��� �˾Ƽ�
VertexOut VS(VertexIn vin)
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

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}


