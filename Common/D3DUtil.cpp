#include "D3DUtil.h"

DxException::DxException(HRESULT hr, const LPCTSTR& functionName, const LPCSTR& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}