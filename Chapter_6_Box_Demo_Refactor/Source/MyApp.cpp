/*  =======================================================
	Direct3D 12 Project-Specific Application Implementation
	=======================================================  */

#include "MyApp.h"

D3D12_INPUT_ELEMENT_DESC Vertex::Layout[Vertex::NumElements] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

MyApp::MyApp(HINSTANCE Instance) : D3DApp(Instance)
{
	mView = MathHelper::Identity4x4();
	mProj = MathHelper::Identity4x4();

	mTheta = 1.5f*DirectX::XM_PI;
	mPhi = DirectX::XM_PIDIV4;
	mRadius = 5.0f;
}

MyApp::~MyApp()
{
	for (UINT i = 0; i < NumRenderObjects; ++i)
	{
		if (mRenderObjects[i] != nullptr)
		{
			delete mRenderObjects[i];
		}
	}
	for (UINT f = 0; f < NumFrames; ++f)
	{
		if (mFrameResources[f] != nullptr)
		{
			delete mFrameResources[f];
		}
	}
}

bool MyApp::Init()
{
	if (D3DApp::Init() == false) 
	{ 
		return false; 
	}

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mGlobalCommandAllocator.Get(), nullptr));

	// Create Render Objects
	mRenderObjects[0] = new RenderCube(mDevice, mCommandList);
	mRenderObjects[1] = new RenderCube(mDevice, mCommandList);

	mRenderObjects[0]->Translate(2, 2, 2);

	// Initialize frame resources
	for (UINT f = 0; f < NumFrames; ++f)
	{
		mFrameResources[f] = new FrameResource(mDevice, NumRenderObjects);
	}

	// Initialize Opaque Pass
	InitializeOpaquePass();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void MyApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	DirectX::XMStoreFloat4x4(&mProj, P);
}

void MyApp::Update(const GameTimer& gt) 
{
	// Increment Frame Index
	mCurrentFrame = (mCurrentFrame + 1) % NumFrames;

	// If the next frame is still in flight, wait for the GPU to finish
	if (mFence->GetCompletedValue() < mFrameResources[mCurrentFrame]->FenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mFrameResources[mCurrentFrame]->FenceValue, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	// Convert Spherical to Cartesian coordinates.
	float x = mRadius*sinf(mPhi)*cosf(mTheta);
	float z = mRadius*sinf(mPhi)*sinf(mTheta);
	float y = mRadius*cosf(mPhi);

	// Build the view matrix.
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&mView, view);

	UpdateOpaquePass();
}

void MyApp::Draw(const GameTimer& gt) 
{
	ThrowIfFailed(mFrameResources[mCurrentFrame]->CmdAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mFrameResources[mCurrentFrame]->CmdAlloc.Get(), mPSO.Get()));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),	D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCBVHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	for (UINT i = 0; i < NumRenderObjects; ++i)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(mCBVHeap->GetGPUDescriptorHandleForHeapStart());
		cbv.Offset(mRenderObjects[i]->GetCBVHeapIndex(), mCbvSrvUavDescriptorSize);

		mCommandList->SetGraphicsRootDescriptorTable(0, cbv);

		mRenderObjects[i]->Draw(mCommandList);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % mNumSwapChainBuffers;

	mFrameResources[mCurrentFrame]->FenceValue = ++mCurrentFence;

	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));
}

void MyApp::InitializeOpaquePass()
{
	// Compile Shaders
	CompileShader(L"Shaders/VertexShader.hlsl", "VS", "vs_5_0", mVSByteCode);
	CompileShader(L"Shaders/PixelShader.hlsl", "PS", "ps_5_0", mPSByteCode);

	// Create Descriptor Heap for ObjectConstants Buffers

	// Need one CBV per object, per frame resource
	UINT numCBVs = NumRenderObjects * NumFrames;
	UINT cbSize = cb_sizeof(ObjectConstants);

	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.NumDescriptors = numCBVs;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&mCBVHeap)));

	// Create one CBV per Render Object per Frame
	for (UINT frameIndex = 0; frameIndex < NumFrames; ++frameIndex)
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mFrameResources[frameIndex]->ConstantBuffer->GetGPUVirtualAddress();
		for (UINT objIndex = 0; objIndex < NumRenderObjects; ++objIndex)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
			cbViewDesc.BufferLocation = cbAddress + (objIndex * cbSize); // Object offset into CB
			cbViewDesc.SizeInBytes = cbSize;

			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCBVHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(frameIndex*NumRenderObjects + objIndex, mCbvSrvUavDescriptorSize);

			mDevice->CreateConstantBufferView(&cbViewDesc, handle);

			mRenderObjects[objIndex]->SetCBVHeapIndex(objIndex);
		}
	}

	// Initialize Root Signature
	CD3DX12_ROOT_PARAMETER rootParameters[1];

	CD3DX12_DESCRIPTOR_RANGE cbDescTable;
	cbDescTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	rootParameters[0].InitAsDescriptorTable(1, &cbDescTable);

	CompileRootSignature(rootParameters, 1, mRootSignature);

	// Initialize PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { Vertex::Layout, Vertex::NumElements };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = { mVSByteCode->GetBufferPointer(), mVSByteCode->GetBufferSize() };
	psoDesc.PS = { mPSByteCode->GetBufferPointer(), mPSByteCode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.DSVFormat = mDepthStencilFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;

	mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
}

void MyApp::UpdateOpaquePass()
{
	for (UINT i = 0; i < NumRenderObjects; ++i)
	{
		RenderObject* obj = mRenderObjects[i];

		DirectX::XMMATRIX world = obj->GetWorldMatrix();
		DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&mView);
		DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&mProj);
		DirectX::XMMATRIX worldViewProj = world*view*proj;

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj));

		BYTE* dest = mFrameResources[mCurrentFrame]->ConstantBufferData;
		memcpy(&dest[obj->GetCBVHeapIndex()*cb_sizeof(ObjectConstants)], &objConstants, sizeof(ObjectConstants));
	}
}

void MyApp::CompileRootSignature(D3D12_ROOT_PARAMETER* params, UINT numParams, ComPtr<ID3D12RootSignature>& rootSig)
{
	CD3DX12_ROOT_SIGNATURE_DESC desc(numParams, params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serialRootSig;
	ComPtr<ID3DBlob> errorBlob;
	D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serialRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	ThrowIfFailed(mDevice->CreateRootSignature(0, serialRootSig->GetBufferPointer(), serialRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSig)));
}