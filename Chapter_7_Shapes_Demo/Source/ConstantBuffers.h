#ifndef CONSTANT_BUFFERS_H
#define CONSTANT_BUFFERS_H

#include "DirectXMath.h"

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World;
};

struct FrameConstants
{
	DirectX::XMFLOAT4X4 ViewProj;
};

#endif // CONSTANT_BUFFERS_H