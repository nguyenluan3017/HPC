# Check intersection of indices for Loop 4:
# for (int i = 0; i < 5590; i++) {
#    a[5591*i] = b[i]*c[i];           // Statement 1 (Write)
#    d[i] = a[3130400+i]+e;           // Statement 2 (Read)
# }

# Generate sets of indices accessed by each statement
statement1_indices = set([5591 * i for i in range(5590)])  # Write indices
statement2_indices = set([3130400 + i for i in range(5590)])  # Read indices

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
print("For intersection to occur, we need: 5591*i1 = 3130400 + i2")
print("Rearranging: 5591*i1 - i2 = 3130400")
print("This is a linear Diophantine equation.")
print("Since gcd(5591, 1) = 1 and 1 divides 3130400, solutions exist.")

# Find actual solutions within bounds
solutions = []
for i1 in range(5590):
    index1 = 5591 * i1
    if 3130400 <= index1 <= 3130400 + 5589:  # Check if index1 is in statement2's range
        i2 = index1 - 3130400
        if 0 <= i2 < 5590:
            solutions.append((i1, i2, index1))

print(f"\nActual solutions found: {len(solutions)}")
for sol in solutions:
    print(f"i1={sol[0]}, i2={sol[1]}, index={sol[2]}")

# Show first few write indices and read range for clarity
print(f"\nFirst 10 write indices: {sorted(list(statement1_indices))[:10]}")
print(f"Last 10 write indices: {sorted(list(statement1_indices))[-10:]}")
print(f"Read indices range: {min(statement2_indices)} to {max(statement2_indices)}")