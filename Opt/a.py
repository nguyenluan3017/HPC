S = "s"
A = "a"
C = "c"
B = "b"
D = "d"
T = "t"

edges = [
	[S, A, 16],
	[S, C, 13],
	[A, C, 10],
	[A, B, 12],
	[C, A, 4],
	[C, D, 14],
	[B, C, 9],
	[B, T, 20],
	[D, B, 7],
	[D, T, 4]
]

vertices = [S, A, C, B, D, T]

adj_in = {}
adj_out = {}
flow_in = {}
flow_out = {}

for start, end, weight in edges:
	adj_list = adj_out.get(start, {})
	adj_list[end] = weight  
	adj_out[start] = adj_list

	adj_list = adj_in.get(end, {})
	adj_list[start] = weight
	adj_in[end] = adj_list

	flow_out[start] = flow_out.get(start, 0) + weight
	flow_in[end] = flow_in.get(end, 0) + weight

for start, end, weight in edges:
    print(f"@variable(model, 0 <= {start}{end} <= {weight})")

for vertex in vertices:
    #print(vertex, adj_in.get(vertex, {}), adj_out.get(vertex, {}))
    in_edge = [neightbor + vertex for neightbor, weight in adj_in.get(vertex, {}).items()]
    out_edge = [vertex + neightbor for neightbor, weight in adj_out.get(vertex, {}).items()]
    if len(in_edge) > 0 and len(out_edge) > 0:
        print(f"@constraint(model, {" + ".join(in_edge)} == {" + ".join(out_edge)})")
    
for start, end, weight in edges:
    print(f"println(\"Flow through {start}{end} is \", value({start}{end}),\"/{weight}\")")