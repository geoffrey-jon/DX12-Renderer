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
    float4x4 gWorld;
};

cbuffer CBPerFrame : register(b1)
{
    float4x4 gViewProj;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
 
    float4 posW = mul(float4(vin.Pos, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    vout.Color = vin.Color;

	return vout;
}