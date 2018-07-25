
## `su_signal.h`

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

## `su_is_regular.h`

Defines a `is_regular` type traits. Regular type are  default constructible, copy constructible, move constructible, copy assignable and move assignable.

Usage:
```C++
struct S {};
static_assert(su::is_regular_v<S>, "huh?");
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

## `su_uuid.h`

Simple UUID class.

Usage:
```C++
su::uuid myId; // note: uninitialised!
myId = su::uuid::create(); // myId is a unique UUID
std::cout << myId.string() << std::endl;
```
