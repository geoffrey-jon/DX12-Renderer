#ifndef D3DUTIL_H
#define D3DUTIL_H

#include <winerror.h>
#include <windows.h>

#include "DirectXColors.h"

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const LPCTSTR& functionName, const LPCSTR& filename, int lineNumber);

	LPCTSTR ToString()const;

	HRESULT ErrorCode = S_OK;
	LPCTSTR FunctionName;
	LPCSTR Filename;
	int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) \
{                        \
    HRESULT hr__ = (x);  \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, __FILE__, __LINE__); } \
}
#endif

#endif // D3DUTIL_H