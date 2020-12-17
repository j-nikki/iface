//
// Tests for syntax errors, missing headers, etc.
//

#include "iface.h"

int main()
{
    struct S {
        void f() const {}
    };
    IFACE((f, void() const)) lifted{S{}};
}