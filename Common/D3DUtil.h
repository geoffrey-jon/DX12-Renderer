/*  ===============================
	Direct3D 12 Utility Definitions
	===============================  */

#ifndef D3DUTIL_H
#define D3DUTIL_H

#include "D3DCompiler.h"
#include "DxException.h"

#include <wrl.h>

using Microsoft::WRL::ComPtr;

#ifndef cb_sizeof
#define cb_sizeof(x) ((sizeof(x) + 255) & ~255)
#endif

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) \
{                        \
    HRESULT hr__ = (x);  \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, __FILE__, __LINE__); } \
}
#endif

void CompileShader(LPCWSTR filename, LPCSTR entrypoint, LPCSTR version, ComPtr<ID3DBlob>& code);

#endif // D3DUTIL_H