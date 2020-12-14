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
        IFACE((f, int() const)(g, int() const)(h, int() const)) lifted = s;
        ASSERT(lifted.f() == 1);
        ASSERT(lifted.g() == 2);
        ASSERT(lifted.h() == 3);
    }

    //
    // Virtual table is accessed into correctly for SOO-interfaces
    //
    {
        const struct {
            int f() const { return 1; }
            int g() const { return 2; }
            int h() const { return 3; }
        } s;
        static_assert(iface::is_soo_apt<decltype(s)>::value);
        IFACE((f, int() const)(g, int() const)(h, int() const)) lifted = s;
        ASSERT(lifted.f() == 1);
        ASSERT(lifted.g() == 2);
        ASSERT(lifted.h() == 3);
    }

    //
    // Object members are correctly accessible for SOO-instances
    //
    {
        const struct {
            int x{1}, y{2};
            int f() const { return x; }
            int g() const { return y; }
        } s;
        static_assert(iface::is_soo_apt<decltype(s)>::value);
        IFACE((f, int() const)(g, int() const)) lifted = s;
        ASSERT(lifted.f() == 1);
        ASSERT(lifted.g() == 2);
    }

    printf("%s: ",
           [&](auto x) { return x ? x + 1 : argv[0]; }(strrchr(argv[0], '\\')));
    printf("\u001b[32;1m%d assertion%s OK\u001b[0m\n", nassertions,
           nassertions == 1 ? "" : "s");

    return 0;
}