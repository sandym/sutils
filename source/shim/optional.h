#ifndef H_SHIM_OPTIONAL
#define H_SHIM_OPTIONAL

#include <experimental/optional>

namespace su {

//using bad_optional_access = std::experimental::bad_optional_access;

template<typename T>
using optional = std::experimental::optional<T>;

}

#endif
