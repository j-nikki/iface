#include <array>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/tuple/rem.hpp>
#include <tuple>
#include <utility>

namespace iface
{

#ifdef _MSC_VER
#define IFACE_INLINE __forceinline inline
#else
#define IFACE_INLINE inline
#endif

namespace detail
{

//
// Iface_base contains reference to the vtable - an array of (opaque) pointers
// that each point to functions of an implementing class. It also contains
// reference to an object of that class.
//

template <class Tbl, class Token, class TblGetter>
struct Iface_base : protected std::tuple<const Tbl &, void *> {
    // I resorted to tuple for data storage due to earlier code generating
    // redundant movaps+movdqa at call site (alignment issues?)
  private:
    using base_type = std::tuple<const Tbl &, void *>;

    template <class T>
    static constexpr Tbl Table_for = TblGetter{}.template operator()<T>();

  public:
    constexpr IFACE_INLINE Iface_base(Token &&)
        : base_type{std::declval<Tbl &>(), nullptr}
    {
    }
    template <class T>
    constexpr IFACE_INLINE Iface_base(T &obj)
        : base_type{Table_for<std::remove_const_t<T>>, std::addressof(obj)}
    {
        static_assert(!std::is_const_v<T>,
                      "const objects not yet supported"); // another TODO: SOO
    }
};

//
// We can't form pointers directly into the implementing classes' member
// functions. For this reason we'll generate a glue function (glue<...>::fn) to
// point to.
//

template <class T>
using Fwd_t = std::conditional_t<std::is_lvalue_reference_v<T>, T,
                                 std::remove_reference_t<T>>;

template <class, class>
struct glue;
template <class R, class... Args, class Fn>
struct glue<R(Args...), Fn> {
    static R fn(void *object, Fwd_t<Args>... args)
    {
        return Fn{}(object, args...);
    }
};

#define IFACE_call(f)                                                          \
    []<class... Args>(void *obj, Args &&...args) noexcept(                     \
        noexcept(                                                              \
            reinterpret_cast<T *>(obj)->f(static_cast<Args &&>(args)...)))     \
        ->decltype(                                                            \
            reinterpret_cast<T *>(obj)->f(static_cast<Args &&>(args)...))      \
    {                                                                          \
        return reinterpret_cast<T *>(obj)->f(static_cast<Args &&>(args)...);   \
    }
#define IFACE_ptrget(r, _, i, x)                                               \
    BOOST_PP_COMMA_IF(i)                                                       \
    &::iface::detail::glue<BOOST_PP_TUPLE_REM() BOOST_PP_TUPLE_POP_FRONT(x),   \
                           decltype(IFACE_call(BOOST_PP_SEQ_HEAD(x)))>::fn

//
// Exposing the functions through a clean interface.
//

template <class...>
struct sig {
};

template <class R, class... Args>
struct sig<R(Args...)> : sig<sig<>, R, Args...> {
};

// Build often when modifying this macro - ICE-prone
#define IFACE_mem_fn(r, _, i, x)                                               \
    using BOOST_PP_CAT(Fn, i) = decltype(                                      \
        []<class Base, class R, class... Args>(                                \
            ::iface::detail::sig<::iface::detail::sig<>, R, Args...>) {        \
            struct Fn : Base {                                                 \
                using Base::Base;                                              \
                R IFACE_INLINE BOOST_PP_SEQ_HEAD(x)(Args && ...args) const     \
                {                                                              \
                    return reinterpret_cast<R (*)(                             \
                        void *, ::iface::detail::Fwd_t<Args>...)>(             \
                        ::std::get<0>(*this)[i])(                              \
                        ::std::get<1>(*this), static_cast<Args &&>(args)...);  \
                }                                                              \
            };                                                                 \
            return Fn{Token{}};                                                \
        }                                                                      \
            .template operator()<BOOST_PP_TUPLE_REM() BOOST_PP_IF(             \
                i, (BOOST_PP_CAT(Fn, BOOST_PP_DEC(i))),                        \
                (::iface::detail::Iface_base<Tbl, Token, TblGetter>))>(        \
                ::iface::detail::sig<BOOST_PP_TUPLE_REM()                      \
                                         BOOST_PP_TUPLE_POP_FRONT(x)>{}));

//
// All is brought together here. Lambdas in unevaluated contexts allow this
// heresy.
//

#define IFACE_impl(s)                                                          \
    decltype([] {                                                              \
        struct Token {                                                         \
        };                                                                     \
        using Tbl       = ::std::array<void *, BOOST_PP_SEQ_SIZE(s)>;          \
        using TblGetter = decltype([]<class T>() {                             \
            return Tbl{BOOST_PP_SEQ_FOR_EACH_I(IFACE_ptrget, _, s)};           \
        });                                                                    \
        BOOST_PP_SEQ_FOR_EACH_I(IFACE_mem_fn, _, s)                            \
        return BOOST_PP_CAT(Fn, BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(s))){Token{}};  \
    }())

} // namespace detail

#define IFACE(...) IFACE_impl(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__))

} // namespace iface