## `su_stackarray.h`

Stack array that will automatically move to heap allocation if more space is
needed.  Stack allocation is fast but size must be known at compile time,
heap allocation is slow but scalable at runtime. This is a trade off allocation
strategy, will use the fast path in case where the needed size is within
the preallocated range and use the slow path if more memory is needed. Fast
in most case, but safe and scalable.

Usage:
```C++
su::stackarray<int,256> data( nb );
// if nb was smaller or equal to 256, data will refer to
// a stack buffer, otherwise it will be heap allocated
// and automatically deallocated.
```

## `su_flat_map.h`

Drop in replacement for std::map, but use std::vector as
storage. Warning: iterators are vector's iterators.

```C++
su::flat_map<std::string,int> m;
```

## `su_flat_set.h`

Drop in replacement for std::set, but use std::vector as
storage. Warning: iterators are vector's iterators.

```C++
su::flat_set<std::string> m;
```

## `su_hash_combine.h`

Combine hash values.

