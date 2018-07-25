## `su_string_utils.h`

Various utilities, mostly built to work in utf8
- utf conversions: to_string(), to_wstring(), to_u16string()
- case conversion tolower(), toupper()
- case insensitive compare: compare_nocase()
- locale-aware compare: ucompare() and ucompare_nocase()
- number-aware compare: ucompare_numerically(),
where "a1" < "a5" < "a10"
- split(), split_view() and join()
- trim_spaces(), trim_spaces_view()
- starts_with(), ends_with() and _nocae variants

## `su_format.h`

Typesafe, variable template argument based, printf-like
string formatting.
Support:
- All of printf formatting
- The posix extension for localisation, (re-ordering arguments).
- An extra format character, '@' that auto detect the type and print anything
that has a to_string() function defined for it.
- The %s format character can take std::string and string_view as well as string literal.
- Will throw an exception if format string and argument type
does not match.

Usage 1, re-ordering argument:
```C++
auto v = "STRING";
auto s = su::format("V=%3$s, d=%2$f, i=%1$d", 1, 6.3, v);
```
Will return "V=STRING, d=6.3, i=1".

Usage 2, custom type:
```C++
struct Foo { ... };
std::string to_string( const Foo &v ) { return ...; }

Foo foo;
auto s = su::format("%@\n", foo); // will call to_string for Foo

```

@todo: compare with other libs
