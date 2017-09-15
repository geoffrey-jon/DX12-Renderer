/*  ==========================================
	Direct3D 12 Generic Application Definition
	==========================================  */

#ifndef D3DAPP_H
#define D3DAPP_H

#include <Windows.h>
#include <WindowsX.h>

#include <assert.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <wchar.h>

#include "d3d12.h"
#include "d3dx12.h"

#include "D3DUtil.h"
#include "DxException.h"

#include "GameTimer.h"

using Microsoft::WRL::ComPtr;

class D3DApp
{
public:
	D3DApp(HINSTANCE Instance);
	~D3DApp();

	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual bool Init();
	virtual void OnResize();

	bool Run();

	bool InitMainWindow();
	bool InitDirect3D();

	void CreateCommandObjects();
	void CreateSwapChain();
	void CreateRtvAndDsvDescriptorHeaps();

	void FlushCommandQueue();
	void CalculateFrameStats();

	inline float AspectRatio() { return (float)mClientWidth / mClientHeight; }

	virtual void Update(const GameTimer& gt) = 0;
	virtual void Draw(const GameTimer& gt) = 0;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

	static D3DApp* GetApp();

	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

protected:
	// D3DApp is a singleton, only one instance is allowed
	static D3DApp* mApp;
	HINSTANCE mAppInstance;

	HWND mMainWindow;
	LPCTSTR mMainWindowCaption;

	ComPtr<IDXGIFactory4> mDXGIFactory;
	ComPtr<ID3D12Device> mDevice;

	ComPtr<IDXGISwapChain> mSwapChain;
	ComPtr<ID3D12Fence> mFence;

	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	ComPtr<ID3D12DescriptorHeap> mRTVHeap;
	ComPtr<ID3D12DescriptorHeap> mDSVHeap;

	UINT mCurrBackBuffer = 0;
	static const UINT mNumSwapChainBuffers = 2;
	ComPtr<ID3D12Resource> mSwapChainBuffer[mNumSwapChainBuffers];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;
	
	UINT mCurrentFence = 0;

	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	int mClientWidth;
	int mClientHeight;

	GameTimer mTimer;

	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled
};

#endif // D3DAPP_H
