#!/usr/bin/env python3
"""
用法：python3 extract_strings.py imgui_demo.cpp 302 737
输出 zh_CN.cpp 条目骨架（key=英文，value 留空供人工翻译）
"""
import sys, re

def extract(path, start, end):
    with open(path) as f:
        lines = f.readlines()
    chunk = ''.join(lines[start-1:end-1])
    strings = re.findall(r'"([^"\n]{2,80})"', chunk)
    seen = set()
    results = []
    skip = re.compile(r'^(##|###|%[0-9.]*[dfsx]$|[0-9.]+$)')
    for s in strings:
        if s in seen or skip.match(s): continue
        seen.add(s)
        results.append(s)
    for s in results:
        escaped = s.replace('\\', '\\\\').replace('"', '\\"')
        print(f'            {{"{escaped}", ""}},')

if __name__ == '__main__':
    extract(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))
