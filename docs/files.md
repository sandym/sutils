## `su_filepath.h`

Encapsulate a location on the file system and define some operations on it.
Simple path handling and file / folder operations.

## `su_resource_access.h`

Retrieve resources at runtime.

Usage:
```C++
auto fp = su::resource_access::get( "myimage.png" );
// fp is a path to myimage.png
```

On macOS, it uses bundle resources.
On other platform it looks in a folder named 'Resources' next to the executable.
