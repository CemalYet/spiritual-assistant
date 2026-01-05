#!/usr/bin/env python3
"""
Optimize web assets for faster loading:
1. Minify HTML/CSS/JS (remove whitespace, comments)
2. Gzip the minified files
3. Show size comparison
"""

import os
import gzip
import re
from pathlib import Path

def minify_html(content):
    """Basic HTML minification"""
    # Remove comments
    content = re.sub(r'<!--.*?-->', '', content, flags=re.DOTALL)
    # Remove extra whitespace between tags
    content = re.sub(r'>\s+<', '><', content)
    # Remove leading/trailing whitespace on lines
    content = '\n'.join(line.strip() for line in content.splitlines() if line.strip())
    return content

def minify_css(content):
    """Basic CSS minification"""
    # Remove comments
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    # Remove extra whitespace
    content = re.sub(r'\s+', ' ', content)
    # Remove spaces around colons, semicolons, braces
    content = re.sub(r'\s*([{}:;,])\s*', r'\1', content)
    return content.strip()

def minify_js(content):
    """Basic JS minification"""
    # Remove single-line comments (but keep URLs)
    content = re.sub(r'(?<!:)//.*?$', '', content, flags=re.MULTILINE)
    # Remove multi-line comments
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    # Remove extra whitespace
    content = re.sub(r'\s+', ' ', content)
    # Remove spaces around operators
    content = re.sub(r'\s*([{}();,=<>+\-*/])\s*', r'\1', content)
    return content.strip()

def compress_file(input_path, output_path, minifier=None):
    """Read, optionally minify, and gzip compress a file"""
    with open(input_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_size = len(content.encode('utf-8'))
    
    # Minify if minifier provided
    if minifier:
        content = minifier(content)
        minified_size = len(content.encode('utf-8'))
    else:
        minified_size = original_size
    
    # Gzip compress
    compressed = gzip.compress(content.encode('utf-8'), compresslevel=9)
    
    with open(output_path, 'wb') as f:
        f.write(compressed)
    
    compressed_size = len(compressed)
    
    return original_size, minified_size, compressed_size

def main():
    # Get the data directory
    script_dir = Path(__file__).parent
    data_dir = script_dir.parent / 'data'
    
    if not data_dir.exists():
        print(f"Error: {data_dir} does not exist")
        return
    
    print("=== Web Asset Optimization ===\n")
    
    files = [
        ('index.html', minify_html),
        ('success.html', minify_html),
        ('style.css', minify_css),
        ('script.js', minify_js),
    ]
    
    total_original = 0
    total_minified = 0
    total_compressed = 0
    
    for filename, minifier in files:
        input_path = data_dir / filename
        output_path = data_dir / f"{filename}.gz"
        
        if not input_path.exists():
            print(f"⚠ Skipping {filename} (not found)")
            continue
        
        try:
            orig, mini, comp = compress_file(input_path, output_path, minifier)
            
            mini_percent = ((orig - mini) / orig * 100) if orig > 0 else 0
            comp_percent = ((orig - comp) / orig * 100) if orig > 0 else 0
            
            print(f"✓ {filename}:")
            print(f"  Original:    {orig:6d} bytes")
            print(f"  Minified:    {mini:6d} bytes ({mini_percent:5.1f}% reduction)")
            print(f"  Compressed:  {comp:6d} bytes ({comp_percent:5.1f}% total reduction)")
            print()
            
            total_original += orig
            total_minified += mini
            total_compressed += comp
            
        except Exception as e:
            print(f"✗ Error processing {filename}: {e}\n")
    
    if total_original > 0:
        total_mini_percent = ((total_original - total_minified) / total_original * 100)
        total_comp_percent = ((total_original - total_compressed) / total_original * 100)
        
        print("=" * 40)
        print(f"Total Original:    {total_original:6d} bytes")
        print(f"Total Minified:    {total_minified:6d} bytes ({total_mini_percent:5.1f}% reduction)")
        print(f"Total Compressed:  {total_compressed:6d} bytes ({total_comp_percent:5.1f}% total reduction)")
        print(f"\nTotal savings: {total_original - total_compressed} bytes")

if __name__ == '__main__':
    main()
