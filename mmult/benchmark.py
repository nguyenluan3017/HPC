#!/usr/bin/env python3
import sys
import os
import subprocess
import platform
import re
import argparse
import json
from contextlib import contextmanager
import math
import threading
import time
import pprint

@contextmanager
def spinner(message, chars="⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏"):
    stop_event = threading.Event()

    def spin():
        idx = 0
        while not stop_event.is_set():
            sys.stdout.write(f"\r{chars[idx % len(chars)]} {message}")
            sys.stdout.flush()
            idx += 1
            time.sleep(0.1)
        sys.stdout.write(f"\r{' ' * (len(message) + 2)}\r")
        sys.stdout.flush()

    spinner_thread = threading.Thread(target=spin)
    spinner_thread.daemon = True
    spinner_thread.start()

    try:
        yield
    finally:
        stop_event.set()
        spinner_thread.join()

def run_command(command, capture_output=True, text=True, shell=False):
    """
    Run a command and return the result with stdout, stderr, and return code.
    Compatible with Python 3.5+

    Args:
        command: Command to run (string if shell=True, list if shell=False)
        capture_output: Whether to capture stdout and stderr
        text: Whether to return output as text (True) or bytes (False)
        shell: Whether to run command through shell

    Returns:
        CompletedProcess object with stdout, stderr, returncode attributes
    """
    try:
        if capture_output:
            result = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=text,
                shell=shell,
                check=False
            )
        else:
            result = subprocess.run(
                command,
                universal_newlines=text,
                shell=shell,
                check=False
            )
        return result
    except Exception as e:
        print(f"Error running command: {e}")
        return None

def get_cpu_cache_linux():
    """Get CPU cache information on Linux"""
    cache_info = {}

    try:
        # Method 1: Try /proc/cpuinfo
        if os.path.exists('/proc/cpuinfo'):
            with open('/proc/cpuinfo', 'r') as f:
                content = f.read()

                # Look for cache size information
                cache_size_match = re.search(r'cache size\s*:\s*(\d+)\s*KB', content)
                if cache_size_match:
                    cache_info['l2_cache'] = f"{cache_size_match.group(1)} KB"

        # Method 2: Try lscpu command
        result = run_command(['lscpu'], shell=False)
        if result and result.returncode == 0:
            lines = result.stdout.split('\n')
            for line in lines:
                if 'L1d cache' in line:
                    cache_info['l1d_cache'] = line.split(':')[1].strip()
                elif 'L1i cache' in line:
                    cache_info['l1i_cache'] = line.split(':')[1].strip()
                elif 'L2 cache' in line:
                    cache_info['l2_cache'] = line.split(':')[1].strip()
                elif 'L3 cache' in line:
                    cache_info['l3_cache'] = line.split(':')[1].strip()

        # Method 3: Try /sys/devices/system/cpu/cpu0/cache/
        cache_path = '/sys/devices/system/cpu/cpu0/cache'
        if os.path.exists(cache_path):
            for cache_dir in os.listdir(cache_path):
                cache_full_path = os.path.join(cache_path, cache_dir)
                if os.path.isdir(cache_full_path):
                    try:
                        # Read cache level
                        level_file = os.path.join(cache_full_path, 'level')
                        if os.path.exists(level_file):
                            with open(level_file, 'r') as f:
                                level = f.read().strip()

                        # Read cache size
                        size_file = os.path.join(cache_full_path, 'size')
                        if os.path.exists(size_file):
                            with open(size_file, 'r') as f:
                                size = f.read().strip()

                        # Read cache type
                        type_file = os.path.join(cache_full_path, 'type')
                        if os.path.exists(type_file):
                            with open(type_file, 'r') as f:
                                cache_type = f.read().strip()

                        cache_key = f"l{level}_{cache_type.lower()}_cache"
                        cache_info[cache_key] = size
                    except:
                        continue

    except Exception as e:
        print(f"Error getting Linux cache info: {e}")

    return cache_info

def get_cpu_cache_macos():
    """Get CPU cache information on macOS"""
    cache_info = {}

    try:
        # Use sysctl to get cache information
        cache_queries = {
            'l1d_cache': 'hw.l1dcachesize',
            'l1i_cache': 'hw.l1icachesize',
            'l2_cache': 'hw.l2cachesize',
            'l3_cache': 'hw.l3cachesize'
        }

        for cache_name, sysctl_key in cache_queries.items():
            result = run_command(['sysctl', '-n', sysctl_key], shell=False)
            if result and result.returncode == 0 and result.stdout.strip():
                size_bytes = int(result.stdout.strip())
                if size_bytes > 0:
                    # Convert to KB or MB for readability
                    if size_bytes >= 1024 * 1024:
                        cache_info[cache_name] = f"{size_bytes // (1024 * 1024)} MB"
                    else:
                        cache_info[cache_name] = f"{size_bytes // 1024} KB"

        # Also try system_profiler for additional info
        result = run_command(['system_profiler', 'SPHardwareDataType'], shell=False)
        if result and result.returncode == 0:
            lines = result.stdout.split('\n')
            for line in lines:
                if 'Cache' in line and ':' in line:
                    parts = line.split(':')
                    if len(parts) == 2:
                        cache_name = parts[0].strip().lower().replace(' ', '_')
                        cache_size = parts[1].strip()
                        if cache_name not in cache_info:
                            cache_info[cache_name] = cache_size

    except Exception as e:
        print(f"Error getting macOS cache info: {e}")

    return cache_info

