/*  ===================================
	Direct3D 12 Utility Implementations
	===================================  */

#include "D3DUtil.h"

void CompileShader(LPCWSTR filename, LPCSTR entrypoint, LPCSTR version, ComPtr<ID3DBlob>& code)
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
