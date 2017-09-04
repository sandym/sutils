## `su_null_stream.h`

Defines a `basic_nullbuf` and `basic_null_stream`. streambuf
and stream that discard their output (think /dev/null).

## `su_teebuf.h`

Defines a tee streambuf, a streambuf that redirect to 2 sub
streambuf (a tee!). So you can redirect one output to, let say, a file and std::cout.

Usage:
```C++
std::ostringstream os1, os2;
su::teebuf tb( os1.rdbuf(), os2.rdbuf() );
std::ostream ostr( &tb );
ostr << 123 << " allo" << std::flush;
```

This will write "123 allo" to both os1 **and** os2.

## `su_membuf.h`

Defines a streambuf that takes raw memory as input, does
**not** copy the input for efficiency, so be carefull.

Usage:
```C++
su::membuf buf( buffer, bufferEnd );
std::istream istr( &buf );
// reading from istr will read from the buffer memory
```
