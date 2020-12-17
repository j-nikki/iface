//
// Tests for missing headers, syntax, etc.
//

#include "iface.h"

int main()
{
    struct S {
        void f() const {}
    };
    IFACE((f, void() const)) lifted{S{}};
}