/*  ===============================
	Direct3D 12 Utility Definitions
	===============================  */

#ifndef D3DUTIL_H
#define D3DUTIL_H

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

#endif // D3DUTIL_H