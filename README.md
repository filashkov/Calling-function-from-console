# x86-64 Unix function calling from dynamic library

## Info
This app can call a function from Unix dynamic library with any number of arguments. Supported types are `unsigned` / `signed` `int`, `long`, `long long`, floating point types `float`, `double` and c strings `char*`.

## Compilation:
```
$ clang++ call.cpp -o call -std=c++2a -ldl
```
`-ldl` is required to load functions from the dynamic library.
## Launch:
```
$ <prog> <[optional, default /lib/x86_64-linux-gnu/libc.so.6] lib path> <function name> <args...>
```
Lib path should start with `/`, `~/` or `./`, because it will help distinguish the name of the library from the name of the called function.

## Examples:
```
$ ./call /lib/x86_64-linux-gnu/libm.so.6 cos 6.283185307179586
============ CALLING FUNCTION ============

========== END OF FUNCTION CALL ==========
%rax value = 0
%xmm0 value = 1
```
```
$ ./call printf "Hello, it's printf! %lld %lf %lld %lf %lld %lf %lld %lf %lld %lf %lld %lf %lld %lf %lld %lf %lld >%s< and finally %lf" -1 -2.0 3 4.4 5 6.0 7 8.0 9 10.10 11 12.5 13 14.7 15 16.7 17 text 18.8
============ CALLING FUNCTION ============
Hello, it's printf! -1 -2.000000 3 4.400000 5 6.000000 7 8.000000 9 10.100000 11 12.500000 13 14.700000 15 16.700000 17 >text< and finally 18.800000
========== END OF FUNCTION CALL ==========
%rax value = 148
%xmm0 value = 9.53282e-130
```
