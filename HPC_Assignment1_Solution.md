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

3. [GCD Test] Dependencies occur if `s1 = s2`, which implies

```
   5591 * i1 + 559 = 5591 * i2 + 3124810
⇒ 5591 * i1 - 5591 * i2 = 3124810 - 559
⇒ 5591 * (i1 - i2) = 3124251              (1)
```

Observe that equation (1) has integral solution iff gcd(3124251, 5591) exists. And, in fact, only 1 is gcd of them. Hence, by GCD test, i1 and i2 are not dependent.

4. [Banerjee's Test] 

`s1 - s2 ⇔  5591 * (i1 - i2) - 3124251`

 Consider the four points:

```
(i1 = 0, i2 = 0)       ⇒ s1 - s2 = 5591 * (0 - 0) - 3124251       = -3124251
(i1 = 0, i2 = 5589)    ⇒ s1 - s2 = 5591 * (0 - 5589) - 3124251    = -34372350
(i1 = 5589, i2 = 0)    ⇒ s1 - s2 = 5591 * (5589 - 0) - 3124251    =  28123848
(i1 = 5589, i2 = 5589) ⇒ s1 - s2 = 5591 * (5589 - 5589) - 3124251 = -3124251
```

Thus, `L = -34372350, U = 28123848`. 

Note that `a0 - b0 = 3124810 - 559 = 3124251`. 
So `L < a0 - b0 < U`. This leads to the conclusion that s1 and s2 may have data dependence.

## Loop 2

```c
for (i=0; i<N; i++) { 
   a[k3*N+i] = b[i]*c[i]; 
   d[i] = a[(k3+1)*N+i]+e; 
}
```

Now substitute N=5590,k3=124 to obtain:

```c
for (i=0; i<N; i++) { 
   a[k3*N+i] = b[i]*c[i]; 
   d[i] = a[(k3+1)*N+i]+e; 
}
```

k3=124, k4=94, k5=144, k6=559