def get_system_info():
    """
    Get comprehensive system information including OS and CPU cache

    Returns:
        dict: System information including OS, CPU, and cache details
    """
    system = platform.system().lower()

    # Basic system info
    sys_info = {
        'os_name': 'unknown',
        'os_version': platform.release(),
        'platform': platform.platform(),
        'machine': platform.machine(),
        'processor': platform.processor(),
        'cpu_count': os.cpu_count(),
        'cache': {}
    }

    # Determine OS name
    if system == 'linux':
        sys_info['os_name'] = 'linux'
        sys_info['cache'] = get_cpu_cache_linux()
    elif system == 'darwin':
        sys_info['os_name'] = 'macos'
        sys_info['cache'] = get_cpu_cache_macos()
    elif system == 'windows':
        sys_info['os_name'] = 'windows'
        # Windows cache detection could be added here

    # Try to get CPU model name
    try:
        if sys_info['os_name'] == 'linux':
            with open('/proc/cpuinfo', 'r') as f:
                for line in f:
                    if line.startswith('model name'):
                        sys_info['cpu_model'] = line.split(':')[1].strip()
                        break
        elif sys_info['os_name'] == 'macos':
            result = run_command(['sysctl', '-n', 'machdep.cpu.brand_string'], shell=False)
            if result and result.returncode == 0:
                sys_info['cpu_model'] = result.stdout.strip()
    except:
        sys_info['cpu_model'] = 'Unknown'

    return sys_info

def print_system_info():
    """Print comprehensive system information"""
    info = get_system_info()

    print("=== System Information ===")
    print(f"OS Name: {info['os_name'].upper()}")
    print(f"OS Version: {info['os_version']}")
    print(f"Platform: {info['platform']}")
    print(f"Architecture: {info['machine']}")
    print(f"CPU Count: {info['cpu_count']}")

    if 'cpu_model' in info:
        print(f"CPU Model: {info['cpu_model']}")

    print(f"\n=== CPU Cache Information ===")
    if info['cache']:
        for cache_name, cache_size in info['cache'].items():
            print(f"{cache_name.upper().replace('_', ' ')}: {cache_size}")
    else:
        print("Cache information not available")

    return info

def get_maximum_block_sizes(cache_info):
    """
    Suggest optimal block sizes based on cache information

    Args:
        cache_info: Dictionary containing cache sizes

    Returns:
        list: Suggested block sizes for matrix multiplication
    """
    block_sizes = set()
    for _, sz_text in cache_info.items():
        size_match = re.search(r'(\d+)\s*(\S+)', sz_text.upper())
        size = 0
        if size_match:
            size_val = int(size_match.group(1))
            unit = size_match.group(2)
            if unit.startswith("M"):
                size = size_val * 1024
            else:
                size = size_val
        if size > 0:
            max_block_size_squared = (size * 1024) // (3 * 8)  # Convert KB to bytes, divide by 24
            max_block_size = int(max_block_size_squared ** 0.5)
            block_sizes.add(max_block_size)
    return sorted(list(block_sizes))

def gcd(*numbers):
    if not numbers:
        return 0
    if len(numbers) == 1:
        return abs(numbers[0])
    if len(numbers) == 2:
        a, b = numbers[0], numbers[1]
        while b:
            a, b = b, a % b
        return a
    result = abs(numbers[0])
    for num in numbers[1:]:
        result = gcd(result, abs(num))
        if result == 1:
            break
    return result

