## `su/json/json.h`

The dropbox json11 lib with a lot of optimisations and some extensions:
- small object optimisation whenever possible:
	- numbers and bool are inline instead of allocating memory for them
- handle int64
- conversion accessors:
	- int_value() would fail on a "123" string, to_int_value() will return an int with value 123.

Overall, uses A LOT less memory than json11 and is MUCH faster. In all cases.

## `su/json/bson`
@todo

## `su/json/smile`
@todo

## `su/json/ubjson`
@todo

## `su/json/MessagePack`
@todo

## `su/json/flat`
@todo

