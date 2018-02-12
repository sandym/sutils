# sutils
A collection of very random C++ utilities I used over the years. Some might
even be good and work as advertised!

This is mostly a re-write with modern c++ in mind for my own use. Most parts
need some C++17 support from the compiler. Just add src to your header search
path and add the .cpp you need, no config should be required for most part.
Or you can do a cmake `add_subdirectory` and use as a library

---

### Some docs:

* [Base](docs/base.md)	
* [Streams](docs/streams.md)
* [Containers](docs/containers.md)
* [Strings](docs/strings.md)
* [Logger](docs/log.md)
* [Concurrency](docs/concurrency.md)
* [JSON](docs/json.md)
* [XML / parser](docs/parsers.md)
* [Files](docs/files.md)
* [Tests](docs/tests.md)
* [Miscellaneous](docs/miscs.md)

---

#### Tested with:
- Xcode 9 (Apple LLVM version 9.0.0)
- Visual Studio 17
- gcc7
