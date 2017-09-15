/*  ===================================================
	Direct3D 12 Project-Specific Application Definition
	===================================================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"

#include "D3DCompiler.h"
#include "DirectXColors.h"

#include "MathHelper.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;

	static D3D12_INPUT_ELEMENT_DESC Layout[2];
};

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj;
};

class MyApp : public D3DApp
{
public:
	MyApp(HINSTANCE Instance);
	~MyApp();

	bool Init() override;

	void OnResize() override;
	void Update(const GameTimer& gt) override;
	void Draw(const GameTimer& gt) override;

private:
	void InitializeStaticBuffer(ComPtr<ID3D12Resource>& buffer, ComPtr<ID3D12Resource>& uploader, void* initData, UINT size);
	void CompileShader(LPCWSTR filename, LPCSTR entrypoint, LPCSTR version, ComPtr<ID3DBlob>& code);

private:
	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mVertexBufferUpload;

	ComPtr<ID3D12Resource> mIndexBuffer;
	ComPtr<ID3D12Resource> mIndexBufferUpload;

	D3D12_VERTEX_BUFFER_VIEW mVBView;
	D3D12_INDEX_BUFFER_VIEW mIBView;

	ComPtr<ID3D12Resource> mConstantBuffer;
	BYTE* mConstantBufferData;

	ComPtr<ID3D12DescriptorHeap> mConstantBufferViewHeap;

	ComPtr<ID3DBlob> mVSByteCode;
	ComPtr<ID3DBlob> mPSByteCode;

	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12PipelineState> mPSO;

	DirectX::XMFLOAT4X4 mProj;
	DirectX::XMFLOAT4X4 mWorld;
	DirectX::XMFLOAT4X4 mView;

	float mTheta;
	float mPhi;
	float mRadius;
};

#endif // MYAPP_H