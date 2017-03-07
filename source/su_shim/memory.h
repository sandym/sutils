
#ifndef H_SU_SHIM_MEMORY
#define H_SU_SHIM_MEMORY

#include <memory>

namespace su {

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}

#endif
