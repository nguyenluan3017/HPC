## Loop 1

```c
for (i=0; i<N; i++) { 
   a[(N+1)*i+k1] = b[i]*c[i]; 
   d[i] = a[(N+1)*i+k2*N]+e; 
}
```


1. Substitute N=5590, k1=k2=559 to the above loop:

```c
for (int i = 0; i < 5590; i++) {
   a[(5591)*i+559] = b[i]*c[i];
   d[i] = a[(5591)*i+3124810]+e;
}
```

2. We have `s1 = 5591 * i1 + 559` and `s2 = 5591 * i2 + 3124810`

3. Dependencies occur if `s1 = s2`, which implies

```
5591 * i1 + 559 = 5591 * i2 + 3124810
=> 5591 * i1 - 5591 * i2 = 3124810 - 559
=> 5591 * (i1 - i2) = 3124251
```

4. [GCD Test] Observe that this equation has integral solution iff gcd(3124251, 5591) exists. And, in fact, only 1 is gcd of them. Hence, by GCD test, i1 and i2 are not dependent.




k2=559, k3=124, k4=94, k5=144, k6=559
