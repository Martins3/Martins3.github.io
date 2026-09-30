# C++ Core Guidelines 完整规则标题索引

配合 [中文整理](core-guidelines.md) 阅读。按原文顺序保留所有具有数字编号的规则标题，点击编号直达规则正文。
这里是英文标题索引，不是逐条中文翻译；原文标记为 `???`、`removed` 的内容照原样保留。
安全配置中以列表而非标题列出的 Type / Bounds / Lifetime 规则另列于末尾。

来源：原文 Markdown 修订 `33bcd015997f0d8e0fa0202eb66254a16f59ad8f`；整理于 2026-09-05。
Copyright (c) Standard C++ Foundation and its contributors。授权见 [随附文本](core-guidelines-LICENSE.txt)。

## [In: Introduction](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-introduction)

- [In.0](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r0): Don't panic!
## [P: Philosophy](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-philosophy)

- [P.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-direct): Express ideas directly in code
- [P.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-cplusplus): Write in ISO Standard C++
- [P.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-what): Express intent
- [P.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-typesafe): Ideally, a program should be statically type safe
- [P.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-compile-time): Prefer compile-time checking to run-time checking
- [P.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-run-time): What cannot be checked at compile time should be checkable at run time
- [P.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-early): Catch run-time errors early
- [P.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-leak): Don't leak any resources
- [P.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-waste): Don't waste time or space
- [P.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-mutable): Prefer immutable data to mutable data
- [P.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-library): Encapsulate messy constructs, rather than spreading through the code
- [P.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-tools): Use supporting tools as appropriate
- [P.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rp-lib): Use support libraries as appropriate
## [I: Interfaces](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-interfaces)

- [I.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-explicit): Make interfaces explicit
- [I.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-global): Avoid non-`const` global variables
- [I.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-singleton): Avoid singletons
- [I.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-typed): Make interfaces precisely and strongly typed
- [I.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-pre): State preconditions (if any)
- [I.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-expects): Prefer `Expects()` for expressing preconditions
- [I.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-post): State postconditions
- [I.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-ensures): Prefer `Ensures()` for expressing postconditions
- [I.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-concepts): If an interface is a template, document its parameters using concepts
- [I.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-except): Use exceptions to signal a failure to perform a required task
- [I.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-raw): Never transfer ownership by a raw pointer (`T*`) or reference (`T&`)
- [I.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-nullptr): Declare a pointer that must not be null as `not_null`
- [I.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-array): Do not pass an array as a single pointer
- [I.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-global-init): Avoid complex initialization of global objects
- [I.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-nargs): Keep the number of function arguments low
- [I.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-unrelated): Avoid adjacent parameters that can be invoked by the same arguments in either order with different meaning
- [I.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-abstract): Prefer empty abstract classes as interfaces to class hierarchies
- [I.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-abi): If you want a cross-compiler ABI, use a C-style subset
- [I.27](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-pimpl): For stable library ABI, consider the Pimpl idiom
- [I.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-encapsulate): Encapsulate rule violations
## [F: Functions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-functions)

- [F.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-package): "Package" meaningful operations as carefully named functions
- [F.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-logical): A function should perform a single logical operation
- [F.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-single): Keep functions short and simple
- [F.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-constexpr): If a function might have to be evaluated at compile time, declare it `constexpr`
- [F.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-inline): If a function is very small and time-critical, declare it `inline`
- [F.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-noexcept): If your function must not throw, declare it `noexcept`
- [F.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-smart): For general use, take `T*` or `T&` arguments rather than smart pointers
- [F.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-pure): Prefer pure functions
- [F.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-unused): Unused parameters should be unnamed
- [F.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-name): If an operation can be reused, give it a name
- [F.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-lambda): Use an unnamed lambda if you need a simple function object in one place only
- [F.15](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-conventional): Prefer simple and conventional ways of passing information
- [F.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-in): For "in" parameters, pass cheaply-copied types by value and others by reference to `const`
- [F.17](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-inout): For "in-out" parameters, pass by reference to non-`const`
- [F.18](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-consume): For "will-move-from" parameters, pass by `X&&` and `std::move` the parameter
- [F.19](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-forward): For "forward" parameters, pass by `TP&&` and only `std::forward` the parameter
- [F.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-out): For "out" output values, prefer return values to output parameters
- [F.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-out-multi): To return multiple "out" values, prefer returning a struct
- [F.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-ptr-ref): Prefer `T*` over `T&` when "no argument" is a valid option
- [F.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-ptr): Use `T*` or `owner<T*>` to designate a single object
- [F.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-nullptr): Use a `not_null<T>` to indicate that "null" is not a valid value
- [F.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-range): Use a `span<T>` or a `span_p<T>` to designate a half-open sequence
- [F.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-zstring): Use a `zstring` or a `not_null<zstring>` to designate a C-style string
- [F.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-unique_ptr): Use a `unique_ptr<T>` to transfer ownership where a pointer is needed
- [F.27](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-shared_ptr): Use a `shared_ptr<T>` to share ownership
- [F.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-ptr): Return a `T*` to indicate a position (only)
- [F.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-dangle): Never (directly or indirectly) return a pointer or a reference to a local object
- [F.44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-ref): Return a `T&` when copy is undesirable and "returning no object" isn't needed
- [F.45](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-ref-ref): Don't return a `T&&`
- [F.46](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-main): `int` is the return type for `main()`
- [F.47](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-assignment-op): Return `T&` from assignment operators
- [F.48](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-move-local): Don't `return std::move(local)`
- [F.49](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-return-const): Don't return `const T`
- [F.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-capture-vs-overload): Use a lambda when a function won't do (to capture local variables, or to write a local function)
- [F.51](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-default-args): Where there is a choice, prefer default arguments over overloading
- [F.52](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-reference-capture): Prefer capturing by reference in lambdas that will be used locally, including passed to algorithms
- [F.53](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-value-capture): Avoid capturing by reference in lambdas that will be used non-locally, including returned, stored on the heap, or passed to another thread
- [F.54](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-this-capture): When writing a lambda that captures `this` or any class data member, don't use `[=]` default capture
- [F.55](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f-varargs): Don't use `va_arg` arguments
- [F.56](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f-nesting): Avoid unnecessary condition nesting
## [C: Classes and class hierarchies](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-class)

