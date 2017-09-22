#ifndef RENDER_CUBE_H
#define RENDER_CUBE_H

#include "RenderObject.h"

using Microsoft::WRL::ComPtr;

class RenderCube : public RenderObject
{
public:
	RenderCube(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList, UINT numFrames);
	~RenderCube();

private:
};

#endif // RENDER_CUBE_H