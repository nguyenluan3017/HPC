# Check intersection of indices for the given loop:
# for (int i = 0; i < 5590; i++) {
#    a[5590*i+94] = b[i]*c[i];    // Statement 1 (Write)
#    d[i] = a[5591*i+144]+e;      // Statement 2 (Read)
# }

# Generate sets of indices accessed by each statement
statement1_indices = set([5590 * i + 94 for i in range(5590)])  # Write indices
statement2_indices = set([5591 * i + 144 for i in range(5590)])  # Read indices

# Find intersection
intersection = statement1_indices.intersection(statement2_indices)

print("Statement 1 (Write) accesses indices:", min(statement1_indices), "to", max(statement1_indices))
print("Statement 2 (Read) accesses indices:", min(statement2_indices), "to", max(statement2_indices))
print("Intersection of indices:", intersection)
print("Number of intersecting indices:", len(intersection))

if len(intersection) == 0:
    print("No data dependency exists between Statement 1 and Statement 2")
else:
    print("Data dependency exists! Conflicting indices:", sorted(intersection))

# Additional analysis
print("\nRange analysis:")
print(f"Statement 1 range: [{min(statement1_indices)}, {max(statement1_indices)}]")
print(f"Statement 2 range: [{min(statement2_indices)}, {max(statement2_indices)}]")

# Check if ranges overlap
if max(statement1_indices) < min(statement2_indices):
    print(f"Gap between ranges: {min(statement2_indices) - max(statement1_indices) - 1} indices")
elif min(statement1_indices) > max(statement2_indices):
    print(f"Gap between ranges: {min(statement1_indices) - max(statement2_indices) - 1} indices")
else:
    print("Ranges overlap!")

# Mathematical verification
print("\nMathematical verification:")
print("For intersection to occur, we need: 5590*i1 + 94 = 5591*i2 + 144")
print("Rearranging: 5590*i1 - 5591*i2 = 50")
print("This is a linear Diophantine equation.")
print("Since gcd(5590, 5591) = 1 and 1 divides 50, solutions exist.")

# Find actual solutions within bounds
solutions = []
for i1 in range(5590):
    for i2 in range(5590):
        if 5590 * i1 + 94 == 5591 * i2 + 144:
            solutions.append((i1, i2, 5590 * i1 + 94))

print(f"\nActual solutions found: {len(solutions)}")
for sol in solutions:
    print(f"i1={sol[0]}, i2={sol[1]}, index={sol[2]}")