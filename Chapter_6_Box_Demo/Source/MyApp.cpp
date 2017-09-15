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
	mConstantBufferData = 0;
		
	mWorld = MathHelper::Identity4x4();
	mView = MathHelper::Identity4x4();
	mProj = MathHelper::Identity4x4();

	mTheta = 1.5f*DirectX::XM_PI;
	mPhi = DirectX::XM_PIDIV4;
	mRadius = 5.0f;
}

MyApp::~MyApp()
{
	if (mConstantBuffer != nullptr)
	{
		mConstantBuffer->Unmap(0, nullptr);
	}
	mConstantBufferData = nullptr;
}

bool MyApp::Init()
{
	if (D3DApp::Init() == false) { return false; }

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	// Initialize Static Geometry
	Vertex vertices[8] =
	{
		{ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) },
		{ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) },
		{ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) },
		{ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) },
		{ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) },
		{ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) },
		{ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) },
		{ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) },
	};

	UINT16 indices[36] =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	// Initialize Vertex Buffer
	InitializeStaticBuffer(mVertexBuffer, mVertexBufferUpload, &vertices, sizeof(Vertex) * 8);
	mVertexBuffer->SetName(L"Vertex Buffer");
	
	mVBView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVBView.SizeInBytes = sizeof(Vertex) * 8;
	mVBView.StrideInBytes = sizeof(Vertex);
	
	// Initialize Index Buffer	
	InitializeStaticBuffer(mIndexBuffer, mIndexBufferUpload, &indices, sizeof(UINT16) * 36);
	mIndexBuffer->SetName(L"Index Buffer");

	mIBView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIBView.Format = DXGI_FORMAT_R16_UINT;
	mIBView.SizeInBytes = sizeof(UINT16) * 36;

	// Create Descriptor Heap for Constant Buffers
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.NumDescriptors = 1;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NodeMask = 0;

	ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&mConstantBufferViewHeap)));
	mConstantBufferViewHeap->SetName(L"Constant Buffer View Heap");

	// Create a Constant Buffer and Map it
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(cb_sizeof(ObjectConstants)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(mConstantBuffer.GetAddressOf())
		));
	mConstantBuffer->SetName(L"Constant Buffer");

	ThrowIfFailed(mConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mConstantBufferData)));

	// Create a Descriptor for the Constant Buffer
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
	cbViewDesc.BufferLocation = mConstantBuffer->GetGPUVirtualAddress();
	cbViewDesc.SizeInBytes = cb_sizeof(ObjectConstants);

	mDevice->CreateConstantBufferView(&cbViewDesc, mConstantBufferViewHeap->GetCPUDescriptorHandleForHeapStart());

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

void MyApp::CompileShader(LPCWSTR filename, LPCSTR entrypoint, LPCSTR version, ComPtr<ID3DBlob>& code)
{
	UINT compileFlags = 0;
	#if defined(DEBUG) || defined(_DEBUG)  
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	#endif

	ComPtr<ID3DBlob> errors;
	HRESULT hr = D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint, version, compileFlags, 0, &code, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);
}

void MyApp::InitializeStaticBuffer(ComPtr<ID3D12Resource>& buffer, ComPtr<ID3D12Resource>& uploader, void* initData, UINT size)
{
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(buffer.GetAddressOf())
		));

	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploader.GetAddressOf())
		));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = size;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	UpdateSubresources<1>(mCommandList.Get(), buffer.Get(), uploader.Get(), 0, 0, 1, &subResourceData);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
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

	DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&mWorld);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX worldViewProj = world*view*proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProj));

	memcpy(mConstantBufferData, &objConstants, sizeof(ObjectConstants));
}

void MyApp::Draw(const GameTimer& gt) 
{
	ThrowIfFailed(mCommandAllocator->Reset());
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), mPSO.Get()));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),	D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VERTEX_BUFFER_VIEW vbViews[] = { mVBView };
	mCommandList->IASetVertexBuffers(0, 1, vbViews);

	mCommandList->IASetIndexBuffer(&mIBView);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mConstantBufferViewHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(mConstantBufferViewHeap->GetGPUDescriptorHandleForHeapStart());
//	cbv.Offset(0, 256);

	mCommandList->SetGraphicsRootDescriptorTable(0, cbv);

	mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % mNumSwapChainBuffers;

	FlushCommandQueue();
}