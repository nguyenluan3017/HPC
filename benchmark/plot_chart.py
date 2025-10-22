import json
import matplotlib.pyplot as plt
import numpy as np

def load_json_data(filename):
    """Load JSON data from file"""
    with open(filename, 'r') as f:
        return json.load(f)

def generate_blocking_vs_nonblocking_table(blocking_data, nonblocking_data, blocking_name, nonblocking_name):
    """
    Generate markdown table comparing blocking vs non-blocking implementations

    Args:
        blocking_data: JSON data from block.json or blas-block.json
        nonblocking_data: JSON data from naive.json or blas.json
        blocking_name: Name for blocking implementation (e.g., "Block", "BLAS-Block")
        nonblocking_name: Name for non-blocking implementation (e.g., "Naive", "BLAS")

    Returns:
        str: Formatted markdown table
    """
    # Create header
    markdown = f"### {blocking_name} vs {nonblocking_name} Performance Comparison\n\n"
    markdown += f"| Matrix Size | {nonblocking_name} (seconds) | Best {blocking_name} Block Size | Best {blocking_name} Time (seconds) | Speedup ({nonblocking_name}/{blocking_name}) |\n"
    markdown += "|-------------|-----------------|-------------------------|---------------------|-------------------------------|\n"

    # Process data for each matrix size
    for blocking_item, nonblocking_item in zip(blocking_data, nonblocking_data):
        matrix_size = blocking_item['matrix-size']
        nonblocking_time = nonblocking_item['runtime']['average']

        # Find best block size and time for blocking implementation
        best_blocking_runtime = min(blocking_item['runtime'], key=lambda x: x['average'])
        best_block_size = best_blocking_runtime['block']
        best_blocking_time = best_blocking_runtime['average']

        # Calculate speedup
        speedup = nonblocking_time / best_blocking_time

        # Add row to table
        markdown += f"| {matrix_size} | {nonblocking_time:.6f} | {best_block_size} | {best_blocking_time:.6f} | {speedup:.2f}x |\n"

    return markdown

def plot_blocking_vs_nonblocking_comparison(blocking_data, nonblocking_data, blocking_name, nonblocking_name, save_name):
    """
    Plot bar chart comparing blocking vs non-blocking implementations

    Args:
        blocking_data: JSON data from block.json or blas-block.json
        nonblocking_data: JSON data from naive.json or blas.json
        blocking_name: Name for blocking implementation (e.g., "Block", "BLAS-Block")
        nonblocking_name: Name for non-blocking implementation (e.g., "Naive", "BLAS")
        save_name: Filename to save the plot
    """
    # Extract data
    matrix_sizes = []
    nonblocking_times = []
    best_blocking_times = []
    best_block_sizes = []

    for blocking_item, nonblocking_item in zip(blocking_data, nonblocking_data):
        matrix_size = blocking_item['matrix-size']
        nonblocking_time = nonblocking_item['runtime']['average']

        # Find best block size and time
        best_blocking_runtime = min(blocking_item['runtime'], key=lambda x: x['average'])
        best_blocking_time = best_blocking_runtime['average']
        best_block_size = best_blocking_runtime['block']

        matrix_sizes.append(matrix_size)
        nonblocking_times.append(nonblocking_time)
        best_blocking_times.append(best_blocking_time)
        best_block_sizes.append(best_block_size)

    # Create the plot
    fig, ax = plt.subplots(figsize=(14, 8))

    x = np.arange(len(matrix_sizes))
    width = 0.35

    # Create bars
    bars1 = ax.bar(x - width/2, nonblocking_times, width,
                   label=nonblocking_name, color='red', alpha=0.7)
    bars2 = ax.bar(x + width/2, best_blocking_times, width,
                   label=f'{blocking_name} (Best)', color='blue', alpha=0.7)

    # Customize the plot
    ax.set_xlabel('Matrix Size')
    ax.set_ylabel('Runtime (seconds)')
    ax.set_title(f'{blocking_name} vs {nonblocking_name} Performance Comparison')
    ax.set_xticks(x)
    ax.set_xticklabels(matrix_sizes)
    ax.legend()
    ax.grid(True, alpha=0.3)

    # Add value labels on bars
    def add_value_labels(bars, values, block_sizes=None):
        for i, (bar, value) in enumerate(zip(bars, values)):
            height = bar.get_height()
            label = f'{value:.3f}s'
            if block_sizes:
                label += f'\n(Block {block_sizes[i]})'
            ax.text(bar.get_x() + bar.get_width()/2., height + height*0.01,
                   label, ha='center', va='bottom', fontsize=9)

    add_value_labels(bars1, nonblocking_times)
    add_value_labels(bars2, best_blocking_times, best_block_sizes)

    # Add speedup annotations
    for i, (x_pos, nb_time, b_time) in enumerate(zip(x, nonblocking_times, best_blocking_times)):
        speedup = nb_time / b_time
        max_height = max(nb_time, b_time)
        ax.annotate(f'{speedup:.2f}x speedup',
                   xy=(x_pos, max_height + 30),
                   ha='center', va='bottom',
                   fontweight='bold', color='green',
                   fontsize=10)

    plt.tight_layout()
    plt.savefig(f'img/{save_name}', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    # Load data from JSON files
    naive_data = load_json_data('results/naive.json')
    blas_data = load_json_data('results/blas.json')
    block_data = load_json_data('results/block.json')
    blas_block_data = load_json_data('results/blas-block.json')

    print(generate_blocking_vs_nonblocking_table(block_data, naive_data, nonblocking_name="Naive", blocking_name="Block"))
    plot_blocking_vs_nonblocking_comparison(block_data, naive_data, blocking_name="Block", nonblocking_name="Naive", save_name="naive_versus_block.png")
    
    print(generate_blocking_vs_nonblocking_table(blas_block_data, blas_data, nonblocking_name="Cblas", blocking_name="Cblas-block"))
    plot_blocking_vs_nonblocking_comparison(blas_block_data, blas_data, blocking_name="Cblas", nonblocking_name="Cblas-block", save_name="cblas_versus_cblas-block.png")

if __name__ == "__main__":
    main()
