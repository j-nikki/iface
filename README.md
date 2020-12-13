# iface

This project brings non-intrusive interfaces similar to Go's to C++. Leveraging [P0315R4](https://wg21.link/P0315R4), iface allows you declare an 'anonymous interface' inside a function signature:

```c++
#include <iface.h>
#include <stdio.h>

void foo(IFACE((speak, void())(walk, void())) animal) {
    animal.speak();
    animal.walk();
}

struct Dog {
    void speak() const noexcept { puts("woof"); }
    void walk() const noexcept { puts("dog lollops along"); }
};

struct Cat {
    void speak() const noexcept { puts("meow"); }
    void walk() const noexcept { puts("tabby won't be bothered"); }
};

int main() {
    Dog dog;
    Cat cat;
    foo(dog);
    foo(cat);
}
```

Things at the moment missing but on the agenda are SOO and support for const semantics, as well as settings up DevOps.
