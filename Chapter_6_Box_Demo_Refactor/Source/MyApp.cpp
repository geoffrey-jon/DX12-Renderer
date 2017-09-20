/*  =======================================================
	Direct3D 12 Project-Specific Application Implementation
	=======================================================  */

#include "MyApp.h"

D3D12_INPUT_ELEMENT_DESC Vertex::Layout[2] =
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
	mRenderObjects[0] = new RenderObject(mDevice, mCommandList);
	mRenderObjects[1] = new RenderObject(mDevice, mCommandList);

	mRenderObjects[0]->Translate(2, 2, 2);

	// Initialize frame resources
	for (UINT f = 0; f < NumFrames; ++f)
	{
		mFrameResources[f] = new FrameResource(mDevice, NumRenderObjects);
	}

	// Create Descriptor Heap for Constant Buffers
	UINT numCBVs = NumRenderObjects * NumFrames;

	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.NumDescriptors = numCBVs;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&mCBVHeap)));
	mCBVHeap->SetName(L"Constant Buffer View Heap");

	// Create one CBV per Render Object per Frame
	for (UINT f = 0; f < NumFrames; ++f)
	{
		for (UINT i = 0; i < NumRenderObjects; ++i)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mFrameResources[f]->ConstantBuffer->GetGPUVirtualAddress();
			cbViewDesc.BufferLocation = cbAddress + (i * cb_sizeof(ObjectConstants));
			cbViewDesc.SizeInBytes = cb_sizeof(ObjectConstants);

			mRenderObjects[i]->SetCBVHeapIndex(i);

			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCBVHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(f*NumRenderObjects + i, mCbvSrvUavDescriptorSize);

			mDevice->CreateConstantBufferView(&cbViewDesc, handle);
		}
	}

	// Initialize Root Signature
	CD3DX12_ROOT_PARAMETER rootParameters[1];

	CD3DX12_DESCRIPTOR_RANGE cbDescTable;
	cbDescTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	rootParameters[0].InitAsDescriptorTable(1, &cbDescTable);

	CD3DX12_ROOT_SIGNATURE_DESC mRootSigeDesc(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	
	ComPtr<ID3DBlob> serialRootSig;
	ComPtr<ID3DBlob> errorBlob;
	D3D12SerializeRootSignature(&mRootSigeDesc, D3D_ROOT_SIGNATURE_VERSION_1, serialRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	ThrowIfFailed(mDevice->CreateRootSignature(0, serialRootSig->GetBufferPointer(), serialRootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
	mRootSignature->SetName(L"Root Signature");

	// Compile Shaders
	CompileShader(L"Shaders/VertexShader.hlsl", "VS", "vs_5_0", mVSByteCode);
	CompileShader(L"Shaders/PixelShader.hlsl", "PS", "ps_5_0", mPSByteCode);

	// Initialize PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { Vertex::Layout, 2 };
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
	mPSO->SetName(L"Default PSO");

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
	mCurrentFrame = (mCurrentFrame + 1) % NumFrames;

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

	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&mProj);
	
	// Update all objects
	for (UINT i = 0; i < NumRenderObjects; ++i)
	{
		DirectX::XMMATRIX world = mRenderObjects[i]->GetWorldMatrix();
		DirectX::XMMATRIX worldViewProj = world*view*proj;

		ObjectConstants objConstants;
		XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj));
		
		BYTE* dest = mFrameResources[mCurrentFrame]->ConstantBufferData;
		memcpy(&dest[mRenderObjects[i]->GetCBVHeapIndex()*cb_sizeof(ObjectConstants)], &objConstants, sizeof(ObjectConstants));
	}
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