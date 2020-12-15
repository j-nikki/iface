#include <array>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/tuple/rem.hpp>
#include <tuple>
#include <utility>

namespace iface
{

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && _MSC_VER >= 1310
#define IFACE_INLINE inline __forceinline
#else
#define IFACE_INLINE inline
#endif

template <class T>
struct is_soo_apt
    : std::bool_constant<sizeof(T) <= sizeof(void *) &&
                         std::is_trivially_copy_constructible_v<T> &&
                         std::is_trivially_destructible_v<T> &&
                         !std::is_lvalue_reference_v<T>> {
};

namespace detail
{

//
// Opaque representation is void*. It can either hold an SOO-apt instance
// in-place or point to an instance of an implementing class.
//

struct opaque {
    constexpr IFACE_INLINE opaque(nullptr_t) noexcept : data_{nullptr} {}
#pragma warning(push)
#pragma warning(disable : 26495) // 'uninitialized member variable'
    template <class T>
    constexpr IFACE_INLINE opaque(T &&x) noexcept
    {
        if constexpr (is_soo_apt<T>::value) {
            new (&data_) std::remove_cvref_t<T>{static_cast<T &&>(x)};
        } else if constexpr (std::is_lvalue_reference_v<T>) {
            data_ = const_cast<void *>(
                reinterpret_cast<const void *>(std::addressof(x)));
        } else
            static_assert(false,
                          "move constructor is prohibited for this type");
    }
#pragma warning(pop)
    constexpr IFACE_INLINE operator void *() noexcept { return data_; }
    constexpr IFACE_INLINE operator const void *() const noexcept
    {
        return data_;
    }

  private:
    void *data_;
};

template <class To, class From>
constexpr IFACE_INLINE auto from_opaque(From &obj) noexcept
{
    static_assert(
        !is_soo_apt<To>::value || std::is_same_v<From, const void *>,
        "SOO instances aren't mutable; use only const interface member "
        "functions or define iface::is_soo_apt<...> : std::false_type {};");
    auto const ptr = (is_soo_apt<To>::value) ? std::addressof(obj) : obj;
    if constexpr (std::is_same_v<From, const void *>)
        return reinterpret_cast<const std::remove_reference_t<To> *>(ptr);
    else if constexpr (std::is_const_v<std::remove_reference_t<To>>)
        static_assert(
            false,
            "a const-qualified object cannot satisfy an interface with a "
            "non-const-qualified member function");
    else
        return reinterpret_cast<std::remove_reference_t<To> *>(ptr);
}

//
// Iface_base contains reference to the vtable - an array of (opaque) pointers
// that each point to functions of an implementing class. It also contains
// reference to an object of that class.
//

template <class Tbl, class Token, class TblGetter>
struct Iface_base : protected std::tuple<opaque, const Tbl &> {
    // I resorted to tuple for data storage due to earlier code generating
    // redundant movaps+movdqa at call site (alignment issues?)
  private:
    using base_type = std::tuple<opaque, const Tbl &>;

    template <class T>
    static constexpr Tbl Table_for = TblGetter{}.template operator()<T>();

  public:
    constexpr IFACE_INLINE Iface_base(Token &&) noexcept
        : base_type{nullptr, std::declval<Tbl &>()}
    {
    }
#pragma warning(push)
#pragma warning(disable : 4268) // 'object filled with zeroes'
    template <class T>
    constexpr IFACE_INLINE Iface_base(T &&obj) noexcept
        : base_type{static_cast<T &&>(obj), Table_for<T>}
    {
    }
#pragma warning(pop)
};

//
// Signature inspection facility
//

template <bool Const, class RetTy, class... Args>
struct sig {
};

template <class>
struct sig_impl;
template <class R, class... Args>
struct sig_impl<R(Args...)> {
    using type = sig<false, R, Args...>;
};
template <class R, class... Args>
struct sig_impl<R(Args...) const> {
    using type = sig<true, R, Args...>;
};

template <class T>
using sig_t = typename sig_impl<T>::type;

//
// We can't form pointers directly into the implementing classes' member
// functions. For this reason we'll generate a glue function (glue<...>::fn) to
// point to.
//

template <class T>
using fwd_t = std::conditional_t<std::is_lvalue_reference_v<T>, T,
                                 std::remove_reference_t<T>>;

template <class, class>
struct glue;
template <bool C, class R, class... Args, class Fn>
struct glue<sig<C, R, Args...>, Fn> {
    static R fn(std::conditional_t<C, const void *, void *> object,
                fwd_t<Args>... args)
    {
        return Fn{}(object, args...);
    }
};

#define IFACE_call(f)                                                          \
    []<class From, class... Args>(From & obj, Args && ...args) noexcept(       \
        noexcept(::iface::detail::from_opaque<T>(obj)->f(                      \
            static_cast<Args &&>(args)...)))                                   \
        ->decltype(::iface::detail::from_opaque<T>(obj)->f(                    \
            static_cast<Args &&>(args)...))                                    \
    {                                                                          \
        return ::iface::detail::from_opaque<T>(obj)->f(                        \
            static_cast<Args &&>(args)...);                                    \
    }
#define IFACE_ptrget(r, _, i, x)                                               \
    BOOST_PP_COMMA_IF(i)                                                       \
    &::iface::detail::glue<::iface::detail::sig_t<BOOST_PP_TUPLE_REM(          \
                               1) BOOST_PP_TUPLE_POP_FRONT(x)>,                \
                           decltype(                                           \
                               IFACE_call(BOOST_PP_TUPLE_ELEM(0, x)))>::fn

//
// Exposing the functions through a clean interface.
//

#define IFACE_mem_fn_ret(i, x, const_)                                         \
    struct Fn : Base {                                                         \
        using Base::Base;                                                      \
        R IFACE_INLINE BOOST_PP_TUPLE_ELEM(0, x)(Args && ...args)              \
            BOOST_PP_EXPR_IIF(const_, const)                                   \
        {                                                                      \
            return reinterpret_cast<R (*)(const void *,                        \
                                          ::iface::detail::fwd_t<Args>...)>(   \
                ::std::get<1>(*this)[i])(::std::get<0>(*this),                 \
                                         static_cast<Args &&>(args)...);       \
        }                                                                      \
    };                                                                         \
    return Fn{Token{}};

// Build often when modifying this macro - ICE-prone
#define IFACE_mem_fn(r, _, i, x)                                               \
    using BOOST_PP_CAT(Fn, i) = decltype(                                      \
        []<class Base, bool C, class R, class... Args>(                        \
            ::iface::detail::sig<C, R, Args...>) {                             \
            if constexpr (C) {                                                 \
                IFACE_mem_fn_ret(i, x, 1)                                      \
            } else {                                                           \
                IFACE_mem_fn_ret(i, x, 0)                                      \
            }                                                                  \
        }                                                                      \
            .template operator()<BOOST_PP_TUPLE_REM(1) BOOST_PP_IF(            \
                i, (BOOST_PP_CAT(Fn, BOOST_PP_DEC(i))),                        \
                (::iface::detail::Iface_base<Tbl, Token, TblGetter>))>(        \
                ::iface::detail::sig_t<BOOST_PP_TUPLE_ELEM(1, x)>{}));

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