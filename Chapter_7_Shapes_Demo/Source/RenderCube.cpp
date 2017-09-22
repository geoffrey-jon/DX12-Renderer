#include "RenderCube.h"

#include "D3DUtil.h"
#include "DirectXColors.h"
#include "MathHelper.h"
#include "DxException.h"
#include "GeometryGenerator.h"

RenderCube::RenderCube(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList, UINT numFrames) : RenderObject(device, cmdList, numFrames)
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData cube = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);

	mVertexCount = cube.Vertices.size();
	mIndexCount = cube.Indices32.size();

	vertices = new Vertex[mVertexCount];

	for (UINT i = 0; i < mVertexCount; ++i)
	{
		vertices[i].Pos = cube.Vertices[i].Position;
		vertices[i].Color = DirectX::XMFLOAT4(DirectX::Colors::Black);
	}

	indices = cube.GetIndices16().data();

	Init(device, cmdList, vertices, indices);
}

RenderCube::~RenderCube()
{
}
