//
// Tests for correct behavior of the library.
//

#include "test_utils.h"

#include <iface.h>
#include <memory>

#define LOGIC_has_member_fn(s, f, ...)                                         \
    std::is_invocable_v<decltype([](auto &&x) -> std::type_identity<decltype(  \
                                                  x.f(__VA_ARGS__))> {         \
                            return {};                                         \
                        }),                                                    \
                        s>

int main(int, char **argv)
{
    int nassertions = 0;

    //
    // Address table is indexed into correctly
    //
    {
        struct {
            int f() { return 1; }
            int g() { return 2; }
            int h() { return 3; }
        } s;
        IFACE((f, int())(g, int())(h, int())) lifted = s;
        ASSERT(lifted.f() == 1);
        ASSERT(lifted.g() == 2);
        ASSERT(lifted.h() == 3);
    }

    //
    // Const member functions and non-so are discerned appropriately
    //
    {
        struct S {
            int f() { return 1; }
            int f() const { return 2; }
        } s;
        IFACE((f, int())(f, int() const)) lifted = s;
        ASSERT(lifted.f() == 1);
        ASSERT([](const auto &x) { return x.f(); }(lifted) == 2);
    }

    //
    // Object address is passed correctly
    //
    {
        struct {
            intptr_t f() { return (intptr_t)(void *)this; }
        } s;
        ASSERT(IFACE((f, intptr_t())){s}.f() == s.f());
    }

    //
    // Object members are correctly accessible
    //
    {
        struct {
            int x{1}, y{2};
            int f() { return x; }
            int g() { return y; }
        } s;
        IFACE((f, int())(g, int())) lifted = s;
        ASSERT(lifted.f() == 1);
        ASSERT(lifted.g() == 2);
    }

    //
    // Reference semantics are respected
    //
    {
        struct {
            int x = 0;
            int f() { return ++x; }
        } s;
        using If = IFACE((f, int()));
        ASSERT(If{s}.f() == 1);
        ASSERT(If{s}.f() == 2);
        ASSERT(If{s}.f() == 3);
    }

    //
    // Promotions are applicable nestingly
    //
    {
        struct {
            int x = 0;
            int f() { return ++x; }
        } s;
        using If = IFACE((f, int()));
        ASSERT(If{s}.f() == 1);
        ASSERT(If{If{s}}.f() == 2);
        ASSERT(If{If{If{s}}}.f() == 3);
    }

    //
    // Virtual table is accessed into correctly for const interfaces
    //
    {
        struct {
            int f() const { return 1; }
            int g() const { return 2; }
            int h() const { return 3; }
        } s;
        static_assert(!iface::is_soo_apt<decltype((s))>::value);
        IFACE((f, int() const)(g, int() const)(h, int() const)) lifted = s;
        ASSERT(lifted.f() == 1);
        ASSERT(lifted.g() == 2);
        ASSERT(lifted.h() == 3);
    }

    //
    // Virtual table is accessed into correctly for SOO-interfaces
    //
    {
        struct S {
            int f() const { return 1; }
            int g() const { return 2; }
            int h() const { return 3; }
        };
        static_assert(iface::is_soo_apt<decltype((S{}))>::value);
        IFACE((f, int() const)(g, int() const)(h, int() const)) lifted = S{};
        ASSERT(lifted.f() == 1);
        ASSERT(lifted.g() == 2);
        ASSERT(lifted.h() == 3);
    }

    //
    // Object members are correctly accessible for SOO-instances
    //
    {
        struct S {
            int16_t x{1}, y{2};
            int f() const { return x; }
            int g() const { return y; }
        };
        static_assert(iface::is_soo_apt<decltype(S{})>::value);
        IFACE((f, int() const)(g, int() const)) lifted = S{};
        ASSERT(lifted.f() == 1);
        ASSERT(lifted.g() == 2);
    }

    //
    // Interface can be copy-constructed from another interface of same
    // functions+names
    //
    {
        struct S {
            intptr_t f() { return (intptr_t)(void *)this; }
        } s;
        IFACE((f, intptr_t())) lifted1 = s;
        IFACE((f, intptr_t())) lifted2 = lifted1;
        ASSERT(lifted1.f() == s.f());
        ASSERT(lifted2.f() == s.f());
    }

    //
    // A copy-constructed base will copy the obj and tbl if and only if
    // constructed-from base is a superset
    //
    {
#define LOGIC_ctor(N, ...)                                                     \
    using If##N = IFACE(__VA_ARGS__);                                          \
    struct S##N : If##N {                                                      \
        using If##N::If##N;                                                    \
        auto get_tbl_addr(std::size_t idx = 0)                                 \
        {                                                                      \
            return (const void *)&std::get<1>(*this)[idx];                     \
        }                                                                      \
        auto get_obj_addr() { return (const void *)std::get<0>(*this); }       \
    };

        LOGIC_ctor(1, (f, int())(g, int())(h, int()));
        LOGIC_ctor(2, (f, int())(g, int())(h, int()));
        LOGIC_ctor(3, (f, int())(g, int()));
        LOGIC_ctor(4, (g, int())(h, int()));
        LOGIC_ctor(5, (g, int())(f, int()));
        LOGIC_ctor(6, (f, int()));
        LOGIC_ctor(7, (g, int()));

        struct {
            int f() { return 0; }
            int g() { return 0; }
            int h() { return 0; }
        } s;

        S1 s1{s};
        S2 s2{s1}; // equal => copy reference
        S3 s3{s1}; // subset => copy reference
        S4 s4{s1}; // subset => copy reference
        S5 s5{s1}; // wrong order => employ glue
        S6 s6{s1}; // table of one => copy value
        S7 s7{s1}; // table of one => copy value

        ASSERT(s2.get_tbl_addr() == s1.get_tbl_addr());
        ASSERT(s2.get_obj_addr() == s1.get_obj_addr());
        ASSERT(s3.get_tbl_addr() == s1.get_tbl_addr());
        ASSERT(s3.get_obj_addr() == s1.get_obj_addr());
        ASSERT(s4.get_tbl_addr() == s1.get_tbl_addr(1));
        ASSERT(s4.get_obj_addr() == s1.get_obj_addr());
        ASSERT(s5.get_tbl_addr() != s1.get_tbl_addr());
        ASSERT(s5.get_obj_addr() != s1.get_obj_addr());
        ASSERT(s6.get_tbl_addr() != s1.get_tbl_addr());
        ASSERT(s6.get_obj_addr() == s1.get_obj_addr());
        ASSERT(s7.get_tbl_addr() != s1.get_tbl_addr());
        ASSERT(s7.get_obj_addr() == s1.get_obj_addr());
    }

    //
    // SOO-aptness depends on sizeof void*
    //
    {
        struct S {
            int64_t x;
            intptr_t f() const { return (intptr_t)(void *)this; }
        };
        static_assert(iface::is_soo_apt<S>::value ==
                      (sizeof(int64_t) <= sizeof(void *)));
    }

    //
    // No access violations arise when copied-from iface destructs and concerns
    //
    {
        struct S {
            int f() { return 0; }
            int g() { return 0; }
        };
        S s;

        const auto test = [&]<class T>() {
            return test_utils::catch_access_violation([&] {
                auto dst = [&]() -> T {
                    auto src = std::make_unique<IFACE((f, int())(g, int()))>(s);
                    return *src;
                }();
                if constexpr (LOGIC_has_member_fn(decltype((dst)), f))
                    ASSERT(dst.f() == 0);
                else
                    ASSERT(dst.g() == 0);
            });
        };
#define IFACE_access_test(...) test.template operator()<IFACE(__VA_ARGS__)>()

        // ... an interface superset
        const auto ok1 = IFACE_access_test((f, int())(g, int()));
        ASSERT_FALSE(ok1);
        const auto ok2 = IFACE_access_test((f, int()));
        ASSERT_FALSE(ok2);
        const auto ok3 = IFACE_access_test((g, int()));
        ASSERT_FALSE(ok3);
    }

    printf("%s: ",
           [&](auto x) { return x ? x + 1 : argv[0]; }(strrchr(argv[0], '\\')));
    printf("\u001b[32;1m%d assertion%s OK\u001b[0m\n", nassertions,
           nassertions == 1 ? "" : "s");

    return 0;
}