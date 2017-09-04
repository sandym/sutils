
## `su_signal.h`

Extremely simple signal class.

Usage:
```C++
su::signal<int> signalTakingAnInt;
signalTakingAnInt.connect( []( int i )
	{ std::cout << i << std::endl; } );
signalTakingAnInt( 45 ); // this will print '45'
```

## `su_attachable.h`

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

## `su_version.h`

Helper to retrieve and compare version information at
runtime. You need to have the macro `PRODUCT_VERSION_FULL`
define when compiling as in:

	-DPRODUCT_VERSION_FULL=1,2,3,4

This would define version 1.2.3 build 4. Note that the
components are separated by **commas**.

You can also optionally define `SVN_REVISION` and/or
`GIT_REVISION` so they can be accessed as
`su::build_revision()`.

Usage:

`su::CURRENT_VERSION()` return the current version object.
From the version object you can access `major()`, `minor()`,
`patch()` and `build()` components to format it yourself or
get it as a formatted `full_string()` (1.2.3.4).

You can also create version object from strings
(`su::version::from_string("1.2.3.4")`) and compare version
objects (==, !=, <, >, etc).

## `su_mempool.h`

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
