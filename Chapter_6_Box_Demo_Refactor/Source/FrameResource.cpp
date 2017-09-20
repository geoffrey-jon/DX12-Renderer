#include "FrameResource.h"

FrameResource::FrameResource(ComPtr<ID3D12Device>& device, UINT numRenderObjects)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdAlloc.GetAddressOf())));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(cb_sizeof(ObjectConstants) * numRenderObjects),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(ConstantBuffer.GetAddressOf())
		));
	ConstantBuffer->SetName(L"Constant Buffer");

	ThrowIfFailed(ConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&ConstantBufferData)));

	FenceValue = 0;
}

FrameResource::~FrameResource()
{
	if (ConstantBuffer != nullptr)
	{
		ConstantBuffer->Unmap(0, nullptr);
	}
	ConstantBufferData = nullptr;
}