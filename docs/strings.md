## `su_string.h`

Various string conversions, string tolower, string toupper,
comparisons, split, join, trim spaces, starts with,
ends with, etc

## `su_string_format.h`

Typesafe, variable template argument based, printf-like
string formatting. Support the posix extension for
localisation (re-ordering arguments).

```C++
auto s = su::format("V=%3$@, d=%2$f, i=%1$d", 1, 6.3, v);
```

TODO: compare with other libs

## `su_string_load.h`

Simple string loading utility, for localisation.

```C++
auto s = su::string_load( "key", "table" );
```
