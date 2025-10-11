#!/usr/bin/env python3
import sys
import os
import subprocess
import platform
import re

def run_command(command, capture_output=True, text=True, shell=False):
    """
    Run a command and return the result with stdout, stderr, and return code.
    
    Args:
        command: Command to run (string if shell=True, list if shell=False)
        capture_output: Whether to capture stdout and stderr
        text: Whether to return output as text (True) or bytes (False)
        shell: Whether to run command through shell
    
    Returns:
        CompletedProcess object with stdout, stderr, returncode attributes
    """
    try:
        result = subprocess.run(
            command,
            capture_output=capture_output,
            text=text,
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

def get_optimal_block_sizes(cache_info):
    """
    Suggest optimal block sizes based on cache information
    
    Args:
        cache_info: Dictionary containing cache sizes
    
    Returns:
        list: Suggested block sizes for matrix multiplication
    """
    block_sizes = {}    

    try:
        for lv, sz_text in cache_info.items():
            if sz_text:
                # Parse cache size (handle KB, MB)
                size_match = re.search(r'(\d+)\s*(KB|MB)', sz_text.upper())
                size = 0
                if size_match:
                    size_val = int(size_match.group(1))
                    unit = size_match.group(2)
                    if unit == 'MB':
                        size = size_val * 1024  # Convert to KB
                    else:
                        size = size_val
            
            if size > 0:
                # Calculate block size that fits in L1 cache
                # For matrix multiplication, we need 3 blocks: A, B, C
                # Each block is block_size^2 * sizeof(double) = block_size^2 * 8 bytes
                # Total memory = 3 * block_size^2 * 8 bytes
                # We want this to fit in L1 cache (in KB)
                
                max_block_size_squared = (size * 1024) // (3 * 8)  # Convert KB to bytes, divide by 24
                max_block_size = int(max_block_size_squared ** 0.5)
                
                # Generate reasonable block sizes
                suggested_sizes = []
                for size in [16, 24, 32, 48, 64, 96, 128, 192, 256]:
                    if size <= max_block_size:
                        suggested_sizes.append(size)

                if suggested_sizes:
                    block_sizes[lv] = suggested_sizes
        
    except Exception as e:
        print(f"Error calculating optimal block sizes: {e}")
    
    return block_sizes

def main():
    """Main function to demonstrate system information gathering"""
    print("System Information and CPU Cache Detection")
    print("=" * 50)
    
    # Get and print system information
    sys_info = print_system_info()
    exec_name = "./bin/mmult." + sys_info["os_name"]

    # Suggest optimal block sizes
    print(f"\n=== Optimization Suggestions ===")
    block_sizes = get_optimal_block_sizes(sys_info['cache'])
    print(f"Suggested block sizes for matrix multiplication:")
    for lv, block_size in block_sizes.items():
        print(f"{lv}: {block_size}")
    
    # Example usage for benchmarking
    print(f"\n=== Example Benchmark Commands ===")
    for lv, sizes in block_sizes.items():
        for blk_size in sizes:
            print(f"{exec_name} --variant block --size 1024 --block {blk_size} --repeat 1000")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
