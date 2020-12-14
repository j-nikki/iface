#include <stdio.h>
#include <stdlib.h>
#include <string>

#define STRINGIFY_impl(x) #x
#define STRINGIFY(x) STRINGIFY_impl(x)

namespace test_utils::detail
{
static std::string error_msg;
template <class T>
struct rhs_catcher {
    std::conditional_t<std::is_lvalue_reference_v<T>, T, std::decay_t<T>> lhs;
    template <class U>
    bool operator==(U &&rhs)
    {
        if (lhs != static_cast<U &&>(rhs)) {
            (((error_msg = {}) += std::to_string(lhs)) += " == ") +=
                std::to_string(rhs);
            return false;
        }
        return true;
    }
};
struct lhs_catcher {
    template <class T>
    rhs_catcher<T> operator==(T &&x) const noexcept
    {
        return {static_cast<T &&>(x)};
    }
};
} // namespace test_utils::detail

#define ASSERT(expr)                                                           \
    ((!!(::test_utils::detail::lhs_catcher{} == expr))                         \
         ? (void)++nassertions                                                 \
         : (fprintf(                                                           \
                stderr,                                                        \
                "\u001b[31;1m%S:" STRINGIFY(__LINE__) ": " #expr               \
                                                      " (%s)\u001b[0m\n",      \
                [](auto x) {                                                   \
                    return x ? x + 1 : __FILEW__;                              \
                }(wcsrchr(__FILEW__, '\\')),                                   \
                ::test_utils::detail::error_msg.c_str()),                      \
            exit(1)))