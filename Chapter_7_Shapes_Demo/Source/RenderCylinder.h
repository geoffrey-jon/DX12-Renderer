#ifndef RENDER_CYLINDER_H
#define RENDER_CYLINDER_H

#include "RenderObject.h"

using Microsoft::WRL::ComPtr;

class RenderCylinder : public RenderObject
{
public:
	RenderCylinder(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& cmdList, UINT numFrames);
	~RenderCylinder();

private:
};

#endif // RENDER_CYLINDER_H