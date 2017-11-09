## `su/streams/null_stream.h`

Defines a `basic_nullbuf` and `basic_null_stream`. streambuf
and stream that discard their output (like redirecting to
/dev/null).

Usage:
```C++
su::null_stream devnull;
devnull << "will be lost" << std::endl;
```

## `su/streams/teebuf.h`

Defines a tee streambuf, a streambuf that redirect to 2 sub
streambuf (a tee!). So you can redirect one output to, let say, a file and
std::cout.

Usage:
```C++
std::ostringstream output1, output2;
su::teebuf tb( output1.rdbuf(), output2.rdbuf() );
std::ostream ostr( &tb );
ostr << 123 << " allo" << std::endl;
```

This will write "123 allo\n" to both streams, output1
**and** output2.

## `su/streams/membuf.h`

Defines a streambuf that takes raw memory as input, does
**not** copy the input for efficiency, so be carefull.

Usage:
```C++
char buffer[len];
// ...
su::membuf buf( buffer, buffer + len );
std::istream istr( &buf );
// reading from istr will read from the memory buffer
```
