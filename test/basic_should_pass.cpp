#include "test_utils.h"
#include <iface.h>

int main()
{
    //
    // Address table is indexed into correctly
    //
    {
        struct {
            int f() { return 0; }
            int g() { return 1; }
            int h() { return 2; }
        } s;
        IFACE((f, int())(g, int())(h, int())) lifted = s;
        ASSERT(lifted.f() == 0);
        ASSERT(lifted.g() == 1);
        ASSERT(lifted.h() == 2);
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