/*  ======================================
	Basic Vertex Shader with Vertex Colors
	====================================== */

struct VertexIn
{
	float3 Pos : POSITION;
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

cbuffer CBPerObject : register(b0)
{
    float4x4 gWorldViewProj;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = mul(float4(vin.Pos, 1.0f), gWorldViewProj);
    vout.Color = vin.Color;

	return vout;
}