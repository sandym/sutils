

## `su_platform.h`

Defines the following macros to 1 or 0 according to
each platforms.

	- UPLATFORM_MAC (macOS)
	- UPLATFORM_IOS (iOS)
	- UPLATFORM_WIN (Windows)
	- UPLATFORM_LINUX (Linux)
	- UPLATFORM_UNIX (any unix)
	- UPLATFORM_64BIT (compiling as 64 bits)

Usage:
```C++
#include "su_platform.h"

#if UPLATFORM_UNIX
	// do unix stuff
#else
	// do other stuff
#endif
```

## `su_always_inline.h`

Defines the macros `always_inline_func` and `never_inline_func` to annotate
functions.

Usage:
```C++
#include "su_always_inline.h"

// compiler will always attempt to inline
always_inline_func int incFunc( int i )
{
	return i + 1;
}
// compiler will not inline
template<typename T>
T never_inline_func incFunc2( T i )
{
	return i + 1;
}
```

## `su_endian.h`

Defines the following enum, where `su::endian::native` equals `su::endian::big`
or `su::endian::little`, depending on the platform.

```C++
namespace su {
enum class endian
{
 little,
 big,
 native // little or big, according to the current architecture
};
}
```

Also define the following template function that can be used for any native
types (`short`, `int`, `long` & `long long`).

- `T little_to_native( const T & )`
- `T native_to_little( const T & )`
- `T big_to_native( const T & )`
- `T native_to_big( const T & )`

## `cfauto.h`

Simple RAII template wrapper for CoreFoundation types (iOS & macOS).

Usage:
```C++
cfauto<CFStringRef> desc( CFErrorCopyDescription( error ) );
// desc will release the string when it goes out-of-scope
```

