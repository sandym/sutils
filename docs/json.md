## `su_json.h`

The dropbox json11 lib (https://github.com/dropbox/json11)
with a lot of
optimisations and some extensions:
- small object optimisation whenever possible:
	- numbers and bool are inline instead of allocating
	memory for them. 
- handle `int64_t`
- conversion accessors:
	- `int_value()` would fail on a "123" string,
	`to_int_value()` will return an int with value 123.

Overall, uses A LOT less memory than json11 and is MUCH
faster. In all cases.

It's also more conformant to the json specs and uses some
parts of rapidjson (https://github.com/Tencent/rapidjson) to achive this (namely the number parsing routine).

[conformance result](https://sandym.github.io/docs/json_results/conformance.html)

[performance result](json_results/https://sandym.github.io/docs/json_results/performance_Corei7-4850HQ@2.30GHz_mac64_clang10.0.html)
