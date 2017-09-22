#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include <wrl.h>

#include "DirectXMath.h"
#include "d3d12.h"
#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

class RenderObject
{
public:
	RenderObject(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList, UINT numFrames);
	~RenderObject();

	void Init(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList, void* vertices, void* indices);
	void Draw(ComPtr<ID3D12GraphicsCommandList>& cmdList);

	DirectX::XMMATRIX GetWorldMatrix();

	void InitializeStaticBuffer(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList, ComPtr<ID3D12Resource>& buffer, ComPtr<ID3D12Resource>& uploader, void* initData, UINT size);

	void SetCBVHeapIndex(UINT index);
	UINT GetCBVHeapIndex();

	void Translate(float x, float y, float z);

protected:
	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mIndexBuffer;

	ComPtr<ID3D12Resource> mVertexBufferUploader;
	ComPtr<ID3D12Resource> mIndexBufferUploader;

	D3D12_VERTEX_BUFFER_VIEW mVBView;
	D3D12_INDEX_BUFFER_VIEW mIBView;

	D3D_PRIMITIVE_TOPOLOGY mTopology;

	UINT mVertexCount;
	UINT mIndexCount;

	DirectX::XMMATRIX mWorld;
	UINT mCBVHeapIndex;

public:
	UINT NumDirtyFrames;
};

#endif // RENDER_OBJECT_H