

## `su_platform.h`

Defines the following macros to 1 or 0 according to
each platforms.

	- UPLATFORM_MAC
	- UPLATFORM_IOS
	- UPLATFORM_WIN
	- UPLATFORM_LINUX
	- UPLATFORM_UNIX
	- UPLATFORM_64BIT

Usage:
```C++
#if UPLATFORM_UNIX
	// do unix stuff
#else
	// do other stuff
#endif
```

## `su_always_inline.h`

Defines the macros `always_inline_func` and `never_inline_func` to annotate functions.

Usage:
```C++
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

Defines the following enum, where `su::endian::native` equals `su::endian::big` or `su::endian::little`, depending on the platform.

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

Also define the following template function that can be used for any native types (`short`, `int`, `long` & `long long`).

- `T little_to_native( const T & )`
- `T native_to_little( const T & )`
- `T big_to_native( const T & )`
- `T native_to_big( const T & )`

## `su_is_regular.h`

Defines a `is_regular` type traits. Regular type are  default constructible, copy constructible, move constructible, copy assignable and move assignable.

Usage:
```C++
struct S {};
static_assert(is_regular_v<S>, "huh?");
```

## `su_statesaver.h`

Simple RAAI template that set and restore a value.

Usage:
```C++
int i = 2;
{
	su::statesaver st( i, 3 );
	// here i == 3
}
// st went out-of-scope i was restored to 2

```

## `cfauto.h`

Simple RAII template wrapper for CoreFoundation types (macOS).

Usage:
```C++
cfauto<CFStringRef> desc( CFErrorCopyDescription( error ) );
// desc will release the string when it goes out-of-scope
```

## `su_stackarray.h`

Stack array that will automatically move to heap allocation if more space than expected is needed.  Stack allocation is fast but size must be known at compile time, heap allocation is slow but scalable at runtime. This is a trade off allocation strategy, will use the fast path in case where the needed size is within preallocated range and use the slow path if more memory is needed. Fast in most case, but safe and scalable.

Usage:
```C++
su::stackarray<int,256> data( nb );
// if nb was smaller or equal to 256, data will refer to
// a stack buffer, otherwise it will be heap allocated
// and automatically deallocated.
```

## `su_uuid.h`

Simple UUID class.

Usage:
```C++
auto myId = su::uuid::create();
// myId is a unique UUID
```
