
print(
set([5591 * i + 559 for i in range(5590)]).intersection(
    set([5591 * i + 3124810 for i in range(5590)])
)
)