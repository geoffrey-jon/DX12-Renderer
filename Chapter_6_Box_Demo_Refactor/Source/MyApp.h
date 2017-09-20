/*  ===================================================
	Direct3D 12 Project-Specific Application Definition
	===================================================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"

#include "D3DCompiler.h"

#include "MathHelper.h"
#include "RenderObject.h"
#include "RenderCube.h"
#include "Vertex.h"
#include "FrameResource.h"
#include "DirectXColors.h"

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
	static const UINT NumFrames = 3;
	static const UINT NumRenderObjects = 2;

	FrameResource* mFrameResources[NumFrames];
	UINT mCurrentFrame = 0;

	ComPtr<ID3D12DescriptorHeap> mCBVHeap;

	// Shaders
	ComPtr<ID3DBlob> mVSByteCode;
	ComPtr<ID3DBlob> mPSByteCode;

	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12PipelineState> mPSO;

	// Camera Settings
	DirectX::XMFLOAT4X4 mProj;
	DirectX::XMFLOAT4X4 mView;

	float mTheta;
	float mPhi;
	float mRadius;

	// Render Objects
	RenderObject* mRenderObjects[NumRenderObjects];
};

#endif // MYAPP_H