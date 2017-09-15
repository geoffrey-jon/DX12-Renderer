/*  =========================================
	Direct3D Exception Handler Implementation
	=========================================  */

#include "DxException.h"
#include <comdef.h>

DxException::DxException(HRESULT hr, const LPCTSTR& functionName, const LPCSTR& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}

LPCWSTR DxException::ToString()
{
	// Get the string description of the error code.
	_com_error err(ErrorCode);
	LPCWSTR msg = err.ErrorMessage();

	_snwprintf_s(text, 1024, L"%s failed in %S; line %d; error: %s", FunctionName, Filename, LineNumber, msg);

	return text;
}
