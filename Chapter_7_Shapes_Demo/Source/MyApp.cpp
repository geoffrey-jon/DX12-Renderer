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
	mCamera = new GFirstPersonCamera();
	mCamera->SetPosition(0.0f, 2.0f, -5.0f);
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
	mRenderObjects[0] = new RenderCylinder(mDevice, mCommandList, NumFrames);
	mRenderObjects[1] = new RenderCube(mDevice, mCommandList, NumFrames);
	mRenderObjects[1]->Translate(2.0f, 0, 0);

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

	mCamera->SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
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

	// Control the camera.
	if (GetAsyncKeyState('W') & 0x8000)
	{
		mCamera->Walk(10.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		mCamera->Walk(-10.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		mCamera->Strafe(-10.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		mCamera->Strafe(10.0f*gt.DeltaTime());
	}

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

	CD3DX12_GPU_DESCRIPTOR_HANDLE frameCBV(mCBVHeap->GetGPUDescriptorHandleForHeapStart());
	UINT tmp = NumFrames*NumRenderObjects + mCurrentFrame;
	frameCBV.Offset(tmp, mCbvSrvUavDescriptorSize);

	mCommandList->SetGraphicsRootDescriptorTable(1, frameCBV);

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

	// Create Descriptor Heap for Constants Buffers

	// Need one CBV per object, per frame resource
	UINT numCBVs = (NumRenderObjects + 1) * NumFrames;

	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.NumDescriptors = numCBVs;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&mCBVHeap)));

	// Create one CBV per Render Object per Frame
	for (UINT frameIndex = 0; frameIndex < NumFrames; ++frameIndex)
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mFrameResources[frameIndex]->ObjectCB->GetGPUVirtualAddress();
		for (UINT objIndex = 0; objIndex < NumRenderObjects; ++objIndex)
		{
			UINT cbSize = cb_sizeof(ObjectConstants);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
			cbViewDesc.BufferLocation = cbAddress + (objIndex * cbSize); // Object offset into CB
			cbViewDesc.SizeInBytes = cbSize;

			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCBVHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(frameIndex*NumRenderObjects + objIndex, mCbvSrvUavDescriptorSize);

			mDevice->CreateConstantBufferView(&cbViewDesc, handle);

			mRenderObjects[objIndex]->SetCBVHeapIndex(objIndex);
		}
	}

	for (UINT frameIndex = 0; frameIndex < NumFrames; ++frameIndex)
	{
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mFrameResources[frameIndex]->FrameCB->GetGPUVirtualAddress();
		UINT cbSize = cb_sizeof(FrameConstants);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
		cbViewDesc.BufferLocation = cbAddress;
		cbViewDesc.SizeInBytes = cbSize;

		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCBVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(NumFrames*NumRenderObjects + frameIndex, mCbvSrvUavDescriptorSize);

		mDevice->CreateConstantBufferView(&cbViewDesc, handle);
	}

	// Initialize Root Signature
	CD3DX12_ROOT_PARAMETER rootParameters[2];

	CD3DX12_DESCRIPTOR_RANGE objCBDescTable;
	objCBDescTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	rootParameters[0].InitAsDescriptorTable(1, &objCBDescTable);

	CD3DX12_DESCRIPTOR_RANGE frameCBDescTable;
	frameCBDescTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	rootParameters[1].InitAsDescriptorTable(1, &frameCBDescTable);

	CompileRootSignature(rootParameters, 2, mRootSignature);

	// Initialize PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { Vertex::Layout, Vertex::NumElements };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = { mVSByteCode->GetBufferPointer(), mVSByteCode->GetBufferSize() };
	psoDesc.PS = { mPSByteCode->GetBufferPointer(), mPSByteCode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
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
	mCamera->UpdateViewMatrix();

	FrameConstants frameConstants;
	XMStoreFloat4x4(&frameConstants.ViewProj, DirectX::XMMatrixTranspose(mCamera->ViewProj()));
	memcpy(mFrameResources[mCurrentFrame]->FrameCBData, &frameConstants, sizeof(FrameConstants));

	for (UINT i = 0; i < NumRenderObjects; ++i)
	{
		RenderObject* obj = mRenderObjects[i];
		if (obj->NumDirtyFrames > 0)
		{
			DirectX::XMMATRIX world = obj->GetWorldMatrix();

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));

			BYTE* dest = mFrameResources[mCurrentFrame]->ObjectCBData;
			memcpy(&dest[obj->GetCBVHeapIndex()*cb_sizeof(ObjectConstants)], &objConstants, sizeof(ObjectConstants));
			obj->NumDirtyFrames--;
		}
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