- [C.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-org): Organize related data into structures (`struct`s or `class`es)
- [C.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-struct): Use `class` if the class has an invariant; use `struct` if the data members can vary independently
- [C.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-interface): Represent the distinction between an interface and an implementation using a class
- [C.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-member): Make a function a member only if it needs direct access to the representation of a class
- [C.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-helper): Place helper functions in the same namespace as the class they support
- [C.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-standalone): Don't define a class or enum and declare a variable of its type in the same statement
- [C.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-class): Use `class` rather than `struct` if any member is non-public
- [C.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-private): Minimize exposure of members
- [C.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-concrete): Prefer concrete types over class hierarchies
- [C.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-regular): Make concrete types regular
- [C.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-constref): Don't make data members `const` or references in a copyable or movable type
- [C.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-lifetime): If data member `B` uses another data member `A`, declare `A` before `B`
- [C.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-zero): If you can avoid defining default operations, do
- [C.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-five): If you define or `=delete` any copy, move, or destructor function, define or `=delete` them all
- [C.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-matched): Make default operations consistent
- [C.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor): Define a destructor if a class needs an explicit action at object destruction
- [C.31](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-release): All resources acquired by a class must be released by the class's destructor
- [C.32](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-ptr): If a class has a raw pointer (`T*`) or reference (`T&`), consider whether it might be owning
- [C.33](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-ptr2): If a class has an owning pointer member, define a destructor
- [C.35](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-virtual): A base class destructor should be either public and virtual, or protected and non-virtual
- [C.36](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-fail): A destructor must not fail
- [C.37](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-dtor-noexcept): Make destructors `noexcept`
- [C.40](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-ctor): Define a constructor if a class has an invariant
- [C.41](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-complete): A constructor should create a fully initialized object
- [C.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-throw): If a constructor cannot construct a valid object, throw an exception
- [C.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-default0): Ensure that a copyable class has a default constructor
- [C.44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-default00): Prefer default constructors to be simple and non-throwing
- [C.45](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-default): Don't define a default constructor that only initializes data members; use default member initializers instead
- [C.46](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-explicit): By default, declare single-argument constructors explicit
- [C.47](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-order): Define and initialize data members in the order of member declaration
- [C.48](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-in-class-initializer): Prefer default member initializers to member initializers in constructors for constant initializers
- [C.49](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-initialize): Prefer initialization to assignment in constructors
- [C.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-factory): Use a factory function if you need "virtual behavior" during initialization
- [C.51](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-delegating): Use delegating constructors to represent common actions for all constructors of a class
- [C.52](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-inheriting): Use inheriting constructors to import constructors into a derived class that does not need further explicit initialization
- [C.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-copy-assignment): Make copy assignment non-`virtual`, take the parameter by `const&`, and return by non-`const&`
- [C.61](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-copy-semantic): A copy operation should copy
- [C.62](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-copy-self): Make copy assignment safe for self-assignment
- [C.63](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-move-assignment): Make move assignment non-`virtual`, take the parameter by `&&`, and return by non-`const&`
- [C.64](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-move-semantic): A move operation should move and leave its source in a valid state
- [C.65](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-move-self): Make move assignment safe for self-assignment
- [C.66](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-move-noexcept): Make move operations `noexcept`
- [C.67](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-copy-virtual): A polymorphic class should suppress public copy/move
- [C.80](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-eqdefault): Use `=default` if you have to be explicit about using the default semantics
- [C.81](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-delete): Use `=delete` when you want to disable default behavior (without wanting an alternative)
- [C.82](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-ctor-virtual): Don't call virtual functions in constructors and destructors
- [C.83](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-swap): For value-like types, consider providing a `noexcept` swap function
- [C.84](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-swap-fail): A `swap` function must not fail
- [C.85](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-swap-noexcept): Make `swap` `noexcept`
- [C.86](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-eq): Make `==` symmetric with respect to operand types and `noexcept`
- [C.87](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-eq-base): Beware of `==` on base classes
- [C.89](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-hash): Make a `hash` `noexcept`
- [C.90](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-memset): Rely on constructors and assignment operators, not `memset` and `memcpy`
- [C.100](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcon-stl): Follow the STL when defining a container
- [C.101](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcon-val): Give a container value semantics
- [C.102](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcon-move): Give a container move operations
- [C.103](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcon-init): Give a container an initializer list constructor
- [C.104](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcon-empty): Give a container a default constructor that sets it to empty
- [C.109](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcon-ptr): If a resource handle has pointer semantics, provide `*` and `->`
- [C.120](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-domain): Use class hierarchies to represent concepts with inherent hierarchical structure (only)
- [C.121](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-abstract): If a base class is used as an interface, make it a pure abstract class
- [C.122](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-separation): Use abstract classes as interfaces when complete separation of interface and implementation is needed
- [C.126](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-abstract-ctor): An abstract class typically doesn't need a user-written constructor
- [C.127](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-dtor): A class with a virtual function should have a virtual or protected destructor
- [C.128](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-override): Virtual functions should specify exactly one of `virtual`, `override`, or `final`
- [C.129](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-kind): When designing a class hierarchy, distinguish between implementation inheritance and interface inheritance
- [C.130](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-copy): For making deep copies of polymorphic classes prefer a virtual `clone` function instead of public copy construction/assignment
- [C.131](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-get): Avoid trivial getters and setters
- [C.132](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-virtual): Don't make a function `virtual` without reason
- [C.133](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-protected): Avoid `protected` data
- [C.134](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-public): Ensure all non-`const` data members have the same access level
- [C.135](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-mi-interface): Use multiple inheritance to represent multiple distinct interfaces
- [C.136](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-mi-implementation): Use multiple inheritance to represent the union of implementation attributes
- [C.137](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-vbase): Use `virtual` bases to avoid overly general base classes
- [C.138](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-using): Create an overload set for a derived class and its bases with `using`
- [C.139](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-final): Use `final` on classes sparingly
- [C.140](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-virtual-default-arg): Do not provide different default arguments for a virtual function and an overrider
- [C.145](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-poly): Access polymorphic objects through pointers and references
- [C.146](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-dynamic_cast): Use `dynamic_cast` where class hierarchy navigation is unavoidable
- [C.147](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-ref-cast): Use `dynamic_cast` to a reference type when failure to find the required class is considered an error
- [C.148](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-ptr-cast): Use `dynamic_cast` to a pointer type when failure to find the required class is considered a valid alternative
- [C.149](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-smart): Use `unique_ptr` or `shared_ptr` to avoid forgetting to `delete` objects created using `new`
- [C.150](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-make_unique): Use `make_unique()` to construct objects owned by `unique_ptr`s
- [C.151](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-make_shared): Use `make_shared()` to construct objects owned by `shared_ptr`s
- [C.152](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-array): Never assign a pointer to an array of derived class objects to a pointer to its base
- [C.153](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rh-use-virtual): Prefer virtual function to casting
- [C.160](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-conventional): Define operators primarily to mimic conventional usage
- [C.161](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-symmetric): Use non-member functions for symmetric operators
- [C.162](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-equivalent): Overload operations that are roughly equivalent
- [C.163](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-equivalent-2): Overload only for operations that are roughly equivalent
- [C.164](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-conversion): Avoid implicit conversion operators
- [C.165](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-custom): Use `using` for customization points
- [C.166](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-address-of): Overload unary `&` only as part of a system of smart pointers and references
- [C.167](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-overload): Use an operator for an operation with its conventional meaning
- [C.168](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-namespace): Define overloaded operators in the namespace of their operands
- [C.170](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-lambda): If you feel like overloading a lambda, use a generic lambda
- [C.180](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ru-union): Use `union`s to save memory
- [C.181](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ru-naked): Avoid "naked" `union`s
- [C.182](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ru-anonymous): Use anonymous `union`s to implement tagged unions
- [C.183](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ru-pun): Don't use a `union` for type punning
## [Enum: Enumerations](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-enum)