def lcm(*numbers):
    if not numbers:
        return 0
    if len(numbers) == 1:
        return numbers[0]
    if len(numbers) == 2:
        a, b = numbers[0], numbers[1]
        return (a // gcd(a, b)) * b
    result = abs(numbers[0])
    for num in numbers[1:]:
        result = lcm(result, abs(num))
    return result

def divisors(number):
    limit = int(math.sqrt(number))
    div = [d for d in range(2, limit) if number % d == 0]
    ans = div + [number // d for d in div if number // d != d]
    return list(sorted(ans))

def output_result(result, output_dir, variant):
    os.makedirs(output_dir, exist_ok=True)

    filename = f"{variant}.json"
    output_file = os.path.join(output_dir, filename)

    json_result = json.dumps(result, indent=4, sort_keys=True)
    with open(output_file, 'w') as f:
        f.write(json_result)

    print(f"\n=== Results written to {output_file} ===")

def run_benchmark_variant(variant, repeat, exec_name, test_scenarios, output_dir):
    results = []
    cache_blocking = 'block' in variant

    def run_mmult(matrix_size, block_size = None):
        command = [
            exec_name,
            "--variant", variant,
            "--size", str(matrix_size),
            "--repeat", str(repeat),
        ]

        if block_size:
            command.extend(["--block", str(block_size)])

        spinner_msg = f"Running: {' '.join(command)}"
        with spinner(spinner_msg):
            output = run_command(command)

        if output and output.returncode == 0:
            runtime_match = re.search(r'\d+\.\d+', output.stdout.upper())
            if runtime_match:
                runtime_val = float(runtime_match.group())
                avg_time = runtime_val / repeat
                run = {
                    "average": avg_time if avg_time >= 1e-6 else round(avg_time, 6),
                    "total": runtime_val,
                    "repeat": repeat,
                }
                if cache_blocking:
                    run['block'] = block_size

            if cache_blocking:
                print(f"✅ Total runtime for block size {block_size}: {runtime_val:.6f}s")
            else:
                print(f"✅ Total runtime: {runtime_val:.6f}s")
            return run
        else:
            print(f"❌ Execution error: {output.stderr}")

    for matrix_size, block_sizes in test_scenarios.items():
        print(f"\n{10 * '*'} MATRIX SIZE: {matrix_size} {10 * '*'}")
        results.append({
            "matrix-size": matrix_size,
            "variant": variant,
            "runtime": [
                run_mmult(matrix_size, block_size) 
                    for block_size in block_sizes
            ] if cache_blocking else run_mmult(matrix_size)
        })
    output_result(results, output_dir, variant)

def build_mmult():
    os_name_short = platform.system().lower()
    exec_path = os.path.join(os.getcwd(), "bin", f"mmult.{os_name_short}")

    print("Attempting to build with build-mmult.sh...")
    build_script = os.path.join(os.getcwd(), "build-mmult.sh")
    if not os.path.exists(build_script):
        print(f"Build script not found: {build_script}")
        return False

    # Make sure the build script is executable
    try:
        os.chmod(build_script, os.stat(build_script).st_mode | 0o111)
    except Exception as e:
        print(f"Warning: could not chmod build script: {e}")

    with spinner("Building mmult binary"):
        build_result = run_command([build_script], shell=True)

    if not build_result or build_result.returncode != 0:
        print("Failed to build mmult binary: ", build_result.stderr)
        if build_result:
            print(build_result.stdout or "")
            print(build_result.stderr or "")
        return False

    # Verify the binary exists after build
    if not os.path.exists(exec_path):
        print(f"Build finished but mmult binary still missing at {exec_path}")
        return False

    print(f"Build successful: {exec_path} found.")
    return exec_path

def main():
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument("--output-dir", type=str, help="Output directory for results", required=True)
    args = args_parser.parse_args()

    print("Matrix Multiplication Benchmark - All Variants")
    print("=" * 60)

    # Ensure build directory exists and the mmult binary is present; build if missing.
    exec_name = build_mmult()
    if not exec_name:
        return -1

    # Get and print system information
    sys_info = print_system_info()
    variants = [('blas-block', 50), ('block', 25), ('naive', 5)]

    # Suggest optimal block sizes
    print("\n=== Test Scenarios ===")
    max_block_size = max(get_maximum_block_sizes(sys_info['cache']))
    test_scenarios = {
        matrix_size: sorted([16, 128, 256, 512]) for matrix_size in range(1024, 4097, 512)
    }

    print(f"📊 Maximum recommended block size: {max_block_size}")
    print()

    print("┌" + "─" * 15 + "┬" + "─" * 8 + "┬" + "─" * 50 + "┐")
    print("│ Matrix Size   │ Count  │ Block Sizes" + " " * 38 + "│")
    print("├" + "─" * 15 + "┼" + "─" * 8 + "┼" + "─" * 50 + "┤")

    for matrix_size, block_sizes in test_scenarios.items():
        size_str = f"{matrix_size:,} x {matrix_size:,}"
        count_str = f"{len(block_sizes)}"
        
        # Format first line of block sizes
        if block_sizes:
            blocks_str = ", ".join(str(b) for b in block_sizes[:6])
            if len(block_sizes) > 6:
                blocks_str += f", ... (+{len(block_sizes) - 6} more)"
        else:
            blocks_str = "None available"
        
        print(f"│ {size_str:<13} │ {count_str:>6} │ {blocks_str:<48} │")

    print("└" + "─" * 15 + "┴" + "─" * 8 + "┴" + "─" * 50 + "┘")
    print()    
        
    for variant, repeat in variants:
        print(f"\n=== Running Benchmarks for {variant} (repeat={repeat}) ===")
        run_benchmark_variant(variant, repeat, exec_name, test_scenarios, args.output_dir)

    print("\n🎉 All benchmarks completed!")
    print(f"📁 Results saved to: {args.output_dir}/")
    print("📄 Files created:")
    for variant, _ in variants:
        output_file = os.path.join(args.output_dir, f"{variant}.json")
        if os.path.exists(output_file):
            print(f"   - {variant}.json")

    return 0

if __name__ == '__main__':
    sys.exit(main())
