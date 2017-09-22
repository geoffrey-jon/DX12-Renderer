#include "RenderCylinder.h"

#include "Vertex.h"
#include "D3DUtil.h"
#include "DirectXColors.h"
#include "MathHelper.h"
#include "DxException.h"
#include "GeometryGenerator.h"

RenderCylinder::RenderCylinder(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList, UINT numFrames) : RenderObject(device, cmdList, numFrames)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.3f, 0.3f, 3.0f, 20, 20);

	mVertexCount = cylinder.Vertices.size();
	mIndexCount = cylinder.Indices32.size();	

	vertices = new Vertex[mVertexCount];

	for (UINT i = 0; i < mVertexCount; ++i)
	{
		vertices[i].Pos = cylinder.Vertices[i].Position;
		vertices[i].Color = DirectX::XMFLOAT4(DirectX::Colors::Red);
	}

	indices = cylinder.GetIndices16().data();

	Init(device, cmdList, vertices, indices);
}

RenderCylinder::~RenderCylinder()
{
}
