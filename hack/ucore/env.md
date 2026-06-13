# install i32 gcc
1. Add env
```plain
export PREFIX="~Application/i386elfgcc"
export TARGET=i386-elf
export PATH="$PREFIX/bin:$PATH"
```
2. install the gcc infrastructure `gmp` `mpc` `mpfr`
3. install `binutil`
4. install gcc
