#ifndef MYAPP_VERTEX_H
#define MYAPP_VERTEX_H

#include "DirectXMath.h"
#include "d3d12.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;

	static const UINT NumElements = 2;
	static D3D12_INPUT_ELEMENT_DESC Layout[NumElements];
};

#endif // MYAPP_VERTEX_H