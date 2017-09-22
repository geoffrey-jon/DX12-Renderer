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
		IID_PPV_ARGS(ObjectCB.GetAddressOf())
		));

	ThrowIfFailed(ObjectCB->Map(0, nullptr, reinterpret_cast<void**>(&ObjectCBData)));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(cb_sizeof(FrameConstants)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(FrameCB.GetAddressOf())
		));

	ThrowIfFailed(FrameCB->Map(0, nullptr, reinterpret_cast<void**>(&FrameCBData)));

	FenceValue = 0;
}

FrameResource::~FrameResource()
{
	if (ObjectCB != nullptr)
	{
		ObjectCB->Unmap(0, nullptr);
	}
	ObjectCBData = nullptr;

	if (FrameCB != nullptr)
	{
		FrameCB->Unmap(0, nullptr);
	}
	FrameCBData = nullptr;
}