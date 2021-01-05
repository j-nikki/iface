#include <stdio.h>
#include <stdlib.h>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define STRINGIFY_impl(x) #x
#define STRINGIFY(x) STRINGIFY_impl(x)

namespace test_utils
{
namespace detail
{
static std::string error_msg;
template <class T>
concept to_stringable = requires(const T &x)
{
    std::to_string(x);
};
template <class T>
struct rhs_catcher {
    std::conditional_t<std::is_lvalue_reference_v<T>, T, std::decay_t<T>> lhs;
    template <class U>
    bool operator==(U &&rhs)
    {
        auto to_string = []<class V>(const V &x) {
            if constexpr (to_stringable<V>)
                return std::to_string(x);
            else
                return "?";
        };
        if (lhs != static_cast<U &&>(rhs)) {
            (((error_msg = {}) += to_string(lhs)) += " == ") += to_string(rhs);
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
} // namespace detail

template <class F, class... Args>
inline bool catch_access_violation(F &&f, Args &&...args)
{
    __try {
        static_cast<F &&>(f)(static_cast<Args &&>(args)...);
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                    ? EXCEPTION_EXECUTE_HANDLER
                    : EXCEPTION_CONTINUE_SEARCH) {

        return true;
    }
    return false;
}

#define ASSERT(expr)                                                           \
    do {                                                                       \
        (!!(::test_utils::detail::lhs_catcher{} == expr))                      \
            ? (void)++nassertions                                              \
            : (fprintf(                                                        \
                   stderr,                                                     \
                   "\u001b[31;1m%S:" STRINGIFY(__LINE__) ": " #expr            \
                                                         " (%s)\u001b[0m\n",   \
                   [](auto x) {                                                \
                       return x ? x + 1 : __FILEW__;                           \
                   }(wcsrchr(__FILEW__, '\\')),                                \
                   ::test_utils::detail::error_msg.c_str()),                   \
               exit(1));                                                       \
    } while (0)

#define ASSERT_TRUE(expr) ASSERT(expr == true)
#define ASSERT_FALSE(expr) ASSERT(expr == false)

} // namespace test_utils