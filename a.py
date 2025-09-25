# Check intersection of indices for the given loop:
# for (i=0; i<5590; i++) { 
#    a[693160+i] = b[i]*c[i];  // Statement 1 (Write)
#    d[i] = a[698750+i]+e;     // Statement 2 (Read)
# }

# Generate sets of indices accessed by each statement
statement1_indices = set([693160 + i for i in range(5590)])  # Write indices
statement2_indices = set([698750 + i for i in range(5590)])  # Read indices

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
print(f"Gap between ranges: {min(statement2_indices) - max(statement1_indices) - 1} indices")