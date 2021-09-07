#!/usr/bin/env python3

n = int(input())
n += 1
print(bin(n))
for i in range(10):
    n |= n >> 2**i
    print(bin(n))
n += 1
print(n)
print(n >> 1)
