# iface

This project brings non-intrusive interfaces similar to Go's to C++. Leveraging [P0315R4](https://wg21.link/P0315R4), iface allows you declare an 'anonymous interface' inside a function signature (see example below). Currently tested on VS 2019 Preview only.

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

Example of code generation below. `Iface_base` occupies 16B (`tbl` 8B + `obj` 8B) and is hence stored on stack, its address passed in `rcx`, as per the [x64 calling convention](https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-160).

```c++
int __declspec(noinline) foo(IFACE((f, int() const)) x) { return x.f(); }
//  mov         rax,qword ptr [rcx]
//  mov         rcx,qword ptr [rcx+8]      ; obj
//  mov         rdx,qword ptr [rax]        ; tbl[0]
//  jmp         rdx

struct S { int f() const noexcept { return 42; } };
//  mov         eax,2Ah
//  ret

int __declspec(noinline) calls_foo() { return foo(S{}); }
//  sub         rsp,38h
//  lea         rax,[S> (...)]
//  lea         rcx,[rsp+20h]              ; Iface_base at [rsp+20h]
//  mov         qword ptr [rsp+20h],rax    ; set tbl
//  call        foo (...)
//  add         rsp,38h
//  ret
```
