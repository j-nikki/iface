# iface

This project brings interfaces similar to Go's to C++. Thanks to [P0315R4](https://wg21.link/P0315R4), you can define an 'anonymous interface' inside a function signature, allowing for quite succinct interface declarations:

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
