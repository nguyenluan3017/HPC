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
   a[(5591)*i+559] = b[i]*c[i];  // statement 1: a1 = 5591, a0 = 559
   d[i] = a[(5591)*i+3124810]+e; // statement 2: b1 = 5591, b0 = 3124810
}
```

2. We have let `s1 = 5591 * i1 + 559` and `s2 = 5591 * i2 + 3124810`

3. By GCD Test, data dependence occur if `s1 = s2`, which implies

```
   5591 * i1 + 559 = 5591 * i2 + 3124810
⇒ 5591 * i1 - 5591 * i2 = 3124810 - 559
⇒ 5591 * (i1 - i2) = 3124251              (1)
```

Observe that equation (1) has integral solution iff `gcd(3124251, 5591)` exists. And, in fact, only 1 is gcd of them. Hence, by GCD test, i1 and i2 are not dependent.

4. Banerjee's Test

`s1 - s2 ⇔  5591 * (i1 - i2) - 3124251`

Consider the four points:

```
(i1 = 0, i2 = 0)       ⇒ s1 - s2 = 5591 * (0 - 0) - 3124251       = -3124251
(i1 = 0, i2 = 5589)    ⇒ s1 - s2 = 5591 * (0 - 5589) - 3124251    = -34372350
(i1 = 5589, i2 = 0)    ⇒ s1 - s2 = 5591 * (5589 - 0) - 3124251    =  28123848
(i1 = 5589, i2 = 5589) ⇒ s1 - s2 = 5591 * (5589 - 5589) - 3124251 = -3124251
```

Thus, `L = -34372350, U = 28123848`. 

Note that `b0 - a0 = 3124810 - 559 = 3124251`. 
So `L < b0 - a0 < U`. This doesn't break the dependence between statement 1 and statement 2.

Now let's add constraint `i1 ≤ i2`, which allows us to exclude the point `(i1 = 5589, i2 = 0)` from our test. Hence, we have

```
(i1 = 0, i2 = 0)       ⇒ s1 - s2 = 5591 * (0 - 0) - 3124251       = -3124251
(i1 = 0, i2 = 5589)    ⇒ s1 - s2 = 5591 * (0 - 5589) - 3124251    = -34372350
(i1 = 5589, i2 = 5589) ⇒ s1 - s2 = 5591 * (5589 - 5589) - 3124251 = -3124251
```

This implies `L = min(-3124251, -34372350, -3124251) = -34372350` and `U = max(-3124251, -34372350, -3124251) = -3124251`. But `b0 - a0 = 3124810 - 559 = 3124251 > U`. This breaks data dependence between statement1 and statement2. 

## Loop 2

1. Substitution:

Given the loop as below:

```c
for (i=0; i<N; i++) { 
   a[k3*N+i] = b[i]*c[i];  
   d[i] = a[(k3+1)*N+i]+e; 
}
```

Now substitute N=5590,k3=124 to obtain the new loop where statement 1 writes to and statement 2 reads from a

```c
for (i=0; i<5590; i++) { 
   a[693160+i] = b[i]*c[i];  // statement 1: a1 = 1, a0 = 693160
   d[i] = a[698750+i]+e;     // statement 2: b1 = 1, b0 = 698750
}
```

Let s1 = 693160+i and s2 = 698750+i

2. Calculation:

The data dependence occurs iff

```
   s1             = s2 for any 0 ≤ i1, i2 ≤ 5590
⇔ 693160+i1      = 698750+i2
⇔ i1 - i2 - 5590 = 0
```

3. By GCD Test, the data dependence occurs iff equation `i1 - i2 - 550 = 0 ⇔ i1 - i2 = 550` has integral solutions. Indeed, gcd(a1 = 1, b1 = 1) = 1 divides b0 - a0 = 550. Hence, GCD test doesn't break data dependence between statement 1 and statement 2.

4. By Banerjee's Test, 

Consider the four points:

```
(i1 = 0, i2 = 0)       ⇒ s1 - s2 = 0 - 0 - 5590       = -5590
(i1 = 0, i2 = 5589)    ⇒ s1 - s2 = 0 - 5589 - 5590    = -11179
(i1 = 5589, i2 = 0)    ⇒ s1 - s2 = 5589 - 0 - 5590    = -1
(i1 = 5589, i2 = 5589) ⇒ s1 - s2 = 5589 - 5589 - 5590 = -5590
```

Now `L = min(-5590, -11179, -1, -5590) = -11179`, `U = max(-5590, -11179, -1, -5590) = -1`. And `b0 - a0 = 698750 - 693160 = 5590` which is not in the range (-11179, -1). This break the dependence between statement 1 and statement 2.

## Loop 3




k3=124, k4=94, k5=144, k6=559
