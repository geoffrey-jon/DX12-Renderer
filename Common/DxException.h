/*  =====================================
	Direct3D Exception Handler Definition
	=====================================  */

#ifndef DXEXCEPTION_H
#define DXEXCEPTION_H

#include <windows.h>
#include <winerror.h>

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const LPCTSTR& functionName, const LPCSTR& filename, int lineNumber);

	LPCTSTR ToString();

	HRESULT ErrorCode = S_OK;
	LPCTSTR FunctionName;
	LPCSTR Filename;
	int LineNumber = -1;

	wchar_t text[1024];
};

#endif // DXEXCEPTION_H