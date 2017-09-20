#ifndef MYAPP_VERTEX_H
#define MYAPP_VERTEX_H

#include "DirectXMath.h"
#include "d3d12.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;

	static D3D12_INPUT_ELEMENT_DESC Layout[2];
};

#endif // MYAPP_VERTEX_H