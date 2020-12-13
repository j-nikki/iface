#include "test_utils.h"
#include <iface.h>

int main()
{
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
    // Object members are normally accessible
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
}