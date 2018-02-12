
## `su/miscs/signal.h`

Extremely simple signal class.

Usage:
```C++
// simple
su::signal<int> signalTakingAnInt;
signalTakingAnInt.connect( []( int i )
	{ std::cout << i << std::endl; } );
signalTakingAnInt( 45 ); // this will print '45'

// connection handling
su::signal<int> sig;
auto conn = sig.connect( []( int i )
	{ std::cout << i << std::endl; } );
sig( 45 ); // this will print '45'
sig.disconnect( conn );
sig( 45 ); // does nothing

// automatic connection handling
su::signal<int> sig;
auto conn = std::make_unique<su::signal<int>::scoped_conn>( sig, []( int i )
	{ std::cout << i << std::endl; } );
sig( 45 ); // this will print '45'
conn.reset();
sig( 45 ); // does nothing
```

There is nothing else to it, no thread safety, no return value from signals.

## `su/miscs/attachable.h`

This kind of implement a basis for the decorator pattern.
Subclasses of `su::attachable` can have arbitrary named
`su::attachments` attach to them and then retreive them by
name.

Usage:
```C++
class View : public su::attachable
{
  void draw()
  {
    if ( auto f = get<DrawFrame>( "frame" ) )
    {
      // I have a frame attached
      f->draw();
    }
	// ...
  }
};

View v;
v.attach( "frame", std::make_unique<DrawFrame>() );
```

## `su/miscs/version.h`

Helper to retrieve and compare version information at
runtime. You need to have the macro `PRODUCT_VERSION_FULL`
define when compiling as in:

	-DPRODUCT_VERSION_FULL=1,2,3,4

This would define version 1.2.3 build 4. Note that the
components are separated by **commas**.

You can also optionally define `GIT_REVISION` so it can be accessed as
`su::build_revision()`.

Usage:

`su::CURRENT_VERSION()` return the current version object.
From the version object you can access `major()`, `minor()`,
`patch()` and `build()` components to format it yourself or
get it as a formatted `full_string()` (1.2.3.4).

You can also create version object from strings
(`su::version::from_string("1.2.3.4")`) and compare version
objects (==, !=, <, >, etc).

## `su/miscs/mempool.h`

Overly simplistic memory pool allocation. It's a growing only
pool of trivially destructible type.

Usage:
```C++
su::mempool<32> mp;

auto p1 = mp.alloc<char>();
auto p2 = mp.alloc<TrivialType>();
auto p3 = mp.alloc<short>();
```
32 is the block size, everytime the pool runs out of memory,
it will allocate a block of that size. All blocks are
deallocated when the pool is destructed.

## `su/base/is_regular.h`

Defines a `is_regular` type traits. Regular type are  default constructible, copy constructible, move constructible, copy assignable and move assignable.

Usage:
```C++
struct S {};
static_assert(su::is_regular_v<S>, "huh?");
```

## `su/base/statesaver.h`

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


## `su/base/uuid.h`

Simple UUID class.

Usage:
```C++
su::uuid myId; // note: uninitialised!
myId = su::uuid::create(); // myId is a unique UUID
std::cout << myId.string() << std::endl;
```
