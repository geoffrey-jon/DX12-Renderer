#include "RenderObject.h"
#include "Vertex.h"
#include "D3DUtil.h"
#include "DirectXColors.h"
#include "MathHelper.h"
#include "DxException.h"

RenderObject::RenderObject(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList)
{
	mTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mWorld = DirectX::XMMatrixIdentity();
	mCBVHeapIndex = 0;
}

RenderObject::~RenderObject()
{
}

void RenderObject::Init(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList, void* vertices, void* indices)
{
	// Initialize Vertex Buffer
	InitializeStaticBuffer(device, cmdList, mVertexBuffer, mVertexBufferUploader, vertices, sizeof(Vertex) * mVertexCount);
	mVertexBuffer->SetName(L"Vertex Buffer");

	mVBView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVBView.SizeInBytes = sizeof(Vertex) * mVertexCount;
	mVBView.StrideInBytes = sizeof(Vertex);

	// Initialize Index Buffer	
	InitializeStaticBuffer(device, cmdList, mIndexBuffer, mIndexBufferUploader, indices, sizeof(UINT16) * mIndexCount);
	mIndexBuffer->SetName(L"Index Buffer");

	mIBView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIBView.Format = DXGI_FORMAT_R16_UINT;
	mIBView.SizeInBytes = sizeof(UINT16) * mIndexCount;
}

void RenderObject::InitializeStaticBuffer(
	ComPtr<ID3D12Device>& device, 
	ComPtr<ID3D12GraphicsCommandList>& cmdList, 
	ComPtr<ID3D12Resource>& buffer, 
	ComPtr<ID3D12Resource>& uploader, 
	void* initData, UINT size)
{
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(buffer.GetAddressOf())
		));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploader.GetAddressOf())
		));

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = size;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	UpdateSubresources<1>(cmdList.Get(), buffer.Get(), uploader.Get(), 0, 0, 1, &subResourceData);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void RenderObject::Draw(ComPtr<ID3D12GraphicsCommandList>& cmdList)
{
	cmdList->IASetPrimitiveTopology(mTopology);

	D3D12_VERTEX_BUFFER_VIEW vbViews[] = { mVBView };
	cmdList->IASetVertexBuffers(0, 1, vbViews);

	cmdList->IASetIndexBuffer(&mIBView);

	cmdList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
}

DirectX::XMMATRIX RenderObject::GetWorldMatrix()
{
	return mWorld;
}

void RenderObject::Translate(float x, float y, float z)
{
	mWorld = DirectX::XMMatrixTranslation(x, y, z);
}

 void RenderObject::SetCBVHeapIndex(UINT index)
{ 
	mCBVHeapIndex = index; 
}

 UINT RenderObject::GetCBVHeapIndex()
 { 
	 return mCBVHeapIndex; 
 }