- [Enum.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-macro): Prefer enumerations over macros
- [Enum.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-set): Use enumerations to represent sets of related named constants
- [Enum.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-class): Prefer class enums over "plain" enums
- [Enum.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-oper): Define operations on enumerations for safe and simple use
- [Enum.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-caps): Don't use `ALL_CAPS` for enumerators
- [Enum.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-unnamed): Avoid unnamed enumerations
- [Enum.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-underlying): Specify the underlying type of an enumeration only when necessary
- [Enum.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#renum-value): Specify enumerator values only when necessary
## [R: Resource management](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-resource)

- [R.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-raii): Manage resources automatically using resource handles and RAII (Resource Acquisition Is Initialization)
- [R.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-use-ptr): In interfaces, use raw pointers to denote individual objects (only)
- [R.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-ptr): A raw pointer (a `T*`) is non-owning
- [R.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-ref): A raw reference (a `T&`) is non-owning
- [R.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-scoped): Prefer scoped objects, don't heap-allocate unnecessarily
- [R.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-global): Avoid non-`const` global variables
- [R.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-mallocfree): Avoid `malloc()` and `free()`
- [R.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-newdelete): Avoid calling `new` and `delete` explicitly
- [R.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-immediate-alloc): Immediately give the result of an explicit resource allocation to a manager object
- [R.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-single-alloc): Perform at most one explicit resource allocation in a single expression statement
- [R.14](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-ap): Avoid `[]` parameters, prefer `span`
- [R.15](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-pair): Always overload matched allocation/deallocation pairs
- [R.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-owner): Use `unique_ptr` or `shared_ptr` to represent ownership
- [R.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-unique): Prefer `unique_ptr` over `shared_ptr` unless you need to share ownership
- [R.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-make_shared): Use `make_shared()` to make `shared_ptr`s
- [R.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-make_unique): Use `make_unique()` to make `unique_ptr`s
- [R.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-weak_ptr): Use `std::weak_ptr` to break cycles of `shared_ptr`s
- [R.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-smartptrparam): Take smart pointers as parameters only to explicitly express lifetime semantics
- [R.31](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-smart): If you have non-`std` smart pointers, follow the basic pattern from `std`
- [R.32](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-uniqueptrparam): Take a `unique_ptr<widget>` parameter to express that a function assumes ownership of a `widget`
- [R.33](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-reseat): Take a `unique_ptr<widget>&` parameter to express that a function reseats the `widget`
- [R.34](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-sharedptrparam-owner): Take a `shared_ptr<widget>` parameter to express shared ownership
- [R.35](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-sharedptrparam): Take a `shared_ptr<widget>&` parameter to express that a function might reseat the shared pointer
- [R.36](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-sharedptrparam-const): Take a `const shared_ptr<widget>&` parameter to express that it might retain a reference count to the object ???
- [R.37](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-smartptrget): Do not pass a pointer or reference obtained from an aliased smart pointer
## [ES: Expressions and statements](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-expr)

- [ES.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-lib): Prefer the standard library to other libraries and to "handcrafted code"
- [ES.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-abstr): Prefer suitable abstractions to direct use of language features
- [ES.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-dry): Don't repeat yourself, avoid redundant code
- [ES.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-scope): Keep scopes small
- [ES.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-cond): Declare names in for-statement initializers and conditions to limit scope
- [ES.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-name-length): Keep common and local names short, and keep uncommon and non-local names longer
- [ES.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-name-similar): Avoid similar-looking names
- [ES.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-not-caps): Avoid `ALL_CAPS` names
- [ES.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-name-one): Declare one name (only) per declaration
- [ES.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-auto): Use `auto` to avoid redundant repetition of type names
- [ES.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-reuse): Do not reuse names in nested scopes
- [ES.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-always): Always initialize an object
- [ES.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-introduce): Don't introduce a variable (or constant) before you need to use it
- [ES.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-init): Don't declare a variable until you have a value to initialize it with
- [ES.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-list): Prefer the `{}`-initializer syntax
- [ES.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-unique): Use a `unique_ptr<T>` to hold pointers
- [ES.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-const): Declare an object `const` or `constexpr` unless you want to modify its value later on
- [ES.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-recycle): Don't use a variable for two unrelated purposes
- [ES.27](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-stack): Use `std::array` or `stack_array` for arrays on the stack
- [ES.28](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-lambda-init): Use lambdas for complex initialization, especially of `const` variables
- [ES.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-macros): Don't use macros for program text manipulation
- [ES.31](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-macros2): Don't use macros for constants or "functions"
- [ES.32](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-all_caps): Use `ALL_CAPS` for all macro names
- [ES.33](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-macros3): If you must use macros, give them unique names
- [ES.34](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-ellipses): Don't define a (C-style) variadic function
- [ES.40](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-complicated): Avoid complicated expressions
- [ES.41](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-parens): If in doubt about operator precedence, parenthesize
- [ES.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-ptr): Keep use of pointers simple and straightforward
- [ES.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-order): Avoid expressions with undefined order of evaluation
- [ES.44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-order-fct): Don't depend on order of evaluation of function arguments
- [ES.45](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-magic): Avoid "magic constants"; use symbolic constants
- [ES.46](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-narrowing): Avoid lossy (narrowing, truncating) arithmetic conversions
- [ES.47](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-nullptr): Use `nullptr` rather than `0` or `NULL`
- [ES.48](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-casts): Avoid casts
- [ES.49](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-casts-named): If you must use a cast, use a named cast
- [ES.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-casts-const): Don't cast away `const`
- [ES.55](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-range-checking): Avoid the need for range checking
- [ES.56](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-move): Write `std::move()` only when you need to explicitly move an object to another scope
- [ES.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-new): Avoid `new` and `delete` outside resource management functions
- [ES.61](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-del): Delete arrays using `delete[]` and non-arrays using `delete`
- [ES.62](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-arr2): Don't compare pointers into different arrays
- [ES.63](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-slice): Don't slice
- [ES.64](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-construct): Use the `T{e}`notation for construction
- [ES.65](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-deref): Don't dereference an invalid pointer
- [ES.70](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-switch-if): Prefer a `switch`-statement to an `if`-statement when there is a choice
- [ES.71](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-for-range): Prefer a range-`for`-statement to a `for`-statement when there is a choice
- [ES.72](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-for-while): Prefer a `for`-statement to a `while`-statement when there is an obvious loop variable
- [ES.73](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-while-for): Prefer a `while`-statement to a `for`-statement when there is no obvious loop variable
- [ES.74](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-for-init): Prefer to declare a loop variable in the initializer part of a `for`-statement
- [ES.75](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-do): Avoid `do`-statements
- [ES.76](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-goto): Avoid `goto`
- [ES.77](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-continue): Minimize the use of `break` and `continue` in loops
- [ES.78](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-break): Don't rely on implicit fallthrough in `switch` statements
- [ES.79](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-default): Use `default` to handle common cases (only)
- [ES.84](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-noname): Don't try to declare a local variable with no name
- [ES.85](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-empty): Make empty statements visible
- [ES.86](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-loop-counter): Avoid modifying loop control variables inside the body of raw for-loops
- [ES.87](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-if): Don't add redundant `==` or `!=` to conditions
- [ES.100](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-mix): Don't mix signed and unsigned arithmetic
- [ES.101](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-unsigned): Use unsigned types for bit manipulation
- [ES.102](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-signed): Use signed types for arithmetic
- [ES.103](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-overflow): Don't overflow
- [ES.104](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-underflow): Don't underflow
- [ES.105](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-zero): Don't divide by integer zero
- [ES.106](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-nonnegative): Don't try to avoid negative values by using `unsigned`
- [ES.107](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-subscripts): Don't use `unsigned` for subscripts, prefer `gsl::index`
## [Per: Performance](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-performance)

- [Per.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-reason): Don't optimize without reason
- [Per.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-knuth): Don't optimize prematurely
- [Per.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-critical): Don't optimize something that's not performance critical
- [Per.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-simple): Don't assume that complicated code is necessarily faster than simple code
- [Per.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-low): Don't assume that low-level code is necessarily faster than high-level code
- [Per.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-measure): Don't make claims about performance without measurements
- [Per.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-efficiency): Design to enable optimization
- [Per.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-type): Rely on the static type system
- [Per.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-comp): Move computation from run time to compile time
- [Per.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-alias): Eliminate redundant aliases
- [Per.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-indirect): Eliminate redundant indirections
- [Per.14](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-alloc): Minimize the number of allocations and deallocations
- [Per.15](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-alloc0): Do not allocate on a critical branch
- [Per.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-compact): Use compact data structures
- [Per.17](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-struct): Declare the most used member of a time-critical struct first
- [Per.18](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-space): Space is time
- [Per.19](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-access): Access memory predictably
- [Per.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rper-context): Avoid context switches on the critical path
## [CP: Concurrency and parallelism](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-concurrency)

- [CP.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-multi): Assume that your code will run as part of a multi-threaded program
- [CP.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-races): Avoid data races
- [CP.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-data): Minimize explicit sharing of writable data
- [CP.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-task): Think in terms of tasks, rather than threads
- [CP.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-volatile): Don't try to use `volatile` for synchronization
- [CP.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-tools): Whenever feasible use tools to validate your concurrent code
- [CP.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-raii): Use RAII, never plain `lock()`/`unlock()`
- [CP.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-lock): Use `std::lock()` or `std::scoped_lock` to acquire multiple `mutex`es
- [CP.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-unknown): Never call unknown code while holding a lock (e.g., a callback)
- [CP.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-join): Think of a joining `thread` as a scoped container
- [CP.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-detach): Think of a `thread` as a global container
- [CP.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-joining_thread): Prefer `gsl::joining_thread` over `std::thread`
- [CP.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-detached_thread): Don't `detach()` a thread
- [CP.31](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-data-by-value): Pass small amounts of data between threads by value, rather than by reference or pointer
- [CP.32](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-shared): To share ownership between unrelated `thread`s use `shared_ptr`
- [CP.40](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-switch): Minimize context switching
- [CP.41](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-create): Minimize thread creation and destruction
- [CP.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-wait): Don't `wait` without a condition
- [CP.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-time): Minimize time spent in a critical section
- [CP.44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-name): Remember to name your `lock_guard`s and `unique_lock`s
- [CP.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-mutex): Define a `mutex` together with the data it guards. Use `synchronized_value<T>` where possible
- [CP.51](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcoro-capture): Do not use capturing lambdas that are coroutines
- [CP.52](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcoro-locks): Do not hold locks or other synchronization primitives across suspension points
- [CP.53](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcoro-reference-parameters): Parameters to coroutines should not be passed by reference
- [CP.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-future): Use a `future` to return a value from a concurrent task
- [CP.61](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-async): Use `async()` to spawn concurrent tasks
- [CP.100](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-lockfree): Don't use lock-free programming unless you absolutely have to
- [CP.101](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-distrust): Distrust your hardware/compiler combination
- [CP.102](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-literature): Carefully study the literature
- [CP.110](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-double): Do not write your own double-checked locking for initialization
- [CP.111](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-double-pattern): Use a conventional pattern if you really need double-checked locking
- [CP.200](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-volatile2): Use `volatile` only to talk to non-C++ memory
- [CP.201](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconc-signal): ??? Signals
## [E: Error handling](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-errors)

- [E.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-design): Develop an error-handling strategy early in a design
- [E.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-throw): Throw an exception to signal that a function can't perform its assigned task
- [E.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-errors): Use exceptions for error handling only
- [E.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-design-invariants): Design your error-handling strategy around invariants
- [E.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-invariant): Let a constructor establish an invariant, and throw if it cannot
- [E.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-raii): Use RAII to prevent leaks
- [E.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-precondition): State your preconditions
- [E.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-postcondition): State your postconditions
- [E.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-noexcept): Use `noexcept` when exiting a function because of a `throw` is impossible or unacceptable
- [E.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-never-throw): Never throw while being the direct owner of an object
- [E.14](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-exception-types): Use purpose-designed user-defined types as exceptions (not built-in types)
- [E.15](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-exception-ref): Throw by value, catch exceptions from a hierarchy by reference
- [E.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-never-fail): Destructors, deallocation, `swap`, and exception type copy/move construction must never fail
- [E.17](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-not-always): Don't try to catch every exception in every function
- [E.18](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-catch): Minimize the use of explicit `try`/`catch`
- [E.19](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-finally): Use a `final_action` object to express cleanup if no suitable resource handle is available
- [E.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-no-throw-raii): If you can't throw exceptions, simulate RAII for resource management
- [E.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-no-throw-crash): If you can't throw exceptions, consider failing fast
- [E.27](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-no-throw-codes): If you can't throw exceptions, use error codes systematically
- [E.28](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-no-throw): Avoid error handling based on global state (e.g. `errno`)
- [E.30](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re-specifications): Don't use exception specifications
- [E.31](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#re_catch): Properly order your `catch`-clauses
## [Con: Constants and immutability](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-const)

- [Con.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-immutable): By default, make objects immutable
- [Con.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-fct): By default, make member functions `const`
- [Con.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-ref): By default, pass pointers and references to `const`s
- [Con.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-const): Use `const` to define objects with values that do not change after construction
- [Con.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rconst-constexpr): Use `constexpr` for values that can be computed at compile time
## [T: Templates and generic programming](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-templates)

- [T.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-raise): Use templates to raise the level of abstraction of code
- [T.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-algo): Use templates to express algorithms that apply to many argument types
- [T.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-cont): Use templates to express containers and ranges
- [T.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-expr): Use templates to express syntax tree manipulation
- [T.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-generic-oo): Combine generic and OO techniques to amplify their strengths, not their costs
- [T.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-concepts): Specify concepts for all template arguments
- [T.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-std-concepts): Whenever possible use standard concepts
- [T.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-auto): Prefer concept names over `auto` for local variables
- [T.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-shorthand): Prefer the shorthand notation for simple, single-type argument concepts
- [T.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-low): Avoid "concepts" without meaningful semantics
- [T.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-complete): Require a complete set of operations for a concept
- [T.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-axiom): Specify axioms for concepts
- [T.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-refine): Differentiate a refined concept from its more general case by adding new use patterns.
- [T.24](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-tag): Use tag classes or traits to differentiate concepts that differ only in semantics.
- [T.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-not): Avoid complementary constraints
- [T.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-use): Prefer to define concepts in terms of use-patterns rather than simple syntax
- [T.40](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-fo): Use function objects to pass operations to algorithms
- [T.41](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-essential): Require only essential properties in a template's concepts
- [T.42](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-alias): Use template aliases to simplify notation and hide implementation details
- [T.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-using): Prefer `using` over `typedef` for defining aliases
- [T.44](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-deduce): Use function templates to deduce class template argument types (where feasible)
- [T.46](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-regular): (removed)
- [T.47](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-visible): Avoid highly visible unconstrained templates with common names
- [T.48](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-concept-def): If your compiler does not support concepts, fake them with `enable_if`
- [T.49](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-erasure): Where possible, avoid type-erasure
- [T.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-depend): Minimize a template's context dependencies
- [T.61](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-scary): Do not over-parameterize members (SCARY)
- [T.62](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-nondependent): Place non-dependent class template members in a non-templated base class
- [T.64](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-specialization): Use specialization to provide alternative implementations of class templates
- [T.65](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-tag-dispatch): Use tag dispatch to provide alternative implementations of a function
- [T.67](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-specialization2): Use specialization to provide alternative implementations for irregular types
- [T.68](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-cast): Use `{}` rather than `()` within templates to avoid ambiguities
- [T.69](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-customization): Inside a template, don't make an unqualified non-member function call unless you intend it to be a customization point
- [T.80](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-hier): Do not naively templatize a class hierarchy
- [T.81](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-array): Do not mix hierarchies and arrays
- [T.82](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-linear): Linearize a hierarchy when virtual functions are undesirable
- [T.83](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-virtual): Do not declare a member function template virtual
- [T.84](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-abi): Use a non-template core implementation to provide an ABI-stable interface
- [T.100](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-variadic): Use variadic templates when you need a function that takes a variable number of arguments of a variety of types
- [T.101](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-variadic-pass): ??? How to pass arguments to a variadic template ???
- [T.102](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-variadic-process): How to process arguments to a variadic template
- [T.103](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-variadic-not): Don't use variadic templates for homogeneous argument lists
- [T.120](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-metameta): Use template metaprogramming only when you really need to
- [T.121](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-emulate): Use template metaprogramming primarily to emulate concepts
- [T.122](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-tmp): Use templates (usually template aliases) to compute types at compile time
- [T.123](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-fct): Use `constexpr` functions to compute values at compile time
- [T.124](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-std-tmp): Prefer to use standard-library TMP facilities
- [T.125](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-lib): If you need to go beyond the standard-library TMP facilities, use an existing library
- [T.140](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-name): If an operation can be reused, give it a name
- [T.141](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-lambda): Use an unnamed lambda if you need a simple function object in one place only
- [T.142?](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-var): Use template variables to simplify notation
- [T.143](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-non-generic): Don't write unintentionally non-generic code
- [T.144](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-specialize-function): Don't specialize function templates
- [T.150](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rt-check-class): Check that a class matches a concept using `static_assert`
## [CPL: C-style programming](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-cpl)

- [CPL.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcpl-c): Prefer C++ to C
- [CPL.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcpl-subset): If you must use C, use the common subset of C and C++, and compile the C code as C++
- [CPL.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rcpl-interface): If you must use C for interfaces, use C++ in the calling code using such interfaces
## [SF: Source files](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-source)

- [SF.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-file-suffix): Use a `.cpp` suffix for code files and `.h` for interface files if your project doesn't already follow another convention
- [SF.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-inline): A header file must not contain object definitions or non-inline function definitions
- [SF.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-declaration-header): Use header files for all declarations used in multiple source files
- [SF.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-include-order): Include header files before other declarations in a file
- [SF.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-consistency): A `.cpp` file must include the header file(s) that defines its interface
- [SF.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-using): Use `using namespace` directives for transition, for foundation libraries (such as `std`), or within a local scope (only)
- [SF.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-using-directive): Don't write `using namespace` at global scope in a header file
- [SF.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-guards): Use `#include` guards for all header files
- [SF.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-cycles): Avoid cyclic dependencies among source files
- [SF.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-implicit): Avoid dependencies on implicitly `#include`d names
- [SF.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-contained): Header files should be self-contained
- [SF.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-incform): Prefer the quoted form of `#include` for files relative to the including file and the angle bracket form everywhere else
- [SF.13](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-portable-header-id): Use portable header identifiers in `#include` statements
- [SF.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-namespace): Use `namespace`s to express logical structure
- [SF.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-unnamed): Don't use an unnamed (anonymous) namespace in a header
- [SF.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-unnamed2): Use an unnamed (anonymous) namespace for all internal/non-exported entities
## [SL: The Standard Library](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-stdlib)

- [SL.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-lib): Use libraries wherever possible
- [SL.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-sl): Prefer the standard library to other libraries
- [SL.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#sl-std): Do not add non-standard entities to namespace `std`
- [SL.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#sl-safe): Use the standard library in a type-safe manner
- [SL.con.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-arrays): Prefer using STL `array` or `vector` instead of a C array
- [SL.con.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-vector): Prefer using STL `vector` by default unless you have a reason to use a different container
- [SL.con.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-bounds): Avoid bounds errors
- [SL.con.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsl-copy): don't use `memset` or `memcpy` for arguments that are not trivially-copyable
- [SL.str.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-string): Use `std::string` to own character sequences
- [SL.str.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-view): Use `std::string_view` or `gsl::span<char>` to refer to character sequences
- [SL.str.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-zstring): Use `zstring` or `czstring` to refer to a C-style, zero-terminated, sequence of characters
- [SL.str.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-charp): Use `char*` to refer to a single character
- [SL.str.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-byte): Use `std::byte` to refer to byte values that do not necessarily represent characters
- [SL.str.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-locale): Use `std::string` when you need to perform locale-sensitive string operations
- [SL.str.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-span): Use `gsl::span<char>` rather than `std::string_view` when you need to mutate a string
- [SL.str.12](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rstr-s): Use the `s` suffix for string literals meant to be standard-library `string`s
- [SL.io.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rio-low): Use character-level input only when you have to
- [SL.io.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rio-validate): When reading, always consider ill-formed input
- [SL.io.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rio-streams): Prefer `iostream`s for I/O
- [SL.io.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rio-sync): Unless you use `printf`-family functions call `ios_base::sync_with_stdio(false)`
- [SL.io.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rio-endl): Avoid `endl`
- [SL.C.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rclib-jmp): Don't use setjmp/longjmp
## [A: Architectural ideas](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-a)

- [A.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ra-stable): Separate stable code from less stable code
- [A.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ra-lib): Express potentially reusable parts as a library
- [A.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ra-dag): There should be no cycles among libraries
## [NR: Non-Rules and myths](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-not)

- [NR.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rnr-top): Don't insist that all declarations should be at the top of a function
- [NR.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rnr-single-return): Don't insist on having only a single `return`-statement in a function
- [NR.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rnr-no-exceptions): Don't avoid exceptions
- [NR.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rnr-lots-of-files): Don't insist on placing each class definition in its own source file
- [NR.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rnr-two-phase-init): Don't use two-phase initialization
- [NR.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rnr-goto-exit): Don't place all cleanup actions at the end of a function and `goto exit`
- [NR.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rnr-protected-data): Don't make data members `protected`
## [NL: Naming and layout suggestions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-naming)

- [NL.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-comments): Don't say in comments what can be clearly stated in code
- [NL.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-comments-intent): State intent in comments
- [NL.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-comments-crisp): Keep comments crisp
- [NL.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-indent): Maintain a consistent indentation style
- [NL.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-name-type): Avoid encoding type information in names
- [NL.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-name-length): Make the length of a name roughly proportional to the length of its scope
- [NL.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-name): Use a consistent naming style
- [NL.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-all-caps): Use `ALL_CAPS` for macro names only
- [NL.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-camel): Prefer `underscore_style` names
- [NL.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-literals): Make literals readable
- [NL.15](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-space): Use spaces sparingly
- [NL.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-order): Use a conventional class member declaration order
- [NL.17](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-knr): Use K&R-derived layout
- [NL.18](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-ptr): Use C++-style declarator layout
- [NL.19](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-misread): Avoid names that are easily misread
- [NL.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-stmt): Don't place two statements on the same line
- [NL.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-dcl): Declare one name (only) per declaration
- [NL.25](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-void): Don't use `void` as an argument type
- [NL.26](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-const): Use conventional `const` notation
- [NL.27](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rl-file-suffix): Use a `.cpp` suffix for code files and `.h` for interface files
## [FAQ: Answers to frequently asked questions](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#s-faq)

- [FAQ.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-aims): What do these guidelines aim to achieve?
- [FAQ.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-announced): When and where was this work first announced?
- [FAQ.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-maintainers): Who are the authors and maintainers of these guidelines?
- [FAQ.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-contribute): How can I contribute?
- [FAQ.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-maintainer): How can I become an editor/maintainer?
- [FAQ.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-iso): Have these guidelines been approved by the ISO C++ standards committee? Do they represent the consensus of the committee?
- [FAQ.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-isocpp): If these guidelines are not approved by the committee, why are they under `github.com/isocpp`?
- [FAQ.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-cpp98): Will there be a C++98 version of these Guidelines? A C++11 version?
- [FAQ.9](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-language-extensions): Do these guidelines propose new language features?
- [FAQ.10](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-markdown): What version of Markdown do these guidelines use?
- [FAQ.50](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl): What is the GSL (guidelines support library)?
- [FAQ.51](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-msgsl): Is [github.com/Microsoft/GSL](https://github.com/Microsoft/GSL) the GSL?
- [FAQ.52](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl-implementation): Why not supply an actual GSL implementation in/with these guidelines?
- [FAQ.53](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-boost): Why weren't the GSL types proposed through Boost?
- [FAQ.54](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl-iso): Has the GSL (guidelines support library) been approved by the ISO C++ standards committee?
- [FAQ.55](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl-string-view): If you're using the standard types where available, why is the GSL `span<char>` different from the `string_view` in the Library Fundamentals 1 Technical Specification and C++17 Working Paper? Why not just use the committee-approved `string_view`?
- [FAQ.56](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl-owner): Is `owner` the same as the proposed `observer_ptr`?
- [FAQ.57](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl-stack-array): Is `stack_array` the same as the standard `array`?
- [FAQ.58](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl-dyn-array): Is `dyn_array` the same as `vector` or the proposed `dynarray`?
- [FAQ.59](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl-expects): Is `Expects` the same as `assert`?
- [FAQ.60](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#faq-gsl-ensures): Is `Ensures` the same as `assert`?

## 安全配置中的编号条目

- [Type.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-type-avoidcasts): Avoid casts
- [Type.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-type-downcast): Don't use `static_cast` to downcast
- [Type.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-type-constcast): Don't use `const_cast` to cast away `const` (i.e., at all)
- [Type.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-type-cstylecast): Don't use C-style `(T)expression` or functional `T(expression)` casts
- [Type.5](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-type-init): Don't use a variable before it has been initialized
- [Type.6](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-type-memberinit): Always initialize a data member
- [Type.7](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-type-union): Avoid naked union
- [Type.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-type-varargs): Avoid varargs
- [Bounds.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-bounds-arithmetic): Don't use pointer arithmetic. Use `span` instead
- [Bounds.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-bounds-arrayindex): Only index into arrays using constant expressions
- [Bounds.3](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-bounds-decay): No array-to-pointer decay
- [Bounds.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-bounds-stdlib): Don't use standard-library functions and types that are not bounds-checked
- [Lifetime.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#pro-lifetime-invalid-deref): Don't dereference a possibly invalid pointer

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
