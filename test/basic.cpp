#include "test_utils.h"
#include <iface.h>

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
    // signatures+names
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
    // A base copy-constructed from a base that compares equal will use the same
    // obj and tbl
    //
    {
        using If1 = IFACE((f, int())(g, int()));
        struct S1 : If1 {
            using If1::If1;
            const void *get_tbl_addr() { return std::get<1>(*this).data(); }
            void *get_obj_addr() { return std::get<0>(*this); }
        };

        using If2 = IFACE((f, int())(g, int()));
        struct S2 : If2 { // compares equal to S1
            using If2::If2;
            const void *get_tbl_addr() { return std::get<1>(*this).data(); }
            void *get_obj_addr() { return std::get<0>(*this); }
        };

        using If3 = IFACE((f, int()));
        struct S3 : If3 { // compares not equal to S1
            using If3::If3;
            const void *get_tbl_addr() { return std::get<1>(*this).data(); }
            void *get_obj_addr() { return std::get<0>(*this); }
        };

        struct S {
            int f() { return 0; }
            int g() { return 0; }
        } s;
        S1 s1{s};
        S2 s2{s1};
        S3 s3{s1};
        ASSERT(s2.get_tbl_addr() == s1.get_tbl_addr());
        ASSERT(s2.get_obj_addr() == s1.get_obj_addr());
        ASSERT(s3.get_tbl_addr() != s1.get_tbl_addr());
        ASSERT(s3.get_obj_addr() != s1.get_obj_addr());
    }

    //
    // SOO-aptness depends on sizeof void*
    //
    {
        const struct S {
            int64_t x;
            intptr_t f() const { return (intptr_t)(void *)this; }
        } s{42};
        ASSERT((IFACE((f, intptr_t() const)){s}.f() == s.f()) ==
               (sizeof(intptr_t) <= sizeof(void *)));
    }

    //
    // equal_base_as works correctly
    //
    {
        // these trigger 2057: expected constant expression (?)
        // using iface::detail::equal_base_as;
        // using If = IFACE((f, int()));
        //
        //// Same name and signature
        // static_assert(equal_base_as<If, IFACE((f, int()))>);
        //
        //// Different name
        // static_assert(!equal_base_as<If, IFACE((g, int()))>);
        //
        //// Different signature
        // static_assert(!equal_base_as<If, IFACE((f, float()))>);
        //
        //// Different constness
        // static_assert(!equal_base_as<If, IFACE((f, int() const))>);
        //
        //// Same order
        // static_assert(equal_base_as<IFACE((f, int())(g, int())),
        //                            IFACE((f, int())(g, int()))>);
        //
        //// Wrong order
        // static_assert(!equal_base_as<IFACE((f, int())(g, int())),
        //                             IFACE((g, int())(f, int()))>);
    }

    printf("%s: ",
           [&](auto x) { return x ? x + 1 : argv[0]; }(strrchr(argv[0], '\\')));
    printf("\u001b[32;1m%d assertion%s OK\u001b[0m\n", nassertions,
           nassertions == 1 ? "" : "s");

    return 0;
}