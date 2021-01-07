# iface

This project brings anonymous, non-intrusive interfaces to C++. Leveraging [P0315R4](https://wg21.link/P0315R4), iface allows declaring an interface directly in a function signature - see example below. Currently tested on VS 2019 Preview only.

## Examples

An interface can be declared anywhere a `decltype` can appear.

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

Example of using iface at interface boundary. `iface_base` occupies 16B (`tbl` 8B + `obj` 8B) and is hence stored on stack, its address passed in `rcx`, as per the [x64 calling convention](https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-160).

```c++
int __declspec(noinline) foo(IFACE((f, int() const)) x) { return x.f(); }
//  mov         rax,qword ptr [rcx]        ; f (tbl SBO)
//  mov         rcx,qword ptr [rcx+8]      ; obj
//  jmp         rax

struct S { int f() const noexcept { return 42; } };
//  mov         eax,2Ah
//  ret

int calls_foo() { return foo(S{}); }
//  sub         rsp,38h
//  lea         rax,[glue::fn (...)]       ; f (tbl SBO)
//  lea         rcx,[rsp+20h]              ; iface_base at [rsp+20h]
//  mov         qword ptr [rsp+20h],rax    ; set tbl
//  call        foo (...)                  ; note no obj setup (obj SOO)
//  add         rsp,38h
//  ret
```

## Using in your project

Please see `LICENSE` for terms of use.

The easiest way to use iface in your project is to include it as a submodule: `git submodule add git@github.com:j-nikki/iface.git`. Now discover iface in your CMake file (so you can `#include <iface.h>` in C++):
```cmake
add_subdirectory("iface")
target_link_libraries(<target-name> ... iface)
